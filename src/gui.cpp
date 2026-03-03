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

// -----------------------------------------------------------------------------------------------
// Constants & IDs
// -----------------------------------------------------------------------------------------------

namespace GuiConstants
{
  // ID Constants for Brightness Slider
  const int ID_SLIDER_BASE = 2000;
  const int ID_SLIDER_STRIDE = 100;
  const int OFFSET_SW_SLIDER = 1;
  const int OFFSET_HW_SLIDER = 2;
  const int OFFSET_SW_LABEL = 3;
  const int OFFSET_HW_LABEL = 4;
  const int OFFSET_SW_VALUE = 5;
  const int OFFSET_HW_VALUE = 6;
  const int OFFSET_MONITOR_LABEL = 7;

  // ID Constants for Settings Window
  const int ID_SETTINGS_STARTUP = 201;
  const int ID_SETTINGS_MONITOR_BASE = 3000;
  const int ID_SETTINGS_STRIDE = 10;
  const int OFFSET_SETTINGS_SW_CHECK = 1;
  const int OFFSET_SETTINGS_HW_CHECK = 2;
  const int ID_INFO_TEXT = 301;

  // Layout Constants
  const int SLIDER_GROUP_WIDTH = 65;
  const int SLIDER_HEIGHT = 190;
  const int PADDING = 10;
  const int WINDOW_BASE_HEIGHT = 260;

  // Slider Range
  const int SLIDER_MIN = 1;    // Software brightness minimum (gamma cannot safely reach 0)
  const int HW_SLIDER_MIN = 0; // Hardware brightness minimum (DDC/CI supports true 0)
  const int SLIDER_MAX = 100;
}

// -----------------------------------------------------------------------------------------------
// Global Window Handles (Internal Module State)
// -----------------------------------------------------------------------------------------------

static HWND g_hwnd_brightness = nullptr;
static HWND g_settings_hwnd = nullptr;
static HWND g_info_hwnd = nullptr;
static HWND g_hwnd_startup_checkbox = nullptr;

static bool g_class_registered = false;
static bool g_info_class_registered = false;
static bool g_settings_class_registered = false;

// -----------------------------------------------------------------------------------------------
// Brightness Slider Window
// -----------------------------------------------------------------------------------------------

