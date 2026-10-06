#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <iostream>

#include "tray.h"
#include "gui.h"
#include "settings.h"
#include "brightness.h"
#include "colortemp.h"
#include "bwfilter.h"
#include "resource.h"

// Global application instance
HINSTANCE g_hInstance = nullptr;
HWND g_hwnd = nullptr;
Settings g_settings;

namespace
{
  // Timer IDs on the main (hidden) window
  const UINT_PTR TIMER_DISPLAY_SETTLE = 1;
  const UINT_PTR TIMER_HW_PROBE = 2;

  // How long display changes must stop arriving before monitors are re-enumerated.
  // A mode switch (e.g. a game entering or leaving exclusive fullscreen) can send
  // several WM_DISPLAYCHANGE messages in a row, and monitors ignore DDC/CI while
  // they re-sync to the new signal. Re-arming the timer on each message collapses
  // a burst into a single refresh once things have settled.
  const UINT DISPLAY_SETTLE_DELAY_MS = 2000;

  // Spacing and limit for re-probing monitors whose DDC/CI didn't answer during
  // enumeration. A monitor that never answers ends up shown as unsupported.
  const UINT HW_PROBE_INTERVAL_MS = 2000;
  const int HW_PROBE_MAX_ATTEMPTS = 5;

  bool g_displaySettlePending = false;
  int g_hwProbeAttemptsLeft = 0;
}

// Function to restore brightness settings on startup
void RestoreBrightnessOnStartup();

// Re-enumerate monitors after a display change or resume, once they have settled
void ScheduleDisplayRefresh(HWND hwnd);

// Periodic re-probing of monitors whose hardware brightness support is unconfirmed
void ScheduleHardwareProbes(HWND hwnd);
void RetryHardwareProbes(HWND hwnd);

// Forward declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
  g_hInstance = hInstance;

  // Initialize common controls
  INITCOMMONCONTROLSEX icc;
  icc.dwSize = sizeof(icc);
  icc.dwICC = ICC_BAR_CLASSES;
  InitCommonControlsEx(&icc);

  // Load settings
  g_settings.load();

  // Initialise the Magnification runtime once for the lifetime of the process
  // (used by BWFilter to apply the system-wide grayscale colour effect).
  BWFilter::Initialize();

  // Register window class
  const wchar_t CLASS_NAME[] = L"CandelaTrayWindowClass";
  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.lpszClassName = CLASS_NAME;
  wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

  RegisterClassExW(&wc);

  // Create main window (hidden helper window)
  g_hwnd = CreateWindowExW(
      0,
      CLASS_NAME,
      L"Candela Brightness Control",
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
      nullptr, nullptr, hInstance, nullptr);

  if (!g_hwnd)
  {
    return 1;
  }

  // Create tray icon
  if (!Tray::createTray(g_hwnd))
  {
    MessageBoxW(nullptr, L"Failed to create tray icon", L"Error", MB_OK | MB_ICONERROR);
    return 1;
  }

  // Ensure window is hidden as we run in the tray
  ShowWindow(g_hwnd, SW_HIDE);
  UpdateWindow(g_hwnd);

  // Apply saved brightness settings (moved after window creation)
  RestoreBrightnessOnStartup();
  ScheduleHardwareProbes(g_hwnd);

  // Main message loop
  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  // Clean up tray icon
  Tray::removeTray(g_hwnd);

  // Return gamma ramps to identity so software brightness and colour temperature
  // don't outlive the app, then release monitor handles. Hardware brightness is
  // the monitor's own setting and is deliberately left as the user set it.
  BrightnessController::ResetGammaRamps();
  BrightnessController::Cleanup();

  // Clear the grayscale colour effect and release Magnification resources.
  BWFilter::Cleanup();

  return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_CREATE:
  {
    break;
  }
  case WM_DISPLAYCHANGE:
  {
    ScheduleDisplayRefresh(hwnd);
    break;
  }
  case WM_POWERBROADCAST:
  {
    if (wParam == PBT_APMRESUMEAUTOMATIC)
    {
      ScheduleDisplayRefresh(hwnd);
    }
    break;
  }
  case WM_TIMER:
  {
    if (wParam == TIMER_DISPLAY_SETTLE)
    {
      KillTimer(hwnd, TIMER_DISPLAY_SETTLE);
      g_displaySettlePending = false;
      BrightnessController::RefreshMonitors();
      RestoreBrightnessOnStartup();
      ScheduleHardwareProbes(hwnd);
    }
    else if (wParam == TIMER_HW_PROBE)
    {
      RetryHardwareProbes(hwnd);
    }
    break;
  }
  case WM_APP + 1:
  {
    if (LOWORD(lParam) == WM_LBUTTONDOWN)
    {
      // Handle left click - show brightness slider. Any monitor still showing
      // "N/A" gets another round of probes so it can recover while visible.
      Tray::handleLeftClick(g_hwnd);
      ScheduleHardwareProbes(g_hwnd);
    }
    else if (LOWORD(lParam) == WM_RBUTTONDOWN || LOWORD(lParam) == WM_CONTEXTMENU)
    {
      // Handle right click - show context menu. Opening Settings re-enumerates
      // monitors, so probe any that didn't answer.
      POINT pt;
      GetCursorPos(&pt);
      Tray::handleRightClick(g_hwnd, pt.x, pt.y);
      ScheduleHardwareProbes(g_hwnd);
    }
    break;
  }
  case WM_DESTROY:
  {
    PostQuitMessage(0);
    break;
  }
  default:
    return DefWindowProc(hwnd, message, wParam, lParam);
  }
  return 0;
}

