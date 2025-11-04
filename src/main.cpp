#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <iostream>

#include "tray.h"
#include "settings.h"
#include "resource.h"

// Global application instance
HINSTANCE g_hInstance = nullptr;
HWND g_hwnd = nullptr;
Settings g_settings;

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

  // Create main window
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

  // Don't show the main window - only the tray icon
  // ShowWindow(g_hwnd, nCmdShow);
  // UpdateWindow(g_hwnd);

  // Only show the window if specifically requested (e.g., for debugging)
  // For normal operation, keep it hidden
  ShowWindow(g_hwnd, SW_HIDE);
  UpdateWindow(g_hwnd);

  // Main message loop
  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  // Clean up tray icon
  Tray::removeTray(g_hwnd);

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
  case WM_APP + 1:
  {
    if (LOWORD(lParam) == WM_LBUTTONDOWN)
    {
      // Handle left click - show brightness slider
      Tray::handleLeftClick(g_hwnd);
    }
    else if (LOWORD(lParam) == WM_RBUTTONDOWN || LOWORD(lParam) == WM_CONTEXTMENU)
    {
      // Handle right click - show context menu
      POINT pt;
      GetCursorPos(&pt);
      Tray::handleRightClick(g_hwnd, pt.x, pt.y);
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