#pragma once
#include <windows.h>
#include <vector>
#include <string>

/**
 * @brief Represents a physical or logical display monitor.
 *
 * Contains handles and state information for controlling both
 * software (gamma-based) and hardware (DDC/CI) brightness.
 */
struct Monitor
{
  HMONITOR hMonitor;
  HDC hdc; // Device Context for software brightness (Gamma)
  std::wstring deviceName;
  int softwareBrightness;  // Current software brightness level (1-100)
  int softwareColorTemp;   // Current software color temperature in Kelvin (1200-6500)
  int hardwareBrightness;  // Current hardware brightness level (0-100)
  HANDLE hPhysicalMonitor; // Handle for hardware brightness (DDC/CI)
  bool supportsHardwareBrightness;
  DWORD hwNativeMin; // Monitor's native DDC/CI brightness minimum
  DWORD hwNativeMax; // Monitor's native DDC/CI brightness maximum

  Monitor()
      : hMonitor(nullptr),
        hdc(nullptr),
        softwareBrightness(100),
        softwareColorTemp(6500),
        hardwareBrightness(50),
        hPhysicalMonitor(nullptr),
        supportsHardwareBrightness(false),
        hwNativeMin(0),
        hwNativeMax(100) {}
};

/**
 * @brief Static controller for managing monitor brightness operations.
 *
 * Handles enumeration of monitors and application of brightness changes
 * via both software (gamma ramp) and hardware (DDC/CI) methods.
 */
class BrightnessController
{
public:
  // Prevent instantiation
  BrightnessController() = delete;

  /**
   * @brief Initializes the brightness control system and enumerates monitors.
   * @return true if initialization was successful.
   */
  static bool Initialize();

  /**
   * @brief Refreshes the list of connected monitors.
   * @return true if monitors were found.
   */
  static bool RefreshMonitors();

  /**
   * @brief Cleans up resources (device contexts, physical monitor handles).
   */
  static void Cleanup();

  /**
   * @brief Retrieves the list of currently detected monitors.
   * @return A constant reference to the vector of Monitor objects.
   */
  static const std::vector<Monitor> &GetMonitors();

  /**
   * @brief Sets the hardware brightness for a specific monitor.
   * @param monitorIndex Index of the monitor in the list.
   * @param brightness Desired brightness level (0-100).
   * @return true if the operation succeeded.
   */
  static bool SetHardwareBrightness(int monitorIndex, int brightness);

  /**
   * @brief Sets the software brightness for a specific monitor.
   * @param monitorIndex Index of the monitor in the list.
   * @param brightness Desired brightness level (1-100).
   * @return true if the operation succeeded.
   */
  static bool SetSoftwareBrightness(int monitorIndex, int brightness);

  /**
   * @brief Gets the current hardware brightness for a specific monitor.
   * @param monitorIndex Index of the monitor in the list.
   * @return Current brightness (0-100), or -1 if the index is invalid or an error occurs.
   */
  static int GetHardwareBrightness(int monitorIndex);

  /**
   * @brief Gets the current software brightness for a specific monitor.
   * @param monitorIndex Index of the monitor in the list.
   * @return Current brightness (1-100), or -1 if the index is invalid or an error occurs.
   */
  static int GetSoftwareBrightness(int monitorIndex);

  /**
   * @brief Sets the software color temperature for a specific monitor via gamma ramp.
   * @param monitorIndex Index of the monitor in the list.
   * @param kelvin Desired color temperature in Kelvin (1200-6500).
   * @return true if the operation succeeded.
   */
  static bool SetSoftwareColorTemp(int monitorIndex, int kelvin);

  /**
   * @brief Gets the current software color temperature for a specific monitor.
   * @param monitorIndex Index of the monitor in the list.
   * @return Current temperature in Kelvin, or -1 if the index is invalid.
   */
  static int GetSoftwareColorTemp(int monitorIndex);
};

/**
 * @brief Maps a linear brightness value (1-100) to a safe gamma factor (49-100).
 *
 * Exposed for use by ColorTempUtils::ApplyGammaRamp so brightness and color
 * temperature are always written together in a single gamma ramp.
 *
 * @param brightness Input brightness 1-100
 * @return Remapped value in [49, 100]; caller divides by 100 to get factor
 */
double MapBrightnessToSafeFactor(int brightness);