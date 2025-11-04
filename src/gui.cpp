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
HWND g_hwnd_slider = nullptr; // Currently active slider (software or hardware)
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

// Global variables for settings window
HWND g_settings_hwnd = nullptr;
HWND g_hwnd_startup_checkbox = nullptr;
HWND g_hwnd_software_checkbox = nullptr;
HWND g_hwnd_hardware_checkbox = nullptr;

// Global variable to track if the window class is registered
static bool g_class_registered = false;

void ShowBrightnessSlider(HWND parent)
{
    // If window already exists, show it again
    if (g_hwnd_brightness && IsWindow(g_hwnd_brightness))
    {
        // If it's hidden, show it again
        ShowWindow(g_hwnd_brightness, SW_SHOW);
        SetForegroundWindow(g_hwnd_brightness);
        // Update the slider position to current setting
        int currentBrightness = g_settings.getBrightness();
        SendMessage(g_hwnd_slider, TBM_SETPOS, TRUE, currentBrightness);

        // Update the toggle to current mode
        if (g_hwnd_toggle)
        {
            SendMessage(g_hwnd_toggle, BM_SETCHECK, g_settings.isHardwareMode() ? BST_CHECKED : BST_UNCHECKED, 0);
        }

        // Update brightness label
        if (g_hwnd_current_brightness)
        {
            wchar_t buffer[50];
            swprintf_s(buffer, L"Brightness: %d%%", currentBrightness);
            SetWindowText(g_hwnd_current_brightness, buffer);
        }
        return;
    }

    // Register the window class for the brightness slider if not already registered
    if (!g_class_registered)
    {
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = BrightnessSliderProc;
        wc.hInstance = g_hInstance;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = L"CandelaBrightnessSlider";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

        if (!RegisterClassEx(&wc))
        {
            MessageBox(nullptr, L"Failed to register brightness slider window class", L"Error", MB_OK | MB_ICONERROR);
            return;
        }
        g_class_registered = true;
    }

    // Get cursor position for window placement
    POINT pt;
    GetCursorPos(&pt);

    // Adjust window size and position based on which sliders are shown
    int window_width = 0;
    if (g_settings.getShowSoftwareBrightness()) window_width += 65;
    if (g_settings.getShowHardwareBrightness()) window_width += 65;
    if (window_width == 0) window_width = 65; // Show at least one slider

    int x = pt.x - window_width / 2;
    int y = pt.y - 260;

    // Create the brightness slider window
    g_hwnd_brightness = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"CandelaBrightnessSlider",
        L"Brightness",
        WS_POPUP,
        x, y,     // Position
        window_width, 230, // Window size
        parent,
        nullptr,
        g_hInstance,
        nullptr);

    if (!g_hwnd_brightness)
    {
        MessageBox(nullptr, L"Failed to create brightness slider window", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    int current_x = 0;
    if (g_settings.getShowSoftwareBrightness())
    {
        // Create software brightness slider
        g_hwnd_software_slider = CreateWindowEx(
            0, TRACKBAR_CLASS, L"",
            WS_CHILD | WS_VISIBLE | TBS_VERT | TBS_AUTOTICKS | TBS_BOTH,
            current_x, 0, 65, 190,
            g_hwnd_brightness, (HMENU)101, g_hInstance, nullptr);

        g_hwnd_software_label = CreateWindowEx(
            0, WC_STATIC, L"Software",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            current_x, 190, 65, 20,
            g_hwnd_brightness, (HMENU)107, g_hInstance, nullptr);

        g_hwnd_software_value = CreateWindowEx(
            0, WC_STATIC, L"",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            current_x, 210, 65, 20,
            g_hwnd_brightness, (HMENU)109, g_hInstance, nullptr);

        int currentBrightness = g_settings.getBrightness();
        SendMessage(g_hwnd_software_slider, TBM_SETPOS, TRUE, currentBrightness);
        wchar_t buffer[20];
        swprintf_s(buffer, L"%d%%", currentBrightness);
        SetWindowText(g_hwnd_software_value, buffer);

        current_x += 65;
    }

    if (g_settings.getShowHardwareBrightness())
    {
        // Create hardware brightness slider
        g_hwnd_hardware_slider = CreateWindowEx(
            0, TRACKBAR_CLASS, L"",
            WS_CHILD | WS_VISIBLE | TBS_VERT | TBS_AUTOTICKS | TBS_BOTH,
            current_x, 0, 65, 190,
            g_hwnd_brightness, (HMENU)102, g_hInstance, nullptr);

        g_hwnd_hardware_label = CreateWindowEx(
            0, WC_STATIC, L"Hardware",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            current_x, 190, 65, 20,
            g_hwnd_brightness, (HMENU)108, g_hInstance, nullptr);

        g_hwnd_hardware_value = CreateWindowEx(
            0, WC_STATIC, L"",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            current_x, 210, 65, 20,
            g_hwnd_brightness, (HMENU)110, g_hInstance, nullptr);

        int currentBrightness = g_settings.getBrightness();
        SendMessage(g_hwnd_hardware_slider, TBM_SETPOS, TRUE, currentBrightness);
        wchar_t buffer[20];
        swprintf_s(buffer, L"%d%%", currentBrightness);
        SetWindowText(g_hwnd_hardware_value, buffer);
    }

    // Set slider range (1-100)
    if (g_hwnd_software_slider)
    {
        SendMessage(g_hwnd_software_slider, TBM_SETRANGE, TRUE, MAKELONG(1, 100));
        SendMessage(g_hwnd_software_slider, TBM_SETPOS, TRUE, g_settings.getBrightness());
        SendMessage(g_hwnd_software_slider, TBM_SETTICFREQ, 10, 0);
    }

    if (g_hwnd_hardware_slider)
    {
        SendMessage(g_hwnd_hardware_slider, TBM_SETRANGE, TRUE, MAKELONG(1, 100));
        SendMessage(g_hwnd_hardware_slider, TBM_SETPOS, TRUE, g_settings.getBrightness());
        SendMessage(g_hwnd_hardware_slider, TBM_SETTICFREQ, 10, 0);
    }

    // Show the window
    ShowWindow(g_hwnd_brightness, SW_SHOW);
    UpdateWindow(g_hwnd_brightness);
    SetForegroundWindow(g_hwnd_brightness);
}

LRESULT CALLBACK BrightnessSliderProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_HSCROLL:
    case WM_VSCROLL:
    {
        HWND trackbar = (HWND)lParam;

        int brightness = (int)SendMessage(trackbar, TBM_GETPOS, 0, 0);

        if (trackbar == g_hwnd_software_slider)
        {
            // Update software brightness value display
            wchar_t buffer[20];
            swprintf_s(buffer, L"%d%%", brightness);
            SetWindowText(g_hwnd_software_value, buffer);

            // Apply software brightness independently
            SetSoftwareBrightness(brightness);

            // Save the new brightness to settings if this is the current mode
            if (!g_settings.isHardwareMode())
            {
                g_settings.setBrightness(brightness);
                g_settings.save();
            }
        }
        else if (trackbar == g_hwnd_hardware_slider)
        {
            // Update hardware brightness value display
            wchar_t buffer[20];
            swprintf_s(buffer, L"%d%%", brightness);
            SetWindowText(g_hwnd_hardware_value, buffer);

            // Apply hardware brightness independently
            SetHardwareBrightness(brightness);

            // Save the new brightness to settings if this is the current mode
            if (g_settings.isHardwareMode())
            {
                g_settings.setBrightness(brightness);
                g_settings.save();
            }
        }
        break;
    }
    case WM_COMMAND:
    {
        // No command handling needed now that we've removed the toggle
        break;
    }
    case WM_DESTROY:
    {
        g_hwnd_brightness = nullptr;
        g_hwnd_slider = nullptr;
        g_hwnd_software_slider = nullptr;
        g_hwnd_hardware_slider = nullptr;
        g_hwnd_toggle = nullptr;
        g_hwnd_current_brightness = nullptr;
        g_hwnd_software_value = nullptr;
        g_hwnd_hardware_value = nullptr;
        g_hwnd_min_label = nullptr;
        g_hwnd_max_label = nullptr;
        g_hwnd_software_label = nullptr;
        g_hwnd_hardware_label = nullptr;
        break;
    }
    case WM_KEYDOWN:
    {
        if (wParam == VK_ESCAPE || wParam == VK_RETURN)
        {
            // Hide the window when ESC or Enter is pressed
            ShowWindow(hwnd, SW_HIDE);
        }
        break;
    }
    case WM_ACTIVATE:
    {
        // If the window is being deactivated, hide it.
        if (LOWORD(wParam) == WA_INACTIVE)
        {
            ShowWindow(hwnd, SW_HIDE);
        }
        break;
    }
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}



