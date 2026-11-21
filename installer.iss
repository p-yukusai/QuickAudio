#define AppName "QuickAudio"
#define AppVersion "1.0.0"

[Setup]
AppId={{8959C799-0C82-43B9-8E71-6EC9318CE594}
AppName={#AppName}
AppVersion={#AppVersion}
DefaultDirName={autopf}\QuickAudio
DefaultGroupName=QuickAudio
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=build-release
OutputBaseFilename=QuickAudio-Setup
Compression=lzma2
SolidCompression=yes
PrivilegesRequired=admin

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: unchecked
Name: "vcredist"; Description: "Install Microsoft Visual C++ runtime"; Flags: checkedonce

[Files]
Source: "build-release\installer-stage\*"; DestDir: "{app}"; Excludes: "vc_redist.x64.exe"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "build-release\installer-stage\vc_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall; Tasks: vcredist

[Icons]
Name: "{group}\QuickAudio"; Filename: "{app}\QuickAudio.exe"
Name: "{autodesktop}\QuickAudio"; Filename: "{app}\QuickAudio.exe"; Tasks: desktopicon

[Run]
Filename: "{tmp}\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Microsoft Visual C++ runtime..."; Tasks: vcredist; Flags: waituntilterminated
Filename: "{app}\QuickAudio.exe"; Description: "Launch QuickAudio"; Flags: nowait postinstall skipifsilent