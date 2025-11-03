#pragma once
#include <windows.h>
#include <string>

class Settings {
public:
    Settings();
    ~Settings();

    // Load settings from registry
    bool load();

    // Save settings to registry
    bool save() const;

    // Getters
    int getBrightness() const { return m_brightness; }
    bool isHardwareMode() const { return m_hardwareMode; }
    bool getStartOnBoot() const { return m_startOnBoot; }
    bool getDefaultMode() const { return m_defaultMode; } // true for hardware, false for software

    // Setters
    void setBrightness(int brightness) { m_brightness = brightness; }
    void setHardwareMode(bool hardwareMode) { m_hardwareMode = hardwareMode; }
    void setStartOnBoot(bool startOnBoot) { m_startOnBoot = startOnBoot; }
    void setDefaultMode(bool defaultMode) { m_defaultMode = defaultMode; }

private:
    int m_brightness;
    bool m_hardwareMode;
    bool m_startOnBoot;
    bool m_defaultMode;  // true for hardware by default, false for software

    // Registry key for settings
    static const wchar_t* const REGISTRY_KEY;
    static const wchar_t* const BRIGHTNESS_VALUE;
    static const wchar_t* const MODE_VALUE;
    static const wchar_t* const START_ON_BOOT_VALUE;
    static const wchar_t* const DEFAULT_MODE_VALUE;
};