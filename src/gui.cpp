#include "gui.h"
#include "brightness.h"
#include "settings.h"
#include "resource.h"
#include <commctrl.h>
#include <windowsx.h>
#include <vector>
#include <string>

// Global external references
extern Settings g_settings;
extern HINSTANCE g_hInstance;

// Global variables for brightness slider
HWND g_hwnd_brightness = nullptr;

// These are kept for compatibility but might not be used individually anymore
HWND g_slider_hwnd = nullptr;
HWND g_hwnd_slider = nullptr; 
HWND g_hwnd_software_slider = nullptr;
HWND g_hwnd_hardware_slider = nullptr;
HWND g_hwnd_toggle = nullptr;
HWND g_hwnd_current_brightness = nullptr;
HWND g_hwnd_software_value = nullptr;
HWND g_hwnd_hardware_value = nullptr;
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
static bool g_info_class_registered = false;
static bool g_settings_class_registered = false;

// Global variable for info window
HWND g_info_hwnd = nullptr;

// ID Constants for Dynamic Controls
const int ID_BASE = 2000;
const int ID_STRIDE = 100;
const int OFFSET_SW_SLIDER = 1;
const int OFFSET_HW_SLIDER = 2;
const int OFFSET_SW_LABEL = 3;
const int OFFSET_HW_LABEL = 4;
const int OFFSET_SW_VALUE = 5;
const int OFFSET_HW_VALUE = 6;
const int OFFSET_MONITOR_LABEL = 7;

