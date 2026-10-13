!define MUI_CUSTOMFUNCTION_GUIINIT NotyExistingInstall

; Preserve an explicit /D or directory-page choice. Adopt an existing Noty
; directory only while the installer still uses one of its default locations.
Function NotyExistingInstall
  Push $R0
  StrCmp $INSTDIR "$DOCUMENTS\Noty" noty_detect
  StrCmp $INSTDIR "$PROGRAMFILES\Noty" noty_detect
  StrCmp $INSTDIR "$PROGRAMFILES64\Noty" noty_detect noty_path_done
  noty_detect:
    ReadRegStr $R0 HKCU "Software\Lyvt-Dev\Noty" ""
    IfFileExists "$R0\bin\Noty.exe" noty_use_existing 0
    ReadRegStr $R0 HKLM "Software\Lyvt-Dev\Noty" ""
    IfFileExists "$R0\bin\Noty.exe" noty_use_existing 0
    ReadRegStr $R0 HKCU "Software\Lyvt-Dev\Noty 0.4.0" ""
    IfFileExists "$R0\bin\Noty.exe" noty_use_existing 0
    ReadRegStr $R0 HKLM "Software\Lyvt-Dev\Noty 0.4.0" ""
    IfFileExists "$R0\bin\Noty.exe" noty_use_existing noty_path_done
  noty_use_existing:
    StrCpy $INSTDIR $R0
  noty_path_done:
  Pop $R0
FunctionEnd

Function NotyRemoveLegacyEntry
  Push $R0
  ReadRegStr $R0 HKCU "Software\Lyvt-Dev\Noty 0.4.0" ""
  StrCmp $R0 $INSTDIR 0 noty_legacy_machine
    DeleteRegKey HKCU "Software\Lyvt-Dev\Noty 0.4.0"
    DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Noty 0.4.0"
  noty_legacy_machine:
  ReadRegStr $R0 HKLM "Software\Lyvt-Dev\Noty 0.4.0" ""
  StrCmp $R0 $INSTDIR 0 noty_cleanup_done
    DeleteRegKey HKLM "Software\Lyvt-Dev\Noty 0.4.0"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Noty 0.4.0"
  noty_cleanup_done:
  Pop $R0
FunctionEnd
