; Candela Brightness Control NSIS Installer Script
;----------------------------------------------

!define APPNAME "Candela Brightness Control"
!define COMPANYNAME "Candela"
!define DESCRIPTION "System tray application for monitor brightness control"
!define VERSIONMAJOR 1
!define VERSIONMINOR 0
!define VERSIONBUILD 0

;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"

;--------------------------------
;General

  ;Name and file
  Name "${APPNAME}"
  OutFile "Candela-Setup.exe"
  
  ;Default installation folder
  InstallDir $PROGRAMFILES64\Candela
  
  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\Candela" ""

  ;Request application privileges for Windows Vista and later
  RequestExecutionLevel admin

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "LICENSE.txt"  ; 
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections

Section "Candela" SecMain
  SetOutPath "$INSTDIR"
  
  ; Files to be installed
  File "build\candela.exe"
  File "README.md"  ;

  ; Store installation folder
  WriteRegStr HKCU "Software\Candela" "" $INSTDIR
  
  ; Create uninstaller
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Candela" \
                   "DisplayName" "${APPNAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Candela" \
                   "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Candela" \
                   "DisplayIcon" "$INSTDIR\candela.exe,0"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Candela" \
                   "Publisher" "${COMPANYNAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Candela" \
                   "DisplayVersion" "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Candela" \
                   "VersionMajor" ${VERSIONMAJOR}
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Candela" \
                   "VersionMinor" ${VERSIONMINOR}
  
  WriteUninstaller "$INSTDIR\uninstall.exe"
  
  ; Add to startup if user had it enabled in previous version
  ReadRegStr $0 HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "Candela"
  ${If} $0 != ""
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "Candela" "$INSTDIR\candela.exe"
  ${EndIf}
  
SectionEnd

; Optional section - Start Menu Shortcuts
Section "Start Menu Shortcuts" SecShortcuts
  CreateDirectory "$SMPROGRAMS\Candela"
  CreateShortCut "$SMPROGRAMS\Candela\${APPNAME}.lnk" "$INSTDIR\candela.exe"
  CreateShortCut "$SMPROGRAMS\Candela\Uninstall.lnk" "$INSTDIR\uninstall.exe"
SectionEnd



;--------------------------------
;Uninstaller Section

Section "Uninstall"
  Delete "$INSTDIR\candela.exe"
  Delete "$INSTDIR\readme.txt"
  Delete "$INSTDIR\uninstall.exe"
  
  RMDir "$INSTDIR"
  
  Delete "$SMPROGRAMS\Candela\${APPNAME}.lnk"
  Delete "$SMPROGRAMS\Candela\Uninstall.lnk"
  RMDir "$SMPROGRAMS\Candela"
  
  DeleteRegKey /ifempty HKCU "Software\Candela"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Candela"
  
  ; Remove from startup
  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "Candela"
SectionEnd