void ShowBrightnessSlider(HWND parent)
{
    // Always refresh monitor list to handle hotplugging
    RefreshMonitorList();
    const auto& monitors = GetMonitors();
    int monitorCount = (int)monitors.size();

    if (monitorCount == 0) return;

    bool show_software = g_settings.getShowSoftwareBrightness();
    bool show_hardware = g_settings.getShowHardwareBrightness();

    // If window exists, destroy it to recreate with new layout
    if (g_hwnd_brightness && IsWindow(g_hwnd_brightness))
    {
        DestroyWindow(g_hwnd_brightness);
        g_hwnd_brightness = nullptr;
    }

    // Register class if needed
    if (!g_class_registered)
    {
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = BrightnessSliderProc;
        wc.hInstance = g_hInstance;
        wc.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON1));
        wc.hIconSm = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON1));
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

    // Calculate dimensions
    int sliderGroupWidth = 0;
    if (show_software && show_hardware) sliderGroupWidth = 130;
    else if (show_software || show_hardware) sliderGroupWidth = 65;
    else sliderGroupWidth = 0; // Should handle this case gracefully

    int padding = 10;
    int totalWidth = (sliderGroupWidth * monitorCount) + (padding * (monitorCount + 1));
    int totalHeight = 260; 

    POINT pt;
    GetCursorPos(&pt);
    int x = pt.x - totalWidth / 2;
    int y = pt.y - totalHeight - 20;

    // Create Main Window
    g_hwnd_brightness = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"CandelaBrightnessSlider",
        L"Brightness",
        WS_POPUP,
        x, y, totalWidth, totalHeight,
        parent, nullptr, g_hInstance, nullptr);

    // Create Controls for each monitor
    for (int i = 0; i < monitorCount; i++)
    {
        int groupX = padding + (i * (sliderGroupWidth + padding));
        int baseY = 30; // Leave space for monitor label

        int baseID = ID_BASE + (i * ID_STRIDE);

        // Monitor Label
        wchar_t monitorLabel[64];
        swprintf_s(monitorLabel, L"Display %d", i + 1);
        CreateWindowEx(
            0, WC_STATIC, monitorLabel,
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            groupX, 5, sliderGroupWidth, 20,
            g_hwnd_brightness, (HMENU)(intptr_t)(baseID + OFFSET_MONITOR_LABEL), g_hInstance, nullptr);

        int currentX = groupX;

        // Software Slider
        if (show_software)
        {
            HWND hSlider = CreateWindowEx(
                0, TRACKBAR_CLASS, L"",
                WS_CHILD | WS_VISIBLE | TBS_VERT | TBS_AUTOTICKS | TBS_BOTH,
                currentX, baseY, 65, 190,
                g_hwnd_brightness, (HMENU)(intptr_t)(baseID + OFFSET_SW_SLIDER), g_hInstance, nullptr);

            CreateWindowEx(
                0, WC_STATIC, L"Software",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                currentX, baseY + 190, 65, 20,
                g_hwnd_brightness, (HMENU)(intptr_t)(baseID + OFFSET_SW_LABEL), g_hInstance, nullptr);

            HWND hValue = CreateWindowEx(
                0, WC_STATIC, L"",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                currentX, baseY + 210, 65, 20,
                g_hwnd_brightness, (HMENU)(intptr_t)(baseID + OFFSET_SW_VALUE), g_hInstance, nullptr);

            int val = monitors[i].softwareBrightness;
            SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELONG(1, 100));
            SendMessage(hSlider, TBM_SETPOS, TRUE, 101 - val);
            SendMessage(hSlider, TBM_SETTICFREQ, 10, 0);

            wchar_t buffer[20];
            swprintf_s(buffer, L"%d%%", val);
            SetWindowText(hValue, buffer);

            currentX += 65;
        }

        // Hardware Slider
        if (show_hardware)
        {
            HWND hSlider = CreateWindowEx(
                0, TRACKBAR_CLASS, L"",
                WS_CHILD | WS_VISIBLE | TBS_VERT | TBS_AUTOTICKS | TBS_BOTH,
                currentX, baseY, 65, 190,
                g_hwnd_brightness, (HMENU)(intptr_t)(baseID + OFFSET_HW_SLIDER), g_hInstance, nullptr);

            CreateWindowEx(
                0, WC_STATIC, L"Hardware",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                currentX, baseY + 190, 65, 20,
                g_hwnd_brightness, (HMENU)(intptr_t)(baseID + OFFSET_HW_LABEL), g_hInstance, nullptr);

            HWND hValue = CreateWindowEx(
                0, WC_STATIC, L"",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                currentX, baseY + 210, 65, 20,
                g_hwnd_brightness, (HMENU)(intptr_t)(baseID + OFFSET_HW_VALUE), g_hInstance, nullptr);

            int val = monitors[i].hardwareBrightness;
            SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELONG(1, 100));
            SendMessage(hSlider, TBM_SETPOS, TRUE, 101 - val);
            SendMessage(hSlider, TBM_SETTICFREQ, 10, 0);

            wchar_t buffer[20];
            swprintf_s(buffer, L"%d%%", val);
            SetWindowText(hValue, buffer);
            
            // Disable if not supported? 
            if (!monitors[i].supportsHardwareBrightness) {
                EnableWindow(hSlider, FALSE);
                SetWindowText(hValue, L"N/A");
            }
        }
    }

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
        int controlID = GetDlgCtrlID(trackbar);
        int relativeID = controlID - ID_BASE;

        if (relativeID >= 0)
        {
            int monitorIndex = relativeID / ID_STRIDE;
            int type = relativeID % ID_STRIDE;
            int brightness = 101 - (int)SendMessage(trackbar, TBM_GETPOS, 0, 0);

            if (type == OFFSET_SW_SLIDER)
            {
                SetSoftwareBrightness(monitorIndex, brightness);
                // Update Value Label
                int valueID = ID_BASE + (monitorIndex * ID_STRIDE) + OFFSET_SW_VALUE;
                HWND hValue = GetDlgItem(hwnd, valueID);
                wchar_t buffer[20];
                swprintf_s(buffer, L"%d%%", brightness);
                SetWindowText(hValue, buffer);
            }
            else if (type == OFFSET_HW_SLIDER)
            {
                SetHardwareBrightness(monitorIndex, brightness);
                // Update Value Label
                int valueID = ID_BASE + (monitorIndex * ID_STRIDE) + OFFSET_HW_VALUE;
                HWND hValue = GetDlgItem(hwnd, valueID);
                wchar_t buffer[20];
                swprintf_s(buffer, L"%d%%", brightness);
                SetWindowText(hValue, buffer);
            }
        }
        break;
    }
    case WM_DESTROY:
    {
        g_hwnd_brightness = nullptr;
        break;
    }
    case WM_KEYDOWN:
    {
        if (wParam == VK_ESCAPE || wParam == VK_RETURN)
        {
            ShowWindow(hwnd, SW_HIDE);
        }
        break;
    }
    case WM_ACTIVATE:
    {
        if (LOWORD(wParam) == WA_INACTIVE)
        {
            ShowWindow(hwnd, SW_HIDE);
        }
        break;
    }
    case WM_SETICON:
    {
        return TRUE;
    }
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

