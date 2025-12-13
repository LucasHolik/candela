#include "brightness.h"
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

// Global internal state
static std::vector<Monitor> g_monitors;
static bool g_initialized = false;

// Forward declaration of the callback
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT, LPARAM dwData);

// -----------------------------------------------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------------------------------------------

/**
 * @brief Maps a linear brightness value (1-100) to a safe gamma range (MIN_SAFE_SOFTWARE_BRIGHTNESS-100).
 *
 * @param brightness Input brightness 1-100
 * @return Remapped brightness value
 */
static double MapBrightnessToSafeRange(int brightness)
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

// -----------------------------------------------------------------------------------------------
// BrightnessController Implementation
// -----------------------------------------------------------------------------------------------

bool BrightnessController::Initialize()
{
  if (g_initialized)
    return true;

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

  return !g_monitors.empty();
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

  // Clamp brightness
  brightness = std::max(MIN_INPUT_BRIGHTNESS, std::min(brightness, MAX_BRIGHTNESS));

  monitor.softwareBrightness = brightness;

  // Calculate factor based on remapped safe range
  double remapped = MapBrightnessToSafeRange(brightness);
  double factor = remapped / 100.0;

  // Build the gamma ramp
  WORD newGammaRamp[256 * 3];
  for (int i = 0; i < 256; i++)
  {
    int value = static_cast<int>(i * factor * 257);
    // Clamp to WORD range
    value = std::max(0, std::min(value, 65535));

    // R, G, B channels
    newGammaRamp[i] = static_cast<WORD>(value);
    newGammaRamp[i + 256] = static_cast<WORD>(value);
    newGammaRamp[i + 512] = static_cast<WORD>(value);
  }

  return SetDeviceGammaRamp(monitor.hdc, newGammaRamp);
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

  return SetMonitorBrightness(monitor.hPhysicalMonitor, brightness);
}

int BrightnessController::GetSoftwareBrightness(int monitorIndex)
{
  if (monitorIndex < 0 || static_cast<size_t>(monitorIndex) >= g_monitors.size())
    return MAX_BRIGHTNESS;
  return g_monitors[monitorIndex].softwareBrightness;
}

int BrightnessController::GetHardwareBrightness(int monitorIndex)
{
  if (monitorIndex < 0 || static_cast<size_t>(monitorIndex) >= g_monitors.size())
    return 50; // Default center
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
    if (GetPhysicalMonitorsFromHMONITOR(hMonitor, monitorCount, physicalMonitors))
    {
      // Support the first physical monitor associated with this logical monitor
      monitor.hPhysicalMonitor = physicalMonitors[0].hPhysicalMonitor;

      // Close handles for any additional physical monitors (unsupported in this version)
      for (DWORD i = 1; i < monitorCount; i++)
      {
        DestroyPhysicalMonitor(physicalMonitors[i].hPhysicalMonitor);
      }
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
  if (monitor.hPhysicalMonitor)
  {
    DWORD minB, curB, maxB;
    if (GetMonitorBrightness(monitor.hPhysicalMonitor, &minB, &curB, &maxB))
    {
      monitor.hardwareBrightness = curB;
      monitor.supportsHardwareBrightness = true;
    }
  }

  monitors->push_back(monitor);
  return TRUE;
}