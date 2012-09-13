; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=Knights
AppVerName=Knights
AppPublisher=Stephen Thompson
AppPublisherURL=http://www.knightsgame.org.uk/
AppSupportURL=http://www.knightsgame.org.uk/
AppUpdatesURL=http://www.knightsgame.org.uk/
DefaultDirName={pf}\Knights
DefaultGroupName=Knights
LicenseFile=COPYRIGHT.txt
OutputBaseFilename=knights_022_installer
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "Knights.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "SDL.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "COPYRIGHT.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "README-SDL.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "knights_data\client\*"; DestDir: "{app}\knights_data\client"; Flags: ignoreversion
Source: "knights_data\client\std_files\*"; DestDir: "{app}\knights_data\client\std_files"; Flags: ignoreversion
Source: "knights_data\server\*"; DestDir: "{app}\knights_data\server"; Flags: ignoreversion
Source: "knights_data\server\classic\*"; DestDir: "{app}\knights_data\server\classic"; Flags: ignoreversion
Source: "knights_data\server\menu\*"; DestDir: "{app}\knights_data\server\menu"; Flags: ignoreversion
Source: "knights_data\server\tutorial\*"; DestDir: "{app}\knights_data\server\tutorial"; Flags: ignoreversion
Source: "docs\*"; DestDir: "{app}\docs"; Flags: ignoreversion
Source: "docs\manual\*"; DestDir: "{app}\docs\manual"; Flags: ignoreversion
Source: "docs\manual\images\*"; DestDir: "{app}\docs\manual\images"; Flags: ignoreversion
Source: "docs\third_party_licences\*"; DestDir: "{app}\docs\third_party_licences"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\Knights"; Filename: "{app}\Knights.exe"; WorkingDir: "{app}"
Name: "{group}\Knights Manual"; Filename: "{app}\docs\manual\index.html"
Name: "{group}\{cm:UninstallProgram,Knights}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\Knights"; Filename: "{app}\Knights.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\knights.exe"; Description: "{cm:LaunchProgram,Knights}"; Flags: nowait postinstall skipifsilent unchecked

