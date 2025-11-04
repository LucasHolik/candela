#include "tray.h"
#include "gui.h"
#include "settings.h"
#include <shellapi.h>
#include <windowsx.h>

// Global external references
extern HWND g_hwnd;
extern Settings g_settings;

bool Tray::createTray(HWND hwnd) {
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    
    // Load icon - for now using default, but we'll add a custom one later
    nid.hIcon = LoadIcon(nullptr, IDI_INFORMATION);
    
    wcscpy_s(nid.szTip, L"Candela Brightness Control");

    bool result = Shell_NotifyIcon(NIM_ADD, &nid);
    if (!result) {
        return false;
    }

    return true;
}

void Tray::removeTray(HWND hwnd) {
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

void Tray::handleLeftClick(HWND hwnd) {
    // Show the brightness slider
    ShowBrightnessSlider(hwnd);
}

void Tray::handleRightClick(HWND hwnd, int x, int y) {
    HMENU hMenu = CreatePopupMenu();
    
    if (hMenu) {
        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_BRIGHTNESS_SLIDER, L"Brightness Slider...");
        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_SETTINGS, L"Settings");
        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_EXIT, L"Exit");

        // Set menu item states based on current settings
        // For example, check marks for current mode

        SetForegroundWindow(hwnd);
        int cmd = TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN | TPM_RETURNCMD, x, y, 0, hwnd, nullptr);
        
        // Process the selected command
        if (cmd == ID_BRIGHTNESS_SLIDER) {
            // Show brightness slider
            ShowBrightnessSlider(hwnd);
        }
        else if (cmd == ID_SETTINGS) {
            // Show settings dialog
            ShowSettingsDialog(hwnd);
        }
        else if (cmd == ID_EXIT) {
            // Exit the application
            PostMessage(hwnd, WM_CLOSE, 0, 0);
        }
        
        DestroyMenu(hMenu);
    }
}