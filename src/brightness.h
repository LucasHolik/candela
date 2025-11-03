#pragma once
#include <windows.h>

// Function to set hardware brightness (via DDC/CI)
bool SetHardwareBrightness(int brightness);

// Function to set hardware brightness for all monitors
bool SetHardwareBrightnessForAllMonitors(int brightness);

// Function to set software brightness (via gamma)
bool SetSoftwareBrightness(int brightness);

// Function to get current hardware brightness
int GetHardwareBrightness();

// Function to get current software brightness
int GetSoftwareBrightness();

// Initialize brightness control system
bool InitBrightnessControl();

// Cleanup brightness control system
void CleanupBrightnessControl();