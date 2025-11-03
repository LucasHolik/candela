#include "gui.h"
#include "brightness.h"
#include "settings.h"
#include <commctrl.h>
#include <windowsx.h>

// Global external references
extern Settings g_settings;
extern HINSTANCE g_hInstance;

// Global variables for brightness slider
HWND g_slider_hwnd = nullptr;
HWND g_hwnd_brightness = nullptr;
HWND g_hwnd_slider = nullptr;  // Currently active slider (software or hardware)
HWND g_hwnd_software_slider = nullptr;
HWND g_hwnd_hardware_slider = nullptr;
HWND g_hwnd_toggle = nullptr;
HWND g_hwnd_current_brightness = nullptr;
HWND g_hwnd_software_value = nullptr;
HWND g_hwnd_hardware_value = nullptr;

// Additional global variables for slider labels
HWND g_hwnd_min_label = nullptr;
HWND g_hwnd_max_label = nullptr;
HWND g_hwnd_software_label = nullptr;
HWND g_hwnd_hardware_label = nullptr;

// Global variable to track if the window class is registered
static bool g_class_registered = false;

void ShowBrightnessSlider(HWND parent) {
    // If window already exists, show it again
    if (g_hwnd_brightness && IsWindow(g_hwnd_brightness)) {
        // If it's hidden, show it again
        ShowWindow(g_hwnd_brightness, SW_SHOW);
        SetForegroundWindow(g_hwnd_brightness);
        // Update the slider position to current setting
        int currentBrightness = g_settings.getBrightness();
        SendMessage(g_hwnd_slider, TBM_SETPOS, TRUE, currentBrightness);
        
        // Update the toggle to current mode
        if (g_hwnd_toggle) {
            SendMessage(g_hwnd_toggle, BM_SETCHECK, g_settings.isHardwareMode() ? BST_CHECKED : BST_UNCHECKED, 0);
        }
        
        // Update brightness label
        if (g_hwnd_current_brightness) {
            wchar_t buffer[50];
            swprintf_s(buffer, L"Brightness: %d%%", currentBrightness);
            SetWindowText(g_hwnd_current_brightness, buffer);
        }
        return;
    }

    // Register the window class for the brightness slider if not already registered
    if (!g_class_registered) {
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = BrightnessSliderProc;
        wc.hInstance = g_hInstance;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = L"CandelaBrightnessSlider";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

        if (!RegisterClassEx(&wc)) {
            MessageBox(nullptr, L"Failed to register brightness slider window class", L"Error", MB_OK | MB_ICONERROR);
            return;
        }
        g_class_registered = true;
    }

    // Get cursor position for window placement
    POINT pt;
    GetCursorPos(&pt);

    // Position window so the bottom aligns with the tray icon
    // Adjust position to place it above the cursor, near the system tray
    int x = pt.x - 85;  // Reduced width for smaller window (170/2)
    int y = pt.y - 280; // Position it above the cursor, with more space for controls

    // Create the brightness slider window
    g_hwnd_brightness = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        L"CandelaBrightnessSlider",
        L"Brightness",
        WS_POPUP | WS_THICKFRAME,
        x, y,               // Position
        170, 280,           // Smaller window
        parent,
        nullptr,
        g_hInstance,
        nullptr
    );

    if (!g_hwnd_brightness) {
        MessageBox(nullptr, L"Failed to create brightness slider window", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Create software brightness slider (left side)
    g_hwnd_software_slider = CreateWindowEx(
        0, TRACKBAR_CLASS, L"",
        WS_CHILD | WS_VISIBLE | TBS_VERT | TBS_AUTOTICKS | TBS_BOTH,
        25, 25, 63, 190,  // Wider slider (63px, about 5% wider than previous)
        g_hwnd_brightness, (HMENU)101, g_hInstance, nullptr
    );
    
    // Create hardware brightness slider (right side)
    g_hwnd_hardware_slider = CreateWindowEx(
        0, TRACKBAR_CLASS, L"",
        WS_CHILD | WS_VISIBLE | TBS_VERT | TBS_AUTOTICKS | TBS_BOTH,
        95, 25, 63, 190,  // Wider slider (63px, about 5% wider than previous), adjusted position
        g_hwnd_brightness, (HMENU)102, g_hInstance, nullptr
    );
    
    // Create labels for slider titles
    g_hwnd_software_label = CreateWindowEx(
        0, WC_STATIC, L"Software",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        25, 215, 63, 20,  // Match slider width
        g_hwnd_brightness, (HMENU)107, g_hInstance, nullptr
    );
    
    g_hwnd_hardware_label = CreateWindowEx(
        0, WC_STATIC, L"Hardware",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        95, 215, 63, 20,  // Match slider width
        g_hwnd_brightness, (HMENU)108, g_hInstance, nullptr
    );
    
    // Create current value displays for both sliders
    g_hwnd_software_value = CreateWindowEx(
        0, WC_STATIC, L"50%",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        25, 235, 63, 20,  // Match slider width
        g_hwnd_brightness, (HMENU)109, g_hInstance, nullptr
    );
    
    g_hwnd_hardware_value = CreateWindowEx(
        0, WC_STATIC, L"50%",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        95, 235, 63, 20,  // Match slider width
        g_hwnd_brightness, (HMENU)110, g_hInstance, nullptr
    );
    
    // Initialize slider values with current settings
    int currentBrightness = g_settings.getBrightness();
    SendMessage(g_hwnd_software_slider, TBM_SETPOS, TRUE, currentBrightness);
    SendMessage(g_hwnd_hardware_slider, TBM_SETPOS, TRUE, currentBrightness);
    
    // Update current value displays
    wchar_t buffer[20];
    swprintf_s(buffer, L"%d%%", currentBrightness);
    SetWindowText(g_hwnd_software_value, buffer);
    SetWindowText(g_hwnd_hardware_value, buffer);
    


    // Set slider range (1-100)
    if (g_hwnd_slider) {
        SendMessage(g_hwnd_slider, TBM_SETRANGE, TRUE, MAKELONG(1, 100));
        SendMessage(g_hwnd_slider, TBM_SETPOS, TRUE, g_settings.getBrightness());
        SendMessage(g_hwnd_slider, TBM_SETTICFREQ, 10, 0);
    }

    // No toggle checkbox - each slider now controls its respective brightness independently

    // Show the window
    ShowWindow(g_hwnd_brightness, SW_SHOW);
    UpdateWindow(g_hwnd_brightness);
}

LRESULT CALLBACK BrightnessSliderProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_HSCROLL:
        case WM_VSCROLL: {
            HWND trackbar = (HWND)lParam;
            
            int brightness = (int)SendMessage(trackbar, TBM_GETPOS, 0, 0);
            
            if (trackbar == g_hwnd_software_slider) {
                // Update software brightness value display
                wchar_t buffer[20];
                swprintf_s(buffer, L"%d%%", brightness);
                SetWindowText(g_hwnd_software_value, buffer);
                
                // Apply software brightness independently
                SetSoftwareBrightness(brightness);
                
                // Save the new brightness to settings if this is the current mode
                if (!g_settings.isHardwareMode()) {
                    g_settings.setBrightness(brightness);
                    g_settings.save();
                }
            } 
            else if (trackbar == g_hwnd_hardware_slider) {
                // Update hardware brightness value display
                wchar_t buffer[20];
                swprintf_s(buffer, L"%d%%", brightness);
                SetWindowText(g_hwnd_hardware_value, buffer);
                
                // Apply hardware brightness independently
                SetHardwareBrightness(brightness);
                
                // Save the new brightness to settings if this is the current mode
                if (g_settings.isHardwareMode()) {
                    g_settings.setBrightness(brightness);
                    g_settings.save();
                }
            }
            break;
        }
        case WM_COMMAND: {
            // No command handling needed now that we've removed the toggle
            break;
        }
        case WM_DESTROY: {
            g_hwnd_brightness = nullptr;
            g_hwnd_slider = nullptr;
            g_hwnd_software_slider = nullptr;
            g_hwnd_hardware_slider = nullptr;
            g_hwnd_toggle = nullptr;
            g_hwnd_current_brightness = nullptr;
            g_hwnd_software_value = nullptr;
            g_hwnd_hardware_value = nullptr;
            g_hwnd_min_label = nullptr;  // Still need to clear these even though they're not used
            g_hwnd_max_label = nullptr;  // Still need to clear these even though they're not used
            g_hwnd_software_label = nullptr;
            g_hwnd_hardware_label = nullptr;
            break;
        }
        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                // Hide the window when ESC is pressed
                ShowWindow(hwnd, SW_HIDE);
            } else if (wParam == VK_RETURN) {
                // Hide the window when Enter is pressed
                ShowWindow(hwnd, SW_HIDE);
            }
            break;
        }
        case WM_KILLFOCUS: {
            // Don't hide the window when it loses focus - this keeps it from disappearing when interacting with controls
            // ShowWindow(hwnd, SW_HIDE);
            break;
        }
        case WM_ACTIVATE: {
            // Handle activation/deactivation
            if (LOWORD(wParam) == WA_INACTIVE) {
                // Don't hide on deactivation either, as this happens when clicking other controls
                // ShowWindow(hwnd, SW_HIDE);
            }
            break;
        }
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

