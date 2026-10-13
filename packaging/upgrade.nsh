!define MUI_CUSTOMFUNCTION_GUIINIT NotyExistingInstall

; Preserve an explicit /D or directory-page choice. Adopt an existing Noty
; directory only while the installer still uses one of its default locations.
Function NotyExistingInstall
  Push $R0
  Push $R1
  StrCmp $INSTDIR "$DOCUMENTS\Noty" noty_detect
  StrCmp $INSTDIR "$PROGRAMFILES\Noty" noty_detect
  StrCmp $INSTDIR "$PROGRAMFILES64\Noty" noty_detect noty_path_done
  noty_detect:
    StrCpy $R1 "JustMe"
    ReadRegStr $R0 HKCU "Software\Lyvt-Dev\Noty" ""
    StrCmp $R0 "" +2
    IfFileExists "$R0\bin\Noty.exe" noty_use_existing 0
    StrCpy $R1 "AllUsers"
    ReadRegStr $R0 HKLM "Software\Lyvt-Dev\Noty" ""
    StrCmp $R0 "" +2
    IfFileExists "$R0\bin\Noty.exe" noty_use_existing 0
    StrCpy $R1 "JustMe"
    ReadRegStr $R0 HKCU "Software\Lyvt-Dev\Noty 0.4.0" ""
    StrCmp $R0 "" +2
    IfFileExists "$R0\bin\Noty.exe" noty_use_existing 0
    StrCpy $R1 "AllUsers"
    ReadRegStr $R0 HKLM "Software\Lyvt-Dev\Noty 0.4.0" ""
    StrCmp $R0 "" noty_path_done
    IfFileExists "$R0\bin\Noty.exe" noty_use_existing noty_path_done
  noty_use_existing:
    StrCpy $INSTDIR $R0
    StrCpy $IS_DEFAULT_INSTALLDIR 0
    StrCpy $SV_ALLUSERS $R1
    StrCmp $R1 "JustMe" 0 noty_context_all
      SetShellVarContext current
      Goto noty_path_done
    noty_context_all:
      SetShellVarContext all
  noty_path_done:
  Pop $R1
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
