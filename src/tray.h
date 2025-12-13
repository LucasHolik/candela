#pragma once
#include <windows.h>

/**
 * @brief Manages the system tray icon and its interactions.
 */
class Tray
{
public:
  /**
   * @brief Creates and displays the system tray icon.
   * @param hwnd Handle to the main application window to receive tray messages.
   * @return true if successful, false otherwise.
   */
  static bool createTray(HWND hwnd);

  /**
   * @brief Removes the system tray icon.
   * @param hwnd Handle to the main application window.
   */
  static void removeTray(HWND hwnd);

  /**
   * @brief Handles left-click events on the tray icon (shows brightness slider).
   * @param hwnd Handle to the main application window.
   */
  static void handleLeftClick(HWND hwnd);

  /**
   * @brief Handles right-click events on the tray icon (shows context menu).
   * @param hwnd Handle to the main application window.
   * @param x Screen X coordinate of the cursor.
   * @param y Screen Y coordinate of the cursor.
   */
  static void handleRightClick(HWND hwnd, int x, int y);

  // Custom message ID for tray events
  static const UINT WM_TRAYICON = WM_APP + 1;

private:
  // Context menu command IDs
  static const UINT ID_EXIT = 1;
  static const UINT ID_SETTINGS = 2;
  static const UINT ID_INFO = 3;
};
