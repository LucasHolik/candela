#pragma once
#include <windows.h>
#include <vector>
#include <string>

struct Monitor {
    HMONITOR hMonitor;
    HDC hdc; // For software brightness
    std::wstring deviceName;
    int softwareBrightness;
    int hardwareBrightness;
    HANDLE hPhysicalMonitor; // For hardware brightness (DDC/CI)
    bool supportsHardwareBrightness;
    
    Monitor() : hMonitor(nullptr), hdc(nullptr), softwareBrightness(100), hardwareBrightness(50), hPhysicalMonitor(nullptr), supportsHardwareBrightness(false) {}
};

// Function to refresh the list of monitors
bool RefreshMonitorList();

// Get the list of monitors
const std::vector<Monitor>& GetMonitors();

// Function to set hardware brightness (via DDC/CI) for a specific monitor
bool SetHardwareBrightness(int monitorIndex, int brightness);

// Function to set software brightness (via gamma) for a specific monitor
bool SetSoftwareBrightness(int monitorIndex, int brightness);

// Function to get current hardware brightness for a specific monitor
int GetHardwareBrightness(int monitorIndex);

// Function to get current software brightness for a specific monitor
int GetSoftwareBrightness(int monitorIndex);

// Initialize brightness control system
bool InitBrightnessControl();

// Cleanup brightness control system
void CleanupBrightnessControl();