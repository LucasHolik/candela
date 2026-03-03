#include "brightness.h"
#include "colortemp.h"
#include <windows.h>
#include <vector>
#include <string>
#include <highlevelmonitorconfigurationapi.h>
#include <physicalmonitorenumerationapi.h>
#include <cmath>
#include <algorithm>

// Internal constants for brightness mapping
namespace
{
  // Software brightness lower bound (0-100 scale) to prevent the screen from going fully black.
  // Setting this too low can make the screen unreadable.
  const int MIN_SAFE_SOFTWARE_BRIGHTNESS = 49;
  const int MAX_BRIGHTNESS = 100;
  const int MIN_INPUT_BRIGHTNESS = 1;
}

double MapBrightnessToSafeFactor(int brightness)
{
  // Clamp input
  brightness = std::max(MIN_INPUT_BRIGHTNESS, std::min(brightness, MAX_BRIGHTNESS));

  // Linear mapping:
  // Out = MinSafe + (In - MinIn) * (MaxSafe - MinSafe) / (MaxIn - MinIn)
  return static_cast<double>(MIN_SAFE_SOFTWARE_BRIGHTNESS) +
         (static_cast<double>(brightness - MIN_INPUT_BRIGHTNESS) *
          (static_cast<double>(MAX_BRIGHTNESS - MIN_SAFE_SOFTWARE_BRIGHTNESS)) /
          (static_cast<double>(MAX_BRIGHTNESS - MIN_INPUT_BRIGHTNESS)));
}

// Global internal state
static std::vector<Monitor> g_monitors;
static bool g_initialized = false;

// Forward declaration of the callback
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT, LPARAM dwData);

// -----------------------------------------------------------------------------------------------
// Helper Functions
// (MapBrightnessToSafeFactor defined above, before the anonymous namespace, for external linkage)
// -----------------------------------------------------------------------------------------------

// -----------------------------------------------------------------------------------------------
// BrightnessController Implementation
// -----------------------------------------------------------------------------------------------

bool BrightnessController::Initialize()
{
  if (g_initialized)
  {
    return true;
  }

  // Refresh the list to initialize
  if (RefreshMonitors())
  {
    g_initialized = true;
    return true;
  }
  return false;
}

bool BrightnessController::RefreshMonitors()
{
  Cleanup(); // Release existing resources first

  g_monitors.clear();
  // Enumerate display monitors
  EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&g_monitors));

  if (!g_monitors.empty())
  {
    g_initialized = true;
    return true;
  }
  return false;
}

void BrightnessController::Cleanup()
{
  for (auto &monitor : g_monitors)
  {
    // Clean up Device Contexts
    if (monitor.hdc)
    {
      DeleteDC(monitor.hdc);
      monitor.hdc = nullptr;
    }
    // Clean up Physical Monitor handles
    if (monitor.hPhysicalMonitor)
    {
      DestroyPhysicalMonitor(monitor.hPhysicalMonitor);
      monitor.hPhysicalMonitor = nullptr;
      monitor.supportsHardwareBrightness = false;
    }
  }
  g_monitors.clear();
  g_initialized = false;
}

const std::vector<Monitor> &BrightnessController::GetMonitors()
{
  if (!g_initialized)
  {
    Initialize();
  }
  return g_monitors;
}

bool BrightnessController::SetSoftwareBrightness(int monitorIndex, int brightness)
{
  if (monitorIndex < 0 || static_cast<size_t>(monitorIndex) >= g_monitors.size())
    return false;

  Monitor &monitor = g_monitors[monitorIndex];
  if (!monitor.hdc)
    return false;

  brightness = std::max(MIN_INPUT_BRIGHTNESS, std::min(brightness, MAX_BRIGHTNESS));
  monitor.softwareBrightness = brightness;
  return ColorTempUtils::ApplyGammaRamp(monitor.hdc, brightness, monitor.softwareColorTemp);
}

bool BrightnessController::SetSoftwareColorTemp(int monitorIndex, int kelvin)
{
  if (monitorIndex < 0 || static_cast<size_t>(monitorIndex) >= g_monitors.size())
    return false;

  Monitor &monitor = g_monitors[monitorIndex];
  if (!monitor.hdc)
    return false;

  kelvin = std::max(ColorTempUtils::KELVIN_MIN, std::min(kelvin, ColorTempUtils::KELVIN_MAX));
  monitor.softwareColorTemp = kelvin;
  return ColorTempUtils::ApplyGammaRamp(monitor.hdc, monitor.softwareBrightness, kelvin);
}

int BrightnessController::GetSoftwareColorTemp(int monitorIndex)
{
  if (monitorIndex < 0 || static_cast<size_t>(monitorIndex) >= g_monitors.size())
    return -1;
  return g_monitors[monitorIndex].softwareColorTemp;
}

bool BrightnessController::SetHardwareBrightness(int monitorIndex, int brightness)
{
  if (monitorIndex < 0 || static_cast<size_t>(monitorIndex) >= g_monitors.size())
    return false;

  Monitor &monitor = g_monitors[monitorIndex];
  if (!monitor.supportsHardwareBrightness)
    return false;

  brightness = std::max(0, std::min(brightness, MAX_BRIGHTNESS));

  monitor.hardwareBrightness = brightness;

  // Map the normalized 0-100 value to the monitor's native brightness range
  DWORD nativeBrightness = monitor.hwNativeMin;
  if (monitor.hwNativeMax > monitor.hwNativeMin)
  {
    nativeBrightness = monitor.hwNativeMin +
                       static_cast<DWORD>(std::round(
                           static_cast<double>(brightness) *
                           (monitor.hwNativeMax - monitor.hwNativeMin) / 100.0));
  }

  for (int attempt = 1; attempt <= 5; ++attempt)
  {
    if (SetMonitorBrightness(monitor.hPhysicalMonitor, nativeBrightness))
    {
      return true;
    }
    Sleep(50);
  }

  return false;
}

