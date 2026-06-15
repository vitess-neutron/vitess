; -- Vitess.iss --
[Setup]
AppName=Vitess
AppVersion=3.8
AppCopyright=GNU GPL (C) 2025 Forschungszentrum Jülich GmbH
DefaultDirName={sd}\Vitess3.8
UsePreviousAppDir=no
DirExistsWarning=no
DisableProgramGroupPage=yes
PrivilegesRequired=none


[Files]
Source: "C:\vitess3.8\*"; DestDir: "{app}"; Flags: recursesubdirs; Components: main
Source: "C:\tcl\*"; DestDir: "{app}\tcl"; Flags: recursesubdirs; Components: tcl
Source: "C:\gr\*"; DestDir: "{app}\gr"; Flags: recursesubdirs; Components: gr

[Icons]
Name: "{userdesktop}\Vitess3.8"; Filename: "{app}\tcl\bin\wish86t.exe"; IconFilename: "{app}\BITMAPS\vwish.ico"; Parameters: " -f Vitess"; WorkingDir: "{app}"; Tasks: desktopicon

[Components]
Name: "main"; Description: "Vitess"; Types: full compact custom; Flags: fixed
Name: "tcl"; Description: "Tcl/TK 8.6"; Types: full
Name: "gr"; Description: "grplot"; Types: full

[Tasks]
Name: desktopicon; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"; Components: main

[Code]
function NextButtonClick(CurPage: Integer): Boolean;
begin
  Result := True;
  if CurPage > 1 then begin
    if Pos(' ', ExpandConstant('{app}')) > 0 then begin
      MsgBox('Make sure the whole path to the VITESS directory has no blanks. Otherwise it will not run after installation.', mbInformation, MB_OK);
      Result := False;
    end;
  end;
end;
