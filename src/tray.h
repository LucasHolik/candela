#pragma once
#include <windows.h>

// Forward declarations for functions used by tray
void ShowBrightnessSlider(HWND parent);
void ShowSettingsDialog(HWND parent, class Settings &settings);

class Tray
{
public:
    static bool createTray(HWND hwnd);
    static void removeTray(HWND hwnd);
    static void handleLeftClick(HWND hwnd);
    static void handleRightClick(HWND hwnd, int x, int y);
    static const UINT WM_TRAYICON = WM_APP + 1;

private:
    static const UINT ID_EXIT = 1;
    static const UINT ID_SETTINGS = 2;
    static const UINT ID_BRIGHTNESS_SLIDER = 3;
};