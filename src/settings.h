#pragma once
#include <windows.h>
#include <string>

class Settings
{
public:
  Settings();
  ~Settings();

  // Load settings from registry
  bool load();

  // Save settings to registry
  bool save() const;

  // Update startup registry based on setting
  bool updateStartupRegistry() const;

  // Getters
  int getSoftwareBrightness() const { return m_softwareBrightness; }
  int getHardwareBrightness() const { return m_hardwareBrightness; }
  bool isHardwareMode() const { return m_hardwareMode; }
  bool getStartOnBoot() const { return m_startOnBoot; }
  bool getDefaultMode() const { return m_defaultMode; } // true for hardware, false for software
  bool getShowSoftwareBrightness() const { return m_showSoftwareBrightness; }
  bool getShowHardwareBrightness() const { return m_showHardwareBrightness; }

  // Setters
  void setSoftwareBrightness(int brightness) { m_softwareBrightness = brightness; }
  void setHardwareBrightness(int brightness) { m_hardwareBrightness = brightness; }
  void setHardwareMode(bool hardwareMode) { m_hardwareMode = hardwareMode; }
  void setStartOnBoot(bool startOnBoot) { m_startOnBoot = startOnBoot; }
  void setDefaultMode(bool defaultMode) { m_defaultMode = defaultMode; }
  void setShowSoftwareBrightness(bool show) { m_showSoftwareBrightness = show; }
  void setShowHardwareBrightness(bool show) { m_showHardwareBrightness = show; }

private:
  int m_softwareBrightness;
  int m_hardwareBrightness;
  bool m_hardwareMode;
  bool m_startOnBoot;
  bool m_defaultMode; // true for hardware by default, false for software
  bool m_showSoftwareBrightness;
  bool m_showHardwareBrightness;

  // Registry key for settings
  static const wchar_t *const REGISTRY_KEY;
  static const wchar_t *const SOFTWARE_BRIGHTNESS_VALUE;
  static const wchar_t *const HARDWARE_BRIGHTNESS_VALUE;
  static const wchar_t *const MODE_VALUE;
  static const wchar_t *const START_ON_BOOT_VALUE;
  static const wchar_t *const DEFAULT_MODE_VALUE;
  static const wchar_t *const SHOW_SOFTWARE_BRIGHTNESS_VALUE;
  static const wchar_t *const SHOW_HARDWARE_BRIGHTNESS_VALUE;
};