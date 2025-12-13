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
