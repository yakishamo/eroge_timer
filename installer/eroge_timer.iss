#ifndef AppVersion
  #define AppVersion "1.1.0"
#endif

#ifndef BuildDir
  #define BuildDir "..\build"
#endif

#define AppName "Eroge Timer"
#define AppPublisher "yakishamo"
#define AppExeName "eroge_timer.exe"

[Setup]
AppId={{A1BB0688-75F7-4938-99D7-7C5FC79E4162}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={localappdata}\Programs\Eroge Timer
DefaultGroupName=Eroge Timer
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
OutputDir={#BuildDir}
OutputBaseFilename=eroge_timer-v{#AppVersion}-setup
SetupIconFile=..\resources\app.ico
UninstallDisplayIcon={app}\{#AppExeName}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
CloseApplications=yes
RestartApplications=no
VersionInfoVersion={#AppVersion}.0
VersionInfoCompany={#AppPublisher}
VersionInfoDescription=Eroge Timer installer
VersionInfoProductName={#AppName}
VersionInfoProductVersion={#AppVersion}

[Languages]
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"

[Tasks]
Name: "desktopicon"; Description: "デスクトップにショートカットを作成"; GroupDescription: "追加ショートカット:"; Flags: unchecked
Name: "startup"; Description: "Windowsへのサインイン時に自動起動"; GroupDescription: "自動起動:"; Flags: unchecked

[Files]
Source: "{#BuildDir}\{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\CHANGELOG.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\RELEASE_NOTES_v{#AppVersion}.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\assets\app-icon.png"; DestDir: "{app}\assets"; Flags: ignoreversion

[Icons]
Name: "{group}\Eroge Timer"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"
Name: "{autodesktop}\Eroge Timer"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon
Name: "{userstartup}\Eroge Timer"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; Tasks: startup

[Run]
Filename: "{app}\{#AppExeName}"; Description: "Eroge Timerを起動"; Flags: nowait postinstall skipifsilent