// Global variable to track if the settings window class is registered

static bool g_settings_class_registered = false;



void ShowSettingsDialog(HWND parent)

{

    if (g_settings_hwnd && IsWindow(g_settings_hwnd))

    {

        ShowWindow(g_settings_hwnd, SW_SHOW);

        SetForegroundWindow(g_settings_hwnd);

        return;

    }



    if (!g_settings_class_registered)

    {

        WNDCLASSEX wc = {};

        wc.cbSize = sizeof(WNDCLASSEX);

        wc.style = CS_HREDRAW | CS_VREDRAW;

        wc.lpfnWndProc = SettingsProc;

        wc.hInstance = g_hInstance;

        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

        wc.lpszClassName = L"CandelaSettings";

        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);



        if (!RegisterClassEx(&wc))

        {

            MessageBox(nullptr, L"Failed to register settings window class", L"Error", MB_OK | MB_ICONERROR);

            return;

        }

        g_settings_class_registered = true;

    }



    g_settings_hwnd = CreateWindowEx(

        0,

        L"CandelaSettings",

        L"Settings",

        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,

        CW_USEDEFAULT, CW_USEDEFAULT,

        250, 200,

        parent,

        nullptr,

        g_hInstance,

        nullptr);



    if (!g_settings_hwnd)

    {

        MessageBox(nullptr, L"Failed to create settings window", L"Error", MB_OK | MB_ICONERROR);

        return;

    }



    g_hwnd_startup_checkbox = CreateWindowEx(

        0, L"BUTTON", L"Start on boot",

        BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,

        10, 10, 200, 30,

        g_settings_hwnd, (HMENU)201, g_hInstance, nullptr);



    g_hwnd_software_checkbox = CreateWindowEx(

        0, L"BUTTON", L"Show software brightness",

        BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,

        10, 50, 200, 30,

        g_settings_hwnd, (HMENU)202, g_hInstance, nullptr);



    g_hwnd_hardware_checkbox = CreateWindowEx(

        0, L"BUTTON", L"Show hardware brightness",

        BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,

        10, 90, 200, 30,

        g_settings_hwnd, (HMENU)203, g_hInstance, nullptr);



    SendMessage(g_hwnd_startup_checkbox, BM_SETCHECK, g_settings.getStartOnBoot() ? BST_CHECKED : BST_UNCHECKED, 0);

    SendMessage(g_hwnd_software_checkbox, BM_SETCHECK, g_settings.getShowSoftwareBrightness() ? BST_CHECKED : BST_UNCHECKED, 0);

    SendMessage(g_hwnd_hardware_checkbox, BM_SETCHECK, g_settings.getShowHardwareBrightness() ? BST_CHECKED : BST_UNCHECKED, 0);



    ShowWindow(g_settings_hwnd, SW_SHOW);

    UpdateWindow(g_settings_hwnd);

}



