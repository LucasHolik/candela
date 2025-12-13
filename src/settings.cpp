#include "settings.h"
#include <shlobj.h>
#include <string>
#include <algorithm>

const wchar_t *const Settings::REGISTRY_KEY = L"Software\\Candela";
const wchar_t *const Settings::MONITORS_SUBKEY = L"Monitors";
const wchar_t *const Settings::START_ON_BOOT_VALUE = L"StartOnBoot";

Settings::Settings()
    : m_startOnBoot(false)
{
}

Settings::~Settings()
{
  save();
}

std::wstring Settings::sanitizeDeviceName(const std::wstring &deviceName)
{
  std::wstring sanitized;
  sanitized.reserve(deviceName.length());
  for (wchar_t c : deviceName)
  {
    if (c == L'\\')
    {
      sanitized += L"##";
    }
    else if (c == L'#')
    {
      sanitized += L"#0";
    }
    else
    {
      sanitized += c;
    }
  }
  return sanitized;
}

MonitorSettings Settings::getMonitorSettings(const std::wstring &deviceName) const
{
  auto it = m_monitorSettings.find(deviceName);
  if (it != m_monitorSettings.end())
  {
    return it->second;
  }
  return MonitorSettings(); // Return defaults
}

void Settings::setMonitorSettings(const std::wstring &deviceName, const MonitorSettings &settings)
{
  m_monitorSettings[deviceName] = settings;
}

bool Settings::load()
{
  HKEY hKey;
  LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey);

  if (result == ERROR_SUCCESS)
  {
    DWORD value, size = sizeof(DWORD);

    // Load start on boot
    size = sizeof(DWORD);
    result = RegQueryValueEx(hKey, START_ON_BOOT_VALUE, nullptr, nullptr,
                             reinterpret_cast<LPBYTE>(&value), &size);
    if (result == ERROR_SUCCESS)
    {
      m_startOnBoot = (value != 0);
    }

    // Load Monitors
    HKEY hMonitorsKey;
    result = RegOpenKeyEx(hKey, MONITORS_SUBKEY, 0, KEY_READ, &hMonitorsKey);
    if (result == ERROR_SUCCESS)
    {
      wchar_t subKeyName[256];
      DWORD index = 0;
      DWORD subKeyLen = 256;
      while (RegEnumKeyEx(hMonitorsKey, index, subKeyName, &subKeyLen, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS)
      {
        std::wstring sanitizedName = subKeyName;
        std::wstring realName;
        realName.reserve(sanitizedName.length());

        for (size_t i = 0; i < sanitizedName.length(); ++i)
        {
          if (sanitizedName[i] == L'#')
          {
            if (i + 1 < sanitizedName.length())
            {
              if (sanitizedName[i + 1] == L'#')
              {
                realName += L'\\';
                i++;
              }
              else if (sanitizedName[i + 1] == L'0')
              {
                realName += L'#';
                i++;
              }
              else
              {
                // Legacy fallback: single # treated as backslash
                realName += L'\\';
              }
            }
            else
            {
              // Trailing # (legacy)
              realName += L'\\';
            }
          }
          else
          {
            realName += sanitizedName[i];
          }
        }

        MonitorSettings settings;
        HKEY hMonitorKey;
        if (RegOpenKeyEx(hMonitorsKey, subKeyName, 0, KEY_READ, &hMonitorKey) == ERROR_SUCCESS)
        {
          DWORD dwVal;
          DWORD dwSize = sizeof(DWORD);

          if (RegQueryValueEx(hMonitorKey, L"ShowSoftware", nullptr, nullptr, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS)
            settings.showSoftware = (dwVal != 0);

          dwSize = sizeof(DWORD);
          if (RegQueryValueEx(hMonitorKey, L"ShowHardware", nullptr, nullptr, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS)
            settings.showHardware = (dwVal != 0);

          dwSize = sizeof(DWORD);
          if (RegQueryValueEx(hMonitorKey, L"LastSoftware", nullptr, nullptr, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS)
            settings.lastSoftwareBrightness = (int)dwVal;

          dwSize = sizeof(DWORD);
          if (RegQueryValueEx(hMonitorKey, L"LastHardware", nullptr, nullptr, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS)
            settings.lastHardwareBrightness = (int)dwVal;

          RegCloseKey(hMonitorKey);
          m_monitorSettings[realName] = settings;
        }

        index++;
        subKeyLen = 256;
      }
      RegCloseKey(hMonitorsKey);
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

  // Save start on boot
  value = m_startOnBoot ? 1 : 0;
  RegSetValueEx(hKey, START_ON_BOOT_VALUE, 0, REG_DWORD,
                reinterpret_cast<const BYTE *>(&value), sizeof(value));

  // Save Monitors
  HKEY hMonitorsKey;
  result = RegCreateKeyEx(hKey, MONITORS_SUBKEY, 0, nullptr,
                          REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hMonitorsKey, nullptr);

  if (result == ERROR_SUCCESS)
  {
    for (const auto &pair : m_monitorSettings)
    {
      std::wstring sanitizedName = sanitizeDeviceName(pair.first);
      const MonitorSettings &settings = pair.second;

      HKEY hMonitorKey;
      if (RegCreateKeyEx(hMonitorsKey, sanitizedName.c_str(), 0, nullptr,
                         REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hMonitorKey, nullptr) == ERROR_SUCCESS)
      {
        value = settings.showSoftware ? 1 : 0;
        RegSetValueEx(hMonitorKey, L"ShowSoftware", 0, REG_DWORD, (const BYTE *)&value, sizeof(value));

        value = settings.showHardware ? 1 : 0;
        RegSetValueEx(hMonitorKey, L"ShowHardware", 0, REG_DWORD, (const BYTE *)&value, sizeof(value));

        value = (DWORD)settings.lastSoftwareBrightness;
        RegSetValueEx(hMonitorKey, L"LastSoftware", 0, REG_DWORD, (const BYTE *)&value, sizeof(value));

        value = (DWORD)settings.lastHardwareBrightness;
        RegSetValueEx(hMonitorKey, L"LastHardware", 0, REG_DWORD, (const BYTE *)&value, sizeof(value));

        RegCloseKey(hMonitorKey);
      }
    }
    RegCloseKey(hMonitorsKey);
  }

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