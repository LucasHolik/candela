#pragma once
#include <windows.h>

// Forward declaration
class Settings;

// Global variables for brightness slider
extern HWND g_slider_hwnd;
extern HWND g_hwnd_brightness;
extern HWND g_hwnd_slider;  // Currently active slider (software or hardware)
extern HWND g_hwnd_software_slider;
extern HWND g_hwnd_hardware_slider;
extern HWND g_hwnd_toggle;
extern HWND g_hwnd_current_brightness;
extern HWND g_hwnd_software_value;
extern HWND g_hwnd_hardware_value;
extern HWND g_hwnd_min_label;
extern HWND g_hwnd_max_label;
extern HWND g_hwnd_software_label;
extern HWND g_hwnd_hardware_label;

// Function to show the brightness slider dialog
void ShowBrightnessSlider(HWND parent);

// Function to show the settings dialog
void ShowSettingsDialog(HWND parent, Settings& settings);

// Window procedure for the brightness slider dialog
LRESULT CALLBACK BrightnessSliderProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

// Window procedure for the settings dialog
LRESULT CALLBACK SettingsProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);