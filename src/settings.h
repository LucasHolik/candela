#pragma once
#include <windows.h>
#include <string>
#include <map>

struct MonitorSettings
{
  bool showSoftware = true;
  bool showHardware = true;
  int lastSoftwareBrightness = 100; // Default to 100% for software (no dimming)
  int lastHardwareBrightness = 50;  // Default to 50% for hardware
  int lastStandardColorTemp = 6500; // Default to 6500K (neutral/daylight)
};

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
  bool getStartOnBoot() const { return m_startOnBoot; }
  bool getShowBWToggle() const { return m_showBWToggle; }
  bool getBWEnabled() const { return m_bwEnabled; }
  MonitorSettings getMonitorSettings(const std::wstring &deviceName) const;

  // Setters
  void setStartOnBoot(bool startOnBoot) { m_startOnBoot = startOnBoot; }
  void setShowBWToggle(bool v) { m_showBWToggle = v; }
  void setBWEnabled(bool v) { m_bwEnabled = v; }
  void setMonitorSettings(const std::wstring &deviceName, const MonitorSettings &settings);

private:
  bool m_startOnBoot;
  bool m_showBWToggle = false; // Whether the tray popup shows the global B&W checkbox
  bool m_bwEnabled = false;    // Persisted state of the global B&W filter
  std::map<std::wstring, MonitorSettings> m_monitorSettings;

  // Registry key for settings
  static const wchar_t *const REGISTRY_KEY;
  static const wchar_t *const MONITORS_SUBKEY;
  static const wchar_t *const START_ON_BOOT_VALUE;

  // Helper to sanitize device name for registry key
  static std::wstring sanitizeDeviceName(const std::wstring &deviceName);
};