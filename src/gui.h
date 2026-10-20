#pragma once
#include <windows.h>

// Forward declaration
class Settings;

/**
 * @brief Displays the brightness control slider window.
 * @param parent Handle to the parent window.
 */
void ShowBrightnessSlider(HWND parent);

/**
 * @brief Re-syncs the brightness popup's hardware slider for one monitor with
 *        the controller's cached state.
 *
 * Used when a monitor's DDC/CI support is confirmed after the popup was built,
 * so a slider showing "N/A" becomes usable without reopening the popup.
 * Does nothing if the popup doesn't exist or doesn't show that slider.
 *
 * @param monitorIndex Index of the monitor in the list.
 */
void UpdateHardwareSlider(int monitorIndex);

/**
 * @brief Displays the settings configuration dialog.
 * @param parent Handle to the parent window.
 */
void ShowSettingsDialog(HWND parent);

/**
 * @brief Displays the application information dialog.
 * @param parent Handle to the parent window.
 */
void ShowInfoDialog(HWND parent);

// Window Procedures
LRESULT CALLBACK BrightnessSliderProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SettingsProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK InfoProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