LRESULT CALLBACK SettingsProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)

{

    switch (message)

    {

    case WM_COMMAND:

    {

        int controlId = LOWORD(wParam);

        if (HIWORD(wParam) == BN_CLICKED)

        {

            switch (controlId)

            {

            case 201: // Start on boot

                g_settings.setStartOnBoot(SendMessage(g_hwnd_startup_checkbox, BM_GETCHECK, 0, 0) == BST_CHECKED);

                g_settings.save();

                break;

            case 202: // Show software brightness

                g_settings.setShowSoftwareBrightness(SendMessage(g_hwnd_software_checkbox, BM_GETCHECK, 0, 0) == BST_CHECKED);

                g_settings.save();

                break;

            case 203: // Show hardware brightness

                g_settings.setShowHardwareBrightness(SendMessage(g_hwnd_hardware_checkbox, BM_GETCHECK, 0, 0) == BST_CHECKED);

                g_settings.save();

                break;

            }

        }

        break;

    }

    case WM_CLOSE:

    {

        ShowWindow(hwnd, SW_HIDE);

        break;

    }

    case WM_DESTROY:

    {

        g_settings_hwnd = nullptr;

        break;

    }

    default:

        return DefWindowProc(hwnd, message, wParam, lParam);

    }

    return 0;

}
