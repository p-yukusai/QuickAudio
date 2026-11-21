#define AppName "QuickAudio"
#define AppVersion "1.0.0"
#ifndef StageDir
  #define StageDir "cmake-build-release\installer-stage"
#endif
#ifndef InstallerOutputDir
  #define InstallerOutputDir "cmake-build-release"
#endif

[Setup]
AppId={{8959C799-0C82-43B9-8E71-6EC9318CE594}
AppName={#AppName}
AppVersion={#AppVersion}
DefaultDirName={autopf}\QuickAudio
DefaultGroupName=QuickAudio
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir={#InstallerOutputDir}
OutputBaseFilename=QuickAudio-Setup
Compression=lzma2
SolidCompression=yes
PrivilegesRequired=admin
SetupIconFile=res\icon.ico
UninstallDisplayIcon={app}\QuickAudio.exe

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: unchecked
Name: "vcredist"; Description: "Install Microsoft Visual C++ runtime"; Flags: checkedonce

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Excludes: "vc_redist.x64.exe"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#StageDir}\vc_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall; Tasks: vcredist

[Icons]
Name: "{group}\QuickAudio"; Filename: "{app}\QuickAudio.exe"
Name: "{autodesktop}\QuickAudio"; Filename: "{app}\QuickAudio.exe"; Tasks: desktopicon

[Run]
Filename: "{tmp}\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Microsoft Visual C++ runtime..."; Tasks: vcredist; Flags: waituntilterminated
Filename: "{app}\QuickAudio.exe"; Description: "Launch QuickAudio"; Flags: nowait postinstall skipifsilent