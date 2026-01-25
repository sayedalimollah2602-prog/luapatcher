[Setup]
AppName=Steam Lua Patcher
AppVersion=1.0
AppPublisher=leVI
AppPublisherURL=https://github.com/sayedalimollah2602-prog/luapatcher
DefaultDirName={autopf}\SteamLuaPatcher
DefaultGroupName=Steam Lua Patcher
AllowNoIcons=yes
OutputDir=dist
OutputBaseFilename=SteamLuaPatcher_Setup
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; The main executable
Source: "dist\SteamLuaPatcher.exe"; DestDir: "{app}"; Flags: ignoreversion
; All other files in the dist folder (DLLs, plugins, etc.)
Source: "dist\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Steam Lua Patcher"; Filename: "{app}\SteamLuaPatcher.exe"
Name: "{commondesktop}\Steam Lua Patcher"; Filename: "{app}\SteamLuaPatcher.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\SteamLuaPatcher.exe"; Description: "{cm:LaunchProgram,Steam Lua Patcher}"; Flags: nowait postinstall skipifsilent