// Subclass proc that forwards Escape/Enter from child slider controls to the parent window,
// allowing the brightness popup to be dismissed by keyboard even when a slider has focus.
static LRESULT CALLBACK SliderKeyboardProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                           UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/)
{
  if (msg == WM_KEYDOWN && (wParam == VK_ESCAPE || wParam == VK_RETURN))
  {
    SendMessage(GetParent(hwnd), WM_KEYDOWN, wParam, lParam);
    return 0;
  }
  return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void ShowBrightnessSlider(HWND parent)
{
  const auto &monitors = BrightnessController::GetMonitors();
  int monitorCount = (int)monitors.size();

  if (monitorCount == 0)
    return;

  // Destroy existing window to recreate with updated layout (e.g., if monitor count changed)
  if (g_hwnd_brightness && IsWindow(g_hwnd_brightness))
  {
    DestroyWindow(g_hwnd_brightness);
    g_hwnd_brightness = nullptr;
  }

  // Register window class if necessary
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

  // Calculate window dimensions
  using namespace GuiConstants;
  int totalWidth = PADDING;
  int totalHeight = WINDOW_BASE_HEIGHT;

  // First pass: calculate total width required
  for (int i = 0; i < monitorCount; i++)
  {
    MonitorSettings settings = g_settings.getMonitorSettings(monitors[i].deviceName);
    int groupWidth = 0;
    if (settings.showSoftware)
      groupWidth += SLIDER_GROUP_WIDTH;
    if (settings.showHardware)
      groupWidth += SLIDER_GROUP_WIDTH;
    if (groupWidth == 0)
      groupWidth = SLIDER_GROUP_WIDTH; // Minimum placeholder width

    totalWidth += groupWidth + PADDING;
  }

  // Position near cursor
  POINT pt;
  GetCursorPos(&pt);
  int x = pt.x - totalWidth / 2;
  int y = pt.y - totalHeight - 20;

  // Ensure window is on screen (basic clamping)
  // Get the monitor containing the cursor
  HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
  MONITORINFO mi = {};
  mi.cbSize = sizeof(MONITORINFO);
  GetMonitorInfo(hMonitor, &mi);

  int screenLeft = mi.rcMonitor.left;
  int screenTop = mi.rcMonitor.top;
  if (x < screenLeft)
    x = screenLeft;
  if (y < screenTop)
    y = screenTop;
  if (x + totalWidth > mi.rcMonitor.right)
    x = mi.rcMonitor.right - totalWidth;
  if (y + totalHeight > mi.rcMonitor.bottom)
    y = mi.rcMonitor.bottom - totalHeight;

  // Final safety clamp in case window is larger than screen
  if (x < screenLeft)
    x = screenLeft;
  if (y < screenTop)
    y = screenTop;

  // Create Main Window
  g_hwnd_brightness = CreateWindowEx(
      WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
      L"CandelaBrightnessSlider",
      L"Brightness",
      WS_POPUP,
      x, y, totalWidth, totalHeight,
      parent, nullptr, g_hInstance, nullptr);

  // Create Controls for each monitor
  int currentX = PADDING;
  for (int i = 0; i < monitorCount; i++)
  {
    MonitorSettings settings = g_settings.getMonitorSettings(monitors[i].deviceName);

    int groupWidth = 0;
    if (settings.showSoftware)
      groupWidth += SLIDER_GROUP_WIDTH;
    if (settings.showHardware)
      groupWidth += SLIDER_GROUP_WIDTH;
    if (groupWidth == 0)
      groupWidth = SLIDER_GROUP_WIDTH;

    int baseY = 30; // Space for monitor label
    int baseID = ID_SLIDER_BASE + (i * ID_SLIDER_STRIDE);

    // Monitor Label
    wchar_t monitorLabel[64];
    swprintf_s(monitorLabel, _countof(monitorLabel), L"Display %d", i + 1);
    CreateWindowEx(
        0, WC_STATIC, monitorLabel,
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        currentX, 5, groupWidth, 20,
        g_hwnd_brightness, (HMENU)(intptr_t)(baseID + OFFSET_MONITOR_LABEL), g_hInstance, nullptr);

    int sliderX = currentX;

    // Software Slider Construction
    if (settings.showSoftware)
    {
      HWND hSlider = CreateWindowEx(
          0, TRACKBAR_CLASS, L"",
          WS_CHILD | WS_VISIBLE | TBS_VERT | TBS_AUTOTICKS | TBS_BOTH,
          sliderX, baseY, SLIDER_GROUP_WIDTH, SLIDER_HEIGHT,
          g_hwnd_brightness, (HMENU)(intptr_t)(baseID + OFFSET_SW_SLIDER), g_hInstance, nullptr);

      CreateWindowEx(
          0, WC_STATIC, L"Software",
          WS_CHILD | WS_VISIBLE | SS_CENTER,
          sliderX, baseY + SLIDER_HEIGHT, SLIDER_GROUP_WIDTH, 20,
          g_hwnd_brightness, (HMENU)(intptr_t)(baseID + OFFSET_SW_LABEL), g_hInstance, nullptr);

      HWND hValue = CreateWindowEx(
          0, WC_STATIC, L"",
          WS_CHILD | WS_VISIBLE | SS_CENTER,
          sliderX, baseY + SLIDER_HEIGHT + 20, SLIDER_GROUP_WIDTH, 20,
          g_hwnd_brightness, (HMENU)(intptr_t)(baseID + OFFSET_SW_VALUE), g_hInstance, nullptr);

      // Initialize slider value
      int val = BrightnessController::GetSoftwareBrightness(i);
      if (val < 1)
        val = 100; // Defensive fallback
      SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELONG(SLIDER_MIN, SLIDER_MAX));
      SendMessage(hSlider, TBM_SETPOS, TRUE, SLIDER_MAX + SLIDER_MIN - val); // Invert: top = 100%
      SendMessage(hSlider, TBM_SETTICFREQ, 10, 0);
      SetWindowSubclass(hSlider, SliderKeyboardProc, 0, 0);

      wchar_t buffer[20];
      swprintf_s(buffer, L"%d%%", val);
      SetWindowText(hValue, buffer);

      sliderX += SLIDER_GROUP_WIDTH;
    }

    // Hardware Slider Construction
    if (settings.showHardware)
    {
      HWND hSlider = CreateWindowEx(
          0, TRACKBAR_CLASS, L"",
          WS_CHILD | WS_VISIBLE | TBS_VERT | TBS_AUTOTICKS | TBS_BOTH,
          sliderX, baseY, SLIDER_GROUP_WIDTH, SLIDER_HEIGHT,
          g_hwnd_brightness, (HMENU)(intptr_t)(baseID + OFFSET_HW_SLIDER), g_hInstance, nullptr);

      CreateWindowEx(
          0, WC_STATIC, L"Hardware",
          WS_CHILD | WS_VISIBLE | SS_CENTER,
          sliderX, baseY + SLIDER_HEIGHT, SLIDER_GROUP_WIDTH, 20,
          g_hwnd_brightness, (HMENU)(intptr_t)(baseID + OFFSET_HW_LABEL), g_hInstance, nullptr);

      HWND hValue = CreateWindowEx(
          0, WC_STATIC, L"",
          WS_CHILD | WS_VISIBLE | SS_CENTER,
          sliderX, baseY + SLIDER_HEIGHT + 20, SLIDER_GROUP_WIDTH, 20,
          g_hwnd_brightness, (HMENU)(intptr_t)(baseID + OFFSET_HW_VALUE), g_hInstance, nullptr);

      int val = BrightnessController::GetHardwareBrightness(i);
      if (val < 0)
        val = 50; // Defensive fallback
      SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELONG(HW_SLIDER_MIN, SLIDER_MAX));
      SendMessage(hSlider, TBM_SETPOS, TRUE, SLIDER_MAX - val); // Invert: top = 100%, bottom = 0%
      SendMessage(hSlider, TBM_SETTICFREQ, 10, 0);
      SetWindowSubclass(hSlider, SliderKeyboardProc, 0, 0);

      wchar_t buffer[20];
      swprintf_s(buffer, L"%d%%", val);
      SetWindowText(hValue, buffer);

      if (!monitors[i].supportsHardwareBrightness)
      {
        EnableWindow(hSlider, FALSE);
        SetWindowText(hValue, L"N/A");
      }
    }

    currentX += groupWidth + PADDING;
  }

  ShowWindow(g_hwnd_brightness, SW_SHOW);
  UpdateWindow(g_hwnd_brightness);
  SetForegroundWindow(g_hwnd_brightness);
}

