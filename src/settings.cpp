#include "settings.h"
#include <shlobj.h>
#include <string>

const wchar_t* const Settings::REGISTRY_KEY = L"Software\\Candela";
const wchar_t* const Settings::BRIGHTNESS_VALUE = L"Brightness";
const wchar_t* const Settings::MODE_VALUE = L"HardwareMode";
const wchar_t* const Settings::START_ON_BOOT_VALUE = L"StartOnBoot";
const wchar_t* const Settings::DEFAULT_MODE_VALUE = L"DefaultMode";

Settings::Settings() 
    : m_brightness(50)
    , m_hardwareMode(false)  // Default to software mode
    , m_startOnBoot(false)
    , m_defaultMode(false)   // Default to software as primary mode
{
}

Settings::~Settings() {
    save();
}

bool Settings::load() {
    HKEY hKey;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey);
    
    if (result == ERROR_SUCCESS) {
        DWORD value, size = sizeof(DWORD);
        
        // Load brightness
        result = RegQueryValueEx(hKey, BRIGHTNESS_VALUE, nullptr, nullptr, 
                                reinterpret_cast<LPBYTE>(&value), &size);
        if (result == ERROR_SUCCESS) {
            m_brightness = static_cast<int>(value);
        }
        
        // Load hardware mode
        size = sizeof(DWORD);
        result = RegQueryValueEx(hKey, MODE_VALUE, nullptr, nullptr, 
                                reinterpret_cast<LPBYTE>(&value), &size);
        if (result == ERROR_SUCCESS) {
            m_hardwareMode = (value != 0);
        }
        
        // Load start on boot
        size = sizeof(DWORD);
        result = RegQueryValueEx(hKey, START_ON_BOOT_VALUE, nullptr, nullptr, 
                                reinterpret_cast<LPBYTE>(&value), &size);
        if (result == ERROR_SUCCESS) {
            m_startOnBoot = (value != 0);
        }
        
        // Load default mode
        size = sizeof(DWORD);
        result = RegQueryValueEx(hKey, DEFAULT_MODE_VALUE, nullptr, nullptr, 
                                reinterpret_cast<LPBYTE>(&value), &size);
        if (result == ERROR_SUCCESS) {
            m_defaultMode = (value != 0);
        }
        
        RegCloseKey(hKey);
    }
    
    // Set start on boot in Windows registry if needed
    HKEY hRunKey;
    result = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                         0, KEY_WRITE, &hRunKey);
    
    if (result == ERROR_SUCCESS) {
        if (m_startOnBoot) {
            // Get the current executable path
            wchar_t exePath[MAX_PATH];
            GetModuleFileName(nullptr, exePath, MAX_PATH);
            
            // Add to startup
            RegSetValueEx(hRunKey, L"Candela", 0, REG_SZ, 
                         reinterpret_cast<const BYTE*>(exePath), 
                         (wcslen(exePath) + 1) * sizeof(wchar_t));
        } else {
            // Remove from startup if not needed
            RegDeleteValue(hRunKey, L"Candela");
        }
        RegCloseKey(hRunKey);
    }
    
    return true;
}

bool Settings::save() const {
    HKEY hKey;
    LONG result = RegCreateKeyEx(HKEY_CURRENT_USER, REGISTRY_KEY, 0, nullptr, 
                                REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    
    if (result != ERROR_SUCCESS) {
        return false;
    }
    
    DWORD value;
    
    // Save brightness
    value = static_cast<DWORD>(m_brightness);
    RegSetValueEx(hKey, BRIGHTNESS_VALUE, 0, REG_DWORD, 
                  reinterpret_cast<const BYTE*>(&value), sizeof(value));
    
    // Save hardware mode
    value = m_hardwareMode ? 1 : 0;
    RegSetValueEx(hKey, MODE_VALUE, 0, REG_DWORD, 
                  reinterpret_cast<const BYTE*>(&value), sizeof(value));
    
    // Save start on boot
    value = m_startOnBoot ? 1 : 0;
    RegSetValueEx(hKey, START_ON_BOOT_VALUE, 0, REG_DWORD, 
                  reinterpret_cast<const BYTE*>(&value), sizeof(value));
    
    // Save default mode
    value = m_defaultMode ? 1 : 0;
    RegSetValueEx(hKey, DEFAULT_MODE_VALUE, 0, REG_DWORD, 
                  reinterpret_cast<const BYTE*>(&value), sizeof(value));
    
    RegCloseKey(hKey);
    
    // Update startup registry based on setting
    HKEY hRunKey;
    result = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                         0, KEY_WRITE, &hRunKey);
    
    if (result == ERROR_SUCCESS) {
        if (m_startOnBoot) {
            // Get the current executable path
            wchar_t exePath[MAX_PATH];
            GetModuleFileName(nullptr, exePath, MAX_PATH);
            
            // Add to startup
            RegSetValueEx(hRunKey, L"Candela", 0, REG_SZ, 
                         reinterpret_cast<const BYTE*>(exePath), 
                         (wcslen(exePath) + 1) * sizeof(wchar_t));
        } else {
            // Remove from startup if not needed
            RegDeleteValue(hRunKey, L"Candela");
        }
        RegCloseKey(hRunKey);
    }
    
    return true;
}