// ... Keep SettingsProc and InfoProc as they are ...
// Re-implementing them here to ensure the file is complete since I'm overwriting

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
      {
        bool is_checked = SendMessage(g_hwnd_software_checkbox, BM_GETCHECK, 0, 0) == BST_CHECKED;
        if (!is_checked && !g_settings.getShowHardwareBrightness())
        {
            MessageBox(hwnd, L"You must have at least one brightness slider enabled.", L"Error", MB_OK | MB_ICONERROR);
            SendMessage(g_hwnd_software_checkbox, BM_SETCHECK, BST_CHECKED, 0);
        }
        else
        {
            g_settings.setShowSoftwareBrightness(is_checked);
            g_settings.save();
        }
        break;
      }
      case 203: // Show hardware brightness
      {
        bool is_checked = SendMessage(g_hwnd_hardware_checkbox, BM_GETCHECK, 0, 0) == BST_CHECKED;
        if (!is_checked && !g_settings.getShowSoftwareBrightness())
        {
            MessageBox(hwnd, L"You must have at least one brightness slider enabled.", L"Error", MB_OK | MB_ICONERROR);
            SendMessage(g_hwnd_hardware_checkbox, BM_SETCHECK, BST_CHECKED, 0);
        }
        else
        {
            g_settings.setShowHardwareBrightness(is_checked);
            g_settings.save();
        }
        break;
      }
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
  case WM_SETICON:
    return TRUE;
  default:
    return DefWindowProc(hwnd, message, wParam, lParam);
  }
  return 0;
}

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
    wc.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wc.hIconSm = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON1));
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

  SendMessage(g_settings_hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON1)));
  SendMessage(g_settings_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON1)));

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

void ShowInfoDialog(HWND parent)
{
  if (g_info_hwnd && IsWindow(g_info_hwnd))
  {
    ShowWindow(g_info_hwnd, SW_SHOW);
    SetForegroundWindow(g_info_hwnd);
    return;
  }

  if (!g_info_class_registered)
  {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = InfoProc;
    wc.hInstance = g_hInstance;
    wc.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wc.hIconSm = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"CandelaInfo";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClassEx(&wc))
    {
      MessageBox(nullptr, L"Failed to register info window class", L"Error", MB_OK | MB_ICONERROR);
      return;
    }
    g_info_class_registered = true;
  }

  g_info_hwnd = CreateWindowEx(
      0,
      L"CandelaInfo",
      L"Information",
      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
      CW_USEDEFAULT, CW_USEDEFAULT,
      400, 320,
      parent,
      nullptr,
      g_hInstance,
      nullptr);

  if (!g_info_hwnd)
  {
    MessageBox(nullptr, L"Failed to create info window", L"Error", MB_OK | MB_ICONERROR);
    return;
  }

  SendMessage(g_info_hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON1)));
  SendMessage(g_info_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON1)));

  CreateWindowEx(
      0, WC_STATIC, L"",
      WS_CHILD | WS_VISIBLE | SS_LEFT,
      10, 10, 365, 240,
      g_info_hwnd, (HMENU)301, g_hInstance, nullptr);

  HWND infoText = GetDlgItem(g_info_hwnd, 301);
  SetWindowText(infoText,
                L"Application Information\n\n"
                L"Start on Boot:\n"
                L"  Enables the application to automatically start when Windows boots.\n\n"
                L"Software Brightness (Gamma Control):\n"
                L"  Adjusts brightness by modifying the gamma ramp of your display.\n"
                L"  This works on all monitors but affects the color profile.\n\n"
                L"Hardware Brightness (DDC/CI):\n"
                L"  Controls your monitor's hardware brightness using DDC/CI protocol.\n"
                L"  Provides true brightness control but requires monitor support.\n"
                L"  Note: This feature may not work on all monitors.\n\n"
                L"Note: Both software and hardware brightness can be controlled\n"
                L"simultaneously if that's what the user wants.");

  ShowWindow(g_info_hwnd, SW_SHOW);
  UpdateWindow(g_info_hwnd);
}

LRESULT CALLBACK InfoProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_COMMAND:
    break;
  case WM_CLOSE:
    ShowWindow(hwnd, SW_HIDE);
    break;
  case WM_DESTROY:
    g_info_hwnd = nullptr;
    break;
  case WM_SETICON:
    return TRUE;
  default:
    return DefWindowProc(hwnd, message, wParam, lParam);
  }
  return 0;
}