LRESULT CALLBACK BrightnessSliderProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  using namespace GuiConstants;

  switch (message)
  {
  case WM_HSCROLL:
  case WM_VSCROLL:
  {
    HWND trackbar = (HWND)lParam;
    int controlID = GetDlgCtrlID(trackbar);
    int relativeID = controlID - ID_SLIDER_BASE;

    if (relativeID >= 0)
    {
      int monitorIndex = relativeID / ID_SLIDER_STRIDE;
      int type = relativeID % ID_SLIDER_STRIDE;
      int pos = (int)SendMessage(trackbar, TBM_GETPOS, 0, 0);
      // Invert position back to brightness: software uses [1,100], hardware uses [0,100]
      int brightness = (type == OFFSET_SW_SLIDER)
                           ? (SLIDER_MAX + SLIDER_MIN - pos)
                           : (SLIDER_MAX - pos);

      const auto &monitors = BrightnessController::GetMonitors();
      if (monitorIndex < (int)monitors.size())
      {
        MonitorSettings settings = g_settings.getMonitorSettings(monitors[monitorIndex].deviceName);

        if (type == OFFSET_SW_SLIDER)
        {
          BrightnessController::SetSoftwareBrightness(monitorIndex, brightness);
          settings.lastSoftwareBrightness = brightness;
          g_settings.setMonitorSettings(monitors[monitorIndex].deviceName, settings);

          // Update Value Label
          int valueID = ID_SLIDER_BASE + (monitorIndex * ID_SLIDER_STRIDE) + OFFSET_SW_VALUE;
          HWND hValue = GetDlgItem(hwnd, valueID);
          wchar_t buffer[20];
          swprintf_s(buffer, L"%d%%", brightness);
          SetWindowText(hValue, buffer);
        }
        else if (type == OFFSET_HW_SLIDER)
        {
          BrightnessController::SetHardwareBrightness(monitorIndex, brightness);
          settings.lastHardwareBrightness = brightness;
          g_settings.setMonitorSettings(monitors[monitorIndex].deviceName, settings);

          // Update Value Label
          int valueID = ID_SLIDER_BASE + (monitorIndex * ID_SLIDER_STRIDE) + OFFSET_HW_VALUE;
          HWND hValue = GetDlgItem(hwnd, valueID);
          wchar_t buffer[20];
          swprintf_s(buffer, L"%d%%", brightness);
          SetWindowText(hValue, buffer);
        }
      }
    }
    break;
  }
  case WM_DESTROY:
  {
    g_hwnd_brightness = nullptr;
    // Persist settings immediately upon window closure
    g_settings.save();
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

// -----------------------------------------------------------------------------------------------
// Settings Window
// -----------------------------------------------------------------------------------------------

void ShowSettingsDialog(HWND parent)
{
  if (g_settings_hwnd && IsWindow(g_settings_hwnd))
  {
    DestroyWindow(g_settings_hwnd); // Recreate to refresh list
    g_settings_hwnd = nullptr;
  }

  BrightnessController::RefreshMonitors();
  const auto &monitors = BrightnessController::GetMonitors();
  int monitorCount = (int)monitors.size();

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

  int baseHeight = 60;
  int perMonitorHeight = 80;
  int width = 300;
  int height = baseHeight + (monitorCount * perMonitorHeight) + 40;

  g_settings_hwnd = CreateWindowEx(
      0,
      L"CandelaSettings",
      L"Settings",
      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
      CW_USEDEFAULT, CW_USEDEFAULT,
      width, height,
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

  using namespace GuiConstants;

  g_hwnd_startup_checkbox = CreateWindowEx(
      0, L"BUTTON", L"Start on boot",
      BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,
      10, 10, 200, 30,
      g_settings_hwnd, (HMENU)ID_SETTINGS_STARTUP, g_hInstance, nullptr);

  SendMessage(g_hwnd_startup_checkbox, BM_SETCHECK, g_settings.getStartOnBoot() ? BST_CHECKED : BST_UNCHECKED, 0);

  int currentY = 50;
  for (int i = 0; i < monitorCount; i++)
  {
    MonitorSettings settings = g_settings.getMonitorSettings(monitors[i].deviceName);
    int baseID = ID_SETTINGS_MONITOR_BASE + (i * ID_SETTINGS_STRIDE);

    // Group Box/Label for Monitor
    wchar_t buffer[64];
    swprintf_s(buffer, L"Display %d", i + 1);
    CreateWindowEx(0, L"BUTTON", buffer, BS_GROUPBOX | WS_CHILD | WS_VISIBLE,
                   10, currentY, width - 40, 70, g_settings_hwnd, (HMENU)-1, g_hInstance, nullptr);

    // Software Checkbox
    HWND swCheck = CreateWindowEx(
        0, L"BUTTON", L"Show Software Brightness",
        BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,
        20, currentY + 20, 240, 20,
        g_settings_hwnd, (HMENU)(intptr_t)(baseID + OFFSET_SETTINGS_SW_CHECK), g_hInstance, nullptr);

    SendMessage(swCheck, BM_SETCHECK, settings.showSoftware ? BST_CHECKED : BST_UNCHECKED, 0);

    // Hardware Checkbox
    HWND hwCheck = CreateWindowEx(
        0, L"BUTTON", L"Show Hardware Brightness",
        BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,
        20, currentY + 45, 240, 20,
        g_settings_hwnd, (HMENU)(intptr_t)(baseID + OFFSET_SETTINGS_HW_CHECK), g_hInstance, nullptr);

    SendMessage(hwCheck, BM_SETCHECK, settings.showHardware ? BST_CHECKED : BST_UNCHECKED, 0);

    currentY += perMonitorHeight;
  }

  ShowWindow(g_settings_hwnd, SW_SHOW);
  UpdateWindow(g_settings_hwnd);
}

LRESULT CALLBACK SettingsProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  using namespace GuiConstants;

  switch (message)
  {
  case WM_COMMAND:
  {
    int controlId = LOWORD(wParam);
    if (HIWORD(wParam) == BN_CLICKED)
    {
      if (controlId == ID_SETTINGS_STARTUP)
      {
        g_settings.setStartOnBoot(SendMessage(g_hwnd_startup_checkbox, BM_GETCHECK, 0, 0) == BST_CHECKED);
        g_settings.save();
      }
      else if (controlId >= ID_SETTINGS_MONITOR_BASE)
      {
        int relative = controlId - ID_SETTINGS_MONITOR_BASE;
        int monitorIndex = relative / ID_SETTINGS_STRIDE;
        int type = relative % ID_SETTINGS_STRIDE;

        const auto &monitors = BrightnessController::GetMonitors();
        if (monitorIndex < (int)monitors.size())
        {
          std::wstring deviceName = monitors[monitorIndex].deviceName;
          MonitorSettings settings = g_settings.getMonitorSettings(deviceName);

          if (type == OFFSET_SETTINGS_SW_CHECK)
          {
            settings.showSoftware = (SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED);
          }
          else if (type == OFFSET_SETTINGS_HW_CHECK)
          {
            settings.showHardware = (SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED);
          }

          // Validate: Ensure at least one control remains enabled to prevent lockout
          if (!settings.showSoftware && !settings.showHardware)
          {
            MessageBox(hwnd, L"You must have at least one brightness slider enabled for this monitor.", L"Configuration Error", MB_OK | MB_ICONERROR);
            // Revert UI state
            SendMessage((HWND)lParam, BM_SETCHECK, BST_CHECKED, 0);
            // Revert internal state
            if (type == OFFSET_SETTINGS_SW_CHECK)
              settings.showSoftware = true;
            else
              settings.showHardware = true;
          }

          g_settings.setMonitorSettings(deviceName, settings);
          g_settings.save();
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

// -----------------------------------------------------------------------------------------------
// Info Window
// -----------------------------------------------------------------------------------------------

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
      g_info_hwnd, (HMENU)GuiConstants::ID_INFO_TEXT, g_hInstance, nullptr);

  HWND infoText = GetDlgItem(g_info_hwnd, GuiConstants::ID_INFO_TEXT);
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
                L"simultaneously if required.");

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