// Function to restore brightness settings on startup
void RestoreBrightnessOnStartup()
{
  // Initialize brightness control
  if (!BrightnessController::Initialize())
  {
    return;
  }

  // Apply saved brightness settings per monitor, in the documented order:
  // hardware brightness → software brightness → colour temp.
  const auto &monitors = BrightnessController::GetMonitors();
  for (size_t i = 0; i < monitors.size(); ++i)
  {
    MonitorSettings settings = g_settings.getMonitorSettings(monitors[i].deviceName);

    BrightnessController::SetHardwareBrightness(static_cast<int>(i), settings.lastHardwareBrightness);
    BrightnessController::SetSoftwareBrightness(static_cast<int>(i), settings.lastSoftwareBrightness);
    BrightnessController::SetSoftwareColorTemp(static_cast<int>(i), settings.lastStandardColorTemp);
  }

  // System-wide B&W filter (Magnification API). Independent of the gamma
  // pipeline above; sits on top of the final composited desktop.
  BWFilter::SetEnabled(g_settings.getBWEnabled());
}

void ScheduleDisplayRefresh(HWND hwnd)
{
  // The displays are mid-switch and can't answer DDC/CI, so probing now would
  // only block; the refresh below starts a fresh round once they've settled.
  KillTimer(hwnd, TIMER_HW_PROBE);
  g_hwProbeAttemptsLeft = 0;

  // SetTimer with an existing ID resets it, debouncing bursts of changes.
  SetTimer(hwnd, TIMER_DISPLAY_SETTLE, DISPLAY_SETTLE_DELAY_MS, nullptr);
  g_displaySettlePending = true;
}

void ScheduleHardwareProbes(HWND hwnd)
{
  // The pending display refresh will schedule probes once the displays settle.
  if (g_displaySettlePending)
    return;

  const auto &monitors = BrightnessController::GetMonitors();
  for (size_t i = 0; i < monitors.size(); ++i)
  {
    if (BrightnessController::IsHardwareProbePending(static_cast<int>(i)))
    {
      // Top up the attempt budget; only start the timer if it isn't already
      // running, so repeated clicks don't keep pushing the next probe back.
      bool running = g_hwProbeAttemptsLeft > 0;
      g_hwProbeAttemptsLeft = HW_PROBE_MAX_ATTEMPTS;
      if (!running)
        SetTimer(hwnd, TIMER_HW_PROBE, HW_PROBE_INTERVAL_MS, nullptr);
      return;
    }
  }
}

void RetryHardwareProbes(HWND hwnd)
{
  // Ignore a tick that slips through after probing was cancelled by a display
  // change (e.g. one already being retrieved when KillTimer ran).
  if (g_hwProbeAttemptsLeft <= 0 || g_displaySettlePending)
  {
    KillTimer(hwnd, TIMER_HW_PROBE);
    return;
  }

  bool stillPending = false;

  const auto &monitors = BrightnessController::GetMonitors();
  for (size_t i = 0; i < monitors.size(); ++i)
  {
    int index = static_cast<int>(i);
    if (!BrightnessController::IsHardwareProbePending(index))
      continue;

    if (BrightnessController::ProbeHardwareBrightness(index))
    {
      // Apply the saved level, as RestoreBrightnessOnStartup would have done
      // had the monitor answered during enumeration.
      MonitorSettings settings = g_settings.getMonitorSettings(monitors[i].deviceName);
      BrightnessController::SetHardwareBrightness(index, settings.lastHardwareBrightness);
      UpdateHardwareSlider(index);
    }
    else
    {
      stillPending = true;
    }
  }

  if (!stillPending || --g_hwProbeAttemptsLeft <= 0)
  {
    KillTimer(hwnd, TIMER_HW_PROBE);
    g_hwProbeAttemptsLeft = 0;
  }
}