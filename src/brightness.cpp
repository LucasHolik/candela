#include "brightness.h"
#include <windows.h>
#include <vector>
#include <string>
#include <highlevelmonitorconfigurationapi.h>
#include <physicalmonitorenumerationapi.h>

#pragma comment(lib, "gdi32.lib")

// Global variables for gamma control
static HDC g_originalDC = nullptr;
static WORD g_originalGammaRamp[256 * 3] = {0};  // Store original gamma for restoration
static bool g_gammaInitialized = false;

// Structure for monitor enumeration
struct MonitorInfo {
    HDC hdc;
    std::wstring deviceName;
};

// Callback function for monitor enumeration
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    // Get monitor info
    MONITORINFOEX monitorInfoEx;
    monitorInfoEx.cbSize = sizeof(MONITORINFOEX);
    if (GetMonitorInfo(hMonitor, &monitorInfoEx)) {
        std::vector<MonitorInfo>* monitors = reinterpret_cast<std::vector<MonitorInfo>*>(dwData);
        MonitorInfo info;
        info.deviceName = monitorInfoEx.szDevice;
        info.hdc = hdcMonitor;
        monitors->push_back(info);
    }
    return TRUE;
}

bool InitBrightnessControl() {
    g_originalDC = GetDC(nullptr);
    if (!g_originalDC) {
        return false;
    }

    // Save the original gamma ramp
    if (!GetDeviceGammaRamp(g_originalDC, g_originalGammaRamp)) {
        ReleaseDC(nullptr, g_originalDC);
        return false;
    }

    g_gammaInitialized = true;
    return true;
}

void CleanupBrightnessControl() {
    if (g_gammaInitialized && g_originalDC) {
        // Restore original gamma ramp
        SetDeviceGammaRamp(g_originalDC, g_originalGammaRamp);
        ReleaseDC(nullptr, g_originalDC);
        g_gammaInitialized = false;
    }
}

bool SetSoftwareBrightness(int brightness) {
    if (!g_gammaInitialized) {
        if (!InitBrightnessControl()) {
            return false;
        }
    }

    // Ensure brightness is within range
    if (brightness < 1 || brightness > 100) {
        return false;
    }

    // Calculate brightness factor
    double factor = brightness / 100.0;

    // Create new gamma ramp
    WORD newGammaRamp[256 * 3]; // RGB gamma ramp

    for (int i = 0; i < 256; i++) {
        // Apply brightness factor to each channel
        int value = static_cast<int>(i * factor * 255);
        if (value > 65535) value = 65535;  // Clamp to max value (16-bit)
        if (value < 0) value = 0;           // Clamp to min value

        // Set the red, green, and blue channels the same for grayscale effect
        newGammaRamp[i] = static_cast<WORD>(value);
        newGammaRamp[i + 256] = static_cast<WORD>(value);
        newGammaRamp[i + 512] = static_cast<WORD>(value);
    }

    // Enumerate all monitors to apply gamma changes
    std::vector<MonitorInfo> monitors;
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));

    bool success = true;
    for (const auto& monitor : monitors) {
        // We'll use device name to get the specific monitor DC
        HDC monitorDC = CreateDC(monitor.deviceName.c_str(), nullptr, nullptr, nullptr);
        if (monitorDC) {
            if (!SetDeviceGammaRamp(monitorDC, newGammaRamp)) {
                success = false;
            }
            DeleteDC(monitorDC);
        }
    }

    return success;
}

bool SetHardwareBrightness(int brightness) {
    // Hardware brightness control via DDC/CI protocol
    // This uses Windows API for monitor configuration
    
    // Ensure brightness is within range
    if (brightness < 0 || brightness > 100) {
        return false;
    }

    // Set brightness for all connected monitors
    return SetHardwareBrightnessForAllMonitors(brightness);
}

// Structure to pass data to monitor enumeration callback
struct BrightnessData {
    int brightness;
};

// Callback function for monitor enumeration
BOOL CALLBACK MonitorEnumCallback(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    DWORD monitorCount = 0;
    PHYSICAL_MONITOR* physicalMonitors = nullptr;
    
    // Get number of physical monitors for this display monitor
    if (GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &monitorCount) && monitorCount > 0) {
        physicalMonitors = new PHYSICAL_MONITOR[monitorCount];
        
        if (GetPhysicalMonitorsFromHMONITOR(hMonitor, monitorCount, physicalMonitors)) {
            int brightness = reinterpret_cast<BrightnessData*>(dwData)->brightness;
            
            // Set brightness for each physical monitor
            for (DWORD i = 0; i < monitorCount; i++) {
                SetMonitorBrightness(physicalMonitors[i].hPhysicalMonitor, brightness);
            }
            
            // Clean up
            for (DWORD i = 0; i < monitorCount; i++) {
                DestroyPhysicalMonitor(physicalMonitors[i].hPhysicalMonitor);
            }
        }
        
        delete[] physicalMonitors;
    }
    
    return TRUE; // Continue enumeration
}

// Additional helper function to handle multiple monitors
bool SetHardwareBrightnessForAllMonitors(int brightness) {
    // Ensure brightness is within range
    if (brightness < 0 || brightness > 100) {
        return false;
    }

    BrightnessData data = {brightness};
    
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumCallback, reinterpret_cast<LPARAM>(&data));
    
    return true;
}

int GetSoftwareBrightness() {
    // For now, just return a default value
    // In a real implementation, this would read the current gamma ramp values
    return 50; // Placeholder
}

int GetHardwareBrightness() {
    // For now, just return a default value
    // In a real implementation, this would query the monitor for current brightness
    return 50; // Placeholder
}