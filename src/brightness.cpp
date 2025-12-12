#include "brightness.h"
#include <windows.h>
#include <vector>
#include <string>
#include <highlevelmonitorconfigurationapi.h>
#include <physicalmonitorenumerationapi.h>
#include <cmath>

// Global variables
static std::vector<Monitor> g_monitors;
static bool g_initialized = false;

// Callback function for monitor enumeration
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT, LPARAM dwData)
{
    (void)hdcMonitor; // Unused
    std::vector<Monitor>* monitors = reinterpret_cast<std::vector<Monitor>*>(dwData);
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

    // Get Physical Monitor for Hardware Brightness
    DWORD monitorCount = 0;
    if (GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &monitorCount) && monitorCount > 0)
    {
        PHYSICAL_MONITOR* physicalMonitors = new PHYSICAL_MONITOR[monitorCount];
        if (GetPhysicalMonitorsFromHMONITOR(hMonitor, monitorCount, physicalMonitors))
        {
            // We only support the first physical monitor per logical monitor for now
            monitor.hPhysicalMonitor = physicalMonitors[0].hPhysicalMonitor;
            monitor.supportsHardwareBrightness = true;

            // Close the handles for the others if any (rare case for standard setups)
            for (DWORD i = 1; i < monitorCount; i++)
            {
                DestroyPhysicalMonitor(physicalMonitors[i].hPhysicalMonitor);
            }
        }
        delete[] physicalMonitors;
    }

    // Get initial values
    // Software
    if (monitor.hdc) {
        WORD currentGammaRamp[256 * 3];
        if (GetDeviceGammaRamp(monitor.hdc, currentGammaRamp)) {
            // Reverse engineer brightness
            double factor = (double)currentGammaRamp[255] / (255.0 * 255.0);
            int remapped = (int)(factor * 100.0);
            monitor.softwareBrightness = 1 + (remapped - 49) * (100 - 1) / (100 - 49);
            if (monitor.softwareBrightness < 1) monitor.softwareBrightness = 1;
            if (monitor.softwareBrightness > 100) monitor.softwareBrightness = 100;
        }
    }

    // Hardware
    if (monitor.supportsHardwareBrightness) {
        DWORD minB, curB, maxB;
        if (GetMonitorBrightness(monitor.hPhysicalMonitor, &minB, &curB, &maxB)) {
            monitor.hardwareBrightness = curB;
        }
    }

    monitors->push_back(monitor);
    return TRUE;
}

bool InitBrightnessControl()
{
    if (g_initialized) return true;
    
    // Just refresh the list
    if (RefreshMonitorList()) {
        g_initialized = true;
        return true;
    }
    return false;
}

bool RefreshMonitorList()
{
    CleanupBrightnessControl(); // Close existing handles first

    g_monitors.clear();
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&g_monitors));
    return !g_monitors.empty();
}

void CleanupBrightnessControl()
{
    for (auto& monitor : g_monitors) {
        if (monitor.hdc) {
            DeleteDC(monitor.hdc);
            monitor.hdc = nullptr;
        }
        if (monitor.supportsHardwareBrightness && monitor.hPhysicalMonitor) {
            DestroyPhysicalMonitor(monitor.hPhysicalMonitor);
            monitor.hPhysicalMonitor = nullptr;
        }
    }
    g_monitors.clear();
    g_initialized = false;
}

const std::vector<Monitor>& GetMonitors()
{
    if (!g_initialized) {
        InitBrightnessControl();
    }
    return g_monitors;
}

bool SetSoftwareBrightness(int monitorIndex, int brightness)
{
    if (monitorIndex < 0 || static_cast<size_t>(monitorIndex) >= g_monitors.size()) return false;
    
    Monitor& monitor = g_monitors[monitorIndex];
    if (!monitor.hdc) return false;

    // Ensure brightness is within range
    if (brightness < 1) brightness = 1;
    if (brightness > 100) brightness = 100;

    monitor.softwareBrightness = brightness;

    // Remap brightness from 1-100 to 49-100 (to prevent screen from going too dark)
    double remapped_brightness = 49.0 + (brightness - 1.0) * (100.0 - 49.0) / (100.0 - 1.0);
    double factor = remapped_brightness / 100.0;

    WORD newGammaRamp[256 * 3];
    for (int i = 0; i < 256; i++)
    {
        int value = static_cast<int>(i * factor * 255);
        if (value > 65535) value = 65535;
        if (value < 0) value = 0;
        newGammaRamp[i] = static_cast<WORD>(value);
        newGammaRamp[i + 256] = static_cast<WORD>(value);
        newGammaRamp[i + 512] = static_cast<WORD>(value);
    }

    return SetDeviceGammaRamp(monitor.hdc, newGammaRamp);
}

bool SetHardwareBrightness(int monitorIndex, int brightness)
{
    if (monitorIndex < 0 || static_cast<size_t>(monitorIndex) >= g_monitors.size()) return false;
    
    Monitor& monitor = g_monitors[monitorIndex];
    if (!monitor.supportsHardwareBrightness) return false;

    if (brightness < 0) brightness = 0;
    if (brightness > 100) brightness = 100;
    
    monitor.hardwareBrightness = brightness;

    return SetMonitorBrightness(monitor.hPhysicalMonitor, brightness);
}

int GetSoftwareBrightness(int monitorIndex)
{
    if (monitorIndex < 0 || static_cast<size_t>(monitorIndex) >= g_monitors.size()) return 50;
    return g_monitors[monitorIndex].softwareBrightness;
}

int GetHardwareBrightness(int monitorIndex)
{
    if (monitorIndex < 0 || static_cast<size_t>(monitorIndex) >= g_monitors.size()) return 50;
    return g_monitors[monitorIndex].hardwareBrightness;
}