void ShowSettingsDialog(HWND parent, Settings& settings) {
    // Create a modeless dialog for settings
    HWND hDlg = CreateDialog(
        g_hInstance,
        MAKEINTRESOURCE(100),  // This would be defined in a resource file
        parent,
        SettingsProc
    );

    if (hDlg) {
        ShowWindow(hDlg, SW_SHOW);
    }
}

LRESULT CALLBACK SettingsProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            // Initialize dialog controls with current settings
            HWND startupCheckbox = GetDlgItem(hwnd, 201); // ID for startup checkbox
            HWND defaultModeCombo = GetDlgItem(hwnd, 202); // ID for mode combo box

            if (startupCheckbox) {
                SendMessage(startupCheckbox, BM_SETCHECK, 
                    g_settings.getStartOnBoot() ? BST_CHECKED : BST_UNCHECKED, 0);
            }

            if (defaultModeCombo) {
                SendMessage(defaultModeCombo, CB_ADDSTRING, 0, (LPARAM)L"Hardware");
                SendMessage(defaultModeCombo, CB_ADDSTRING, 0, (LPARAM)L"Software");
                SendMessage(defaultModeCombo, CB_SETCURSEL, 
                    g_settings.getDefaultMode() ? 0 : 1, 0); // 0 for hardware, 1 for software
            }

            return TRUE;
        }
        case WM_COMMAND: {
            int controlId = LOWORD(wParam);
            int eventCode = HIWORD(wParam);

            switch (controlId) {
                case 201: // Startup checkbox
                    if (eventCode == BN_CLICKED) {
                        g_settings.setStartOnBoot(
                            (SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        );
                    }
                    break;
                case 202: // Default mode combo box
                    if (eventCode == CBN_SELCHANGE) {
                        int selection = (int)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                        g_settings.setDefaultMode(selection == 0); // 0 for hardware, 1 for software
                    }
                    break;
                case IDOK: // OK button
                    g_settings.save();
                    EndDialog(hwnd, IDOK);
                    break;
                case IDCANCEL: // Cancel button
                    EndDialog(hwnd, IDCANCEL);
                    break;
            }
            break;
        }
        case WM_CLOSE: {
            EndDialog(hwnd, 0);
            break;
        }
    }
    return 0;
}