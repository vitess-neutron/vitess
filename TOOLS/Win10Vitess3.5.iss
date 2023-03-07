; -- Vitess.iss --                                           
[Setup]
AppName=Vitess
AppVerName=Vitess version 3.5
AppCopyright=GNU GPL (C) 2018 Helmholtz-Zentrum Berlin fuer Materialien und Energie GmbH
DefaultDirName={userprograms}\Vitess
DirExistsWarning=no
DisableProgramGroupPage=yes
PrivilegesRequired=none


[Files]
Source: "C:\vitess3.5\*"; DestDir: "{app}"; Flags: recursesubdirs; Components: main
Source: "C:\tcl\*"; DestDir: "{app}"; Flags: recursesubdirs; Components: tcl
Source: "C:\gnuplot\*"; DestDir: "{app}\gnuplot"; Flags: recursesubdirs; Components: gnuplot
Source: "C:\vitess3.5\install-windows.txt"; DestDir: "{app}"; Flags: isreadme

[Icons]
Name: "{userdesktop}\Vitess3.5"; Filename: "{app}\bin\wish86t.exe"; IconFilename: "{app}\Bitmaps\vwish.exe"; Parameters: " -f Vitess"; WorkingDir: "{app}"; Tasks: desktopicon

[Components]
Name: "main"; Description: "Vitess"; Types: full compact custom; Flags: fixed
Name: "tcl"; Description: "Tcl/TK 8.6"; Types: full
Name: "gnuplot"; Description: "Gnuplot 5.4"; Types: full

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
