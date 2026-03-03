; Candela Brightness Control NSIS Installer Script
;----------------------------------------------

!define APPNAME       "Candela Brightness Control"
!define COMPANYNAME   "Candela"
!define DESCRIPTION   "System tray application for monitor brightness control"
!define VERSIONMAJOR  1
!define VERSIONMINOR  0
!define VERSIONBUILD  0
!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Candela"

;--------------------------------
; Modern UI

  !include "MUI2.nsh"

;--------------------------------
; General

  Name    "${APPNAME}"
  OutFile "Candela-Setup.exe"

  ; Default installation folder
  InstallDir "$PROGRAMFILES64\Candela"

  ; Use previously recorded installation path if available
  InstallDirRegKey HKCU "Software\Candela" ""

  RequestExecutionLevel admin

;--------------------------------
; Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
; Installer Pages

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

;--------------------------------
; Uninstaller Pages

  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
; Language

  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
; Installer Section

Section "Candela" SecMain

  ; Kill any running instance so files are not locked during install
  nsExec::Exec '"taskkill" /f /im candela.exe'
  Pop $0  ; discard exit code — non-zero is normal if the process was not running

  ; Wipe the existing installation directory for a clean install
  RMDir /r "$INSTDIR"

  SetOutPath "$INSTDIR"
  File "build\candela.exe"
  File "README.md"

  ; Record the installation path for future upgrades
  WriteRegStr HKCU "Software\Candela" "" "$INSTDIR"

  ; Register with Windows Add/Remove Programs
  WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayName"    "${APPNAME}"
  WriteRegStr   HKLM "${UNINSTALL_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayIcon"    "$INSTDIR\candela.exe,0"
  WriteRegStr   HKLM "${UNINSTALL_KEY}" "Publisher"      "${COMPANYNAME}"
  WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayVersion" "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}"
  WriteRegDWORD HKLM "${UNINSTALL_KEY}" "VersionMajor"  ${VERSIONMAJOR}
  WriteRegDWORD HKLM "${UNINSTALL_KEY}" "VersionMinor"  ${VERSIONMINOR}
  WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoModify"      1
  WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoRepair"      1

  WriteUninstaller "$INSTDIR\uninstall.exe"

  ; If the app was previously configured to start on boot, update the path
  ReadRegStr $0 HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "Candela"
  ${If} $0 != ""
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "Candela" "$INSTDIR\candela.exe"
  ${EndIf}

SectionEnd

;--------------------------------
; Start Menu Shortcuts (optional section)

Section "Start Menu Shortcuts" SecShortcuts
  CreateDirectory "$SMPROGRAMS\Candela"
  CreateShortCut  "$SMPROGRAMS\Candela\${APPNAME}.lnk" "$INSTDIR\candela.exe"
  CreateShortCut  "$SMPROGRAMS\Candela\Uninstall.lnk"  "$INSTDIR\uninstall.exe"
SectionEnd

;--------------------------------
; Uninstaller Section

Section "Uninstall"

  ; Kill any running instance so files are not locked during removal
  nsExec::Exec '"taskkill" /f /im candela.exe'
  Pop $0  ; discard exit code

  ; Remove all installed files and the installation directory
  RMDir /r "$INSTDIR"

  ; Remove Start Menu shortcuts
  Delete "$SMPROGRAMS\Candela\${APPNAME}.lnk"
  Delete "$SMPROGRAMS\Candela\Uninstall.lnk"
  RMDir  "$SMPROGRAMS\Candela"

  ; Remove startup entry
  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "Candela"

  ; Remove application settings (brightness preferences, monitor config)
  DeleteRegKey HKCU "Software\Candela"

  ; Remove Add/Remove Programs entry
  DeleteRegKey HKLM "${UNINSTALL_KEY}"

SectionEnd