int BrightnessController::GetSoftwareBrightness(int monitorIndex)
{
  if (monitorIndex < 0 || static_cast<size_t>(monitorIndex) >= g_monitors.size())
    return -1;
  return g_monitors[monitorIndex].softwareBrightness;
}

int BrightnessController::GetHardwareBrightness(int monitorIndex)
{
  if (monitorIndex < 0 || static_cast<size_t>(monitorIndex) >= g_monitors.size())
    return -1;
  return g_monitors[monitorIndex].hardwareBrightness;
}

// -----------------------------------------------------------------------------------------------
// Callbacks
// -----------------------------------------------------------------------------------------------

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT, LPARAM dwData)
{
  (void)hdcMonitor; // Unused
  auto *monitors = reinterpret_cast<std::vector<Monitor> *>(dwData);
  Monitor monitor;
  monitor.hMonitor = hMonitor;

  MONITORINFOEX monitorInfoEx;
  monitorInfoEx.cbSize = sizeof(MONITORINFOEX);
  if (GetMonitorInfo(hMonitor, &monitorInfoEx))
  {
    monitor.deviceName = monitorInfoEx.szDevice;
    // Create a dedicated DC for this monitor for software brightness
    monitor.hdc = CreateDC(nullptr, monitorInfoEx.szDevice, nullptr, nullptr);
  }

  // Attempt to get Physical Monitor for Hardware Brightness (DDC/CI)
  DWORD monitorCount = 0;
  if (GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &monitorCount) && monitorCount > 0)
  {
    auto *physicalMonitors = new PHYSICAL_MONITOR[monitorCount];

    for (int attempt = 1; attempt <= 5; ++attempt)
    {
      if (GetPhysicalMonitorsFromHMONITOR(hMonitor, monitorCount, physicalMonitors))
      {
        if (physicalMonitors[0].hPhysicalMonitor != nullptr)
        {
          monitor.hPhysicalMonitor = physicalMonitors[0].hPhysicalMonitor;

          // Close handles for any additional physical monitors (unsupported in this version)
          for (DWORD i = 1; i < monitorCount; i++)
          {
            DestroyPhysicalMonitor(physicalMonitors[i].hPhysicalMonitor);
          }
          break;
        }
        else
        {
          // Call succeeded but returned a null handle — release all entries before retrying
          for (DWORD i = 0; i < monitorCount; i++)
          {
            if (physicalMonitors[i].hPhysicalMonitor)
              DestroyPhysicalMonitor(physicalMonitors[i].hPhysicalMonitor);
          }
        }
      }
      Sleep(100);
    }

    delete[] physicalMonitors;
  }

  // Initialize current values

  // 1. Software Brightness (Reverse calculation from Gamma Ramp)
  if (monitor.hdc)
  {
    WORD currentGammaRamp[256 * 3];
    if (GetDeviceGammaRamp(monitor.hdc, currentGammaRamp))
    {
      // Calculate brightness factor from the top of the ramp (white point)
      double factor = (double)currentGammaRamp[255] / (255.0 * 257.0);

      // Reverse map from Safe Range to 1-100 Range
      // remapped = factor * 100
      // brightness = ((remapped - MinSafe) * (MaxIn - MinIn) / (MaxSafe - MinSafe)) + MinIn

      double remapped = factor * 100.0;
      double brightness = ((remapped - MIN_SAFE_SOFTWARE_BRIGHTNESS) *
                           (MAX_BRIGHTNESS - MIN_INPUT_BRIGHTNESS) /
                           (MAX_BRIGHTNESS - MIN_SAFE_SOFTWARE_BRIGHTNESS)) +
                          MIN_INPUT_BRIGHTNESS;

      monitor.softwareBrightness = static_cast<int>(std::round(brightness));

      // Clamp
      monitor.softwareBrightness = std::max(MIN_INPUT_BRIGHTNESS, std::min(monitor.softwareBrightness, MAX_BRIGHTNESS));
    }
  }

  // 2. Hardware Brightness
  if (monitor.hPhysicalMonitor != nullptr)
  {
    DWORD minB, curB, maxB;
    bool success = false;
    for (int attempt = 1; attempt <= 5; ++attempt)
    {
      if (GetMonitorBrightness(monitor.hPhysicalMonitor, &minB, &curB, &maxB))
      {
        monitor.hwNativeMin = minB;
        monitor.hwNativeMax = maxB;
        if (maxB > minB)
        {
          // Clamp curB to [minB, maxB] before arithmetic: some DDC/CI implementations
          // return values slightly outside the reported range, and unsigned underflow
          // on DWORD subtraction would produce UB when cast back to int.
          curB = std::max(minB, std::min(curB, maxB));
          double normalized = static_cast<double>(curB - minB) / (maxB - minB) * 100.0;
          monitor.hardwareBrightness = static_cast<int>(std::round(normalized));
        }
        else
        {
          monitor.hardwareBrightness = 50; // Native range indeterminate; use midpoint
        }
        monitor.supportsHardwareBrightness = true;
        success = true;
        break;
      }
      else
      {
        Sleep(50); // Wait 50ms before retrying
      }
    }

    if (!success)
    {
      monitor.supportsHardwareBrightness = false;
    }
  }

  monitors->push_back(monitor);
  return TRUE;
}