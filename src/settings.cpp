#include "settings.h"
#include <shlobj.h>
#include <string>

const wchar_t *const Settings::REGISTRY_KEY = L"Software\\Candela";
const wchar_t *const Settings::SOFTWARE_BRIGHTNESS_VALUE = L"SoftwareBrightness";
const wchar_t *const Settings::HARDWARE_BRIGHTNESS_VALUE = L"HardwareBrightness";
const wchar_t *const Settings::MODE_VALUE = L"HardwareMode";
const wchar_t *const Settings::START_ON_BOOT_VALUE = L"StartOnBoot";
const wchar_t *const Settings::DEFAULT_MODE_VALUE = L"DefaultMode";
const wchar_t *const Settings::SHOW_SOFTWARE_BRIGHTNESS_VALUE = L"ShowSoftwareBrightness";
const wchar_t *const Settings::SHOW_HARDWARE_BRIGHTNESS_VALUE = L"ShowHardwareBrightness";

Settings::Settings()
    : m_softwareBrightness(50), m_hardwareBrightness(50), m_hardwareMode(false) // Default to software mode
      ,
      m_startOnBoot(false), m_defaultMode(false) // Default to software as primary mode
      ,
      m_showSoftwareBrightness(true), m_showHardwareBrightness(true)
{
}

Settings::~Settings()
{
  save();
}

bool Settings::load()
{
  HKEY hKey;
  LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey);

  if (result == ERROR_SUCCESS)
  {
    DWORD value, size = sizeof(DWORD);

    // Load software brightness
    result = RegQueryValueEx(hKey, SOFTWARE_BRIGHTNESS_VALUE, nullptr, nullptr,
                             reinterpret_cast<LPBYTE>(&value), &size);
    if (result == ERROR_SUCCESS)
    {
      m_softwareBrightness = static_cast<int>(value);
    }

    // Load hardware brightness
    size = sizeof(DWORD);
    result = RegQueryValueEx(hKey, HARDWARE_BRIGHTNESS_VALUE, nullptr, nullptr,
                             reinterpret_cast<LPBYTE>(&value), &size);
    if (result == ERROR_SUCCESS)
    {
      m_hardwareBrightness = static_cast<int>(value);
    }

    // Load hardware mode
    size = sizeof(DWORD);
    result = RegQueryValueEx(hKey, MODE_VALUE, nullptr, nullptr,
                             reinterpret_cast<LPBYTE>(&value), &size);
    if (result == ERROR_SUCCESS)
    {
      m_hardwareMode = (value != 0);
    }

    // Load start on boot
    size = sizeof(DWORD);
    result = RegQueryValueEx(hKey, START_ON_BOOT_VALUE, nullptr, nullptr,
                             reinterpret_cast<LPBYTE>(&value), &size);
    if (result == ERROR_SUCCESS)
    {
      m_startOnBoot = (value != 0);
    }

    // Load default mode
    size = sizeof(DWORD);
    result = RegQueryValueEx(hKey, DEFAULT_MODE_VALUE, nullptr, nullptr,
                             reinterpret_cast<LPBYTE>(&value), &size);
    if (result == ERROR_SUCCESS)
    {
      m_defaultMode = (value != 0);
    }

    // Load show software brightness
    size = sizeof(DWORD);
    result = RegQueryValueEx(hKey, SHOW_SOFTWARE_BRIGHTNESS_VALUE, nullptr, nullptr,
                             reinterpret_cast<LPBYTE>(&value), &size);
    if (result == ERROR_SUCCESS)
    {
      m_showSoftwareBrightness = (value != 0);
    }

    // Load show hardware brightness
    size = sizeof(DWORD);
    result = RegQueryValueEx(hKey, SHOW_HARDWARE_BRIGHTNESS_VALUE, nullptr, nullptr,
                             reinterpret_cast<LPBYTE>(&value), &size);
    if (result == ERROR_SUCCESS)
    {
      m_showHardwareBrightness = (value != 0);
    }

    RegCloseKey(hKey);
  }

  // Update startup registry based on loaded setting
  updateStartupRegistry();

  return true;
}

bool Settings::save() const
{
  HKEY hKey;
  LONG result = RegCreateKeyEx(HKEY_CURRENT_USER, REGISTRY_KEY, 0, nullptr,
                               REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);

  if (result != ERROR_SUCCESS)
  {
    return false;
  }

  DWORD value;

  // Save software brightness
  value = static_cast<DWORD>(m_softwareBrightness);
  RegSetValueEx(hKey, SOFTWARE_BRIGHTNESS_VALUE, 0, REG_DWORD,
                reinterpret_cast<const BYTE *>(&value), sizeof(value));

  // Save hardware brightness
  value = static_cast<DWORD>(m_hardwareBrightness);
  RegSetValueEx(hKey, HARDWARE_BRIGHTNESS_VALUE, 0, REG_DWORD,
                reinterpret_cast<const BYTE *>(&value), sizeof(value));

  // Save hardware mode
  value = m_hardwareMode ? 1 : 0;
  RegSetValueEx(hKey, MODE_VALUE, 0, REG_DWORD,
                reinterpret_cast<const BYTE *>(&value), sizeof(value));

  // Save start on boot
  value = m_startOnBoot ? 1 : 0;
  RegSetValueEx(hKey, START_ON_BOOT_VALUE, 0, REG_DWORD,
                reinterpret_cast<const BYTE *>(&value), sizeof(value));

  // Save default mode
  value = m_defaultMode ? 1 : 0;
  RegSetValueEx(hKey, DEFAULT_MODE_VALUE, 0, REG_DWORD,
                reinterpret_cast<const BYTE *>(&value), sizeof(value));

  // Save show software brightness
  value = m_showSoftwareBrightness ? 1 : 0;
  RegSetValueEx(hKey, SHOW_SOFTWARE_BRIGHTNESS_VALUE, 0, REG_DWORD,
                reinterpret_cast<const BYTE *>(&value), sizeof(value));

  // Save show hardware brightness
  value = m_showHardwareBrightness ? 1 : 0;
  RegSetValueEx(hKey, SHOW_HARDWARE_BRIGHTNESS_VALUE, 0, REG_DWORD,
                reinterpret_cast<const BYTE *>(&value), sizeof(value));

  RegCloseKey(hKey);

  // Update startup registry based on setting
  updateStartupRegistry();

  return true;
}

bool Settings::updateStartupRegistry() const
{
  HKEY hRunKey;
  LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                             0, KEY_WRITE, &hRunKey);

  if (result == ERROR_SUCCESS)
  {
    if (m_startOnBoot)
    {
      // Get the current executable path
      wchar_t exePath[MAX_PATH];
      DWORD pathLength = GetModuleFileName(nullptr, exePath, MAX_PATH);

      if (pathLength > 0 && pathLength < MAX_PATH)
      {
        // Add to startup
        result = RegSetValueEx(hRunKey, L"Candela", 0, REG_SZ,
                               reinterpret_cast<const BYTE *>(exePath),
                               (wcslen(exePath) + 1) * sizeof(wchar_t));
      }
    }
    else
    {
      // Remove from startup if not needed
      result = RegDeleteValue(hRunKey, L"Candela");
      // ERROR_FILE_NOT_FOUND is acceptable when removing if key doesn't exist
      if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND)
      {
        // Log or handle the error appropriately
      }
    }
    RegCloseKey(hRunKey);
    return (result == ERROR_SUCCESS || (m_startOnBoot == false && result == ERROR_FILE_NOT_FOUND));
  }

  return false;
}