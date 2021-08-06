#define MyAppName "Ships"
#define MyAppVersion "1.2.1"
#define MyAppExeName "Ships.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application. Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{D33DB72C-3506-472C-BEE8-80EEAFB1E776}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
DefaultDirName={autopf}\{#MyAppName}
DisableDirPage=yes
DisableProgramGroupPage=yes
; Remove the following line to run in administrative install mode (install for all users.)
PrivilegesRequired=lowest
OutputDir=bin
OutputBaseFilename=Ships
Compression=lzma
SolidCompression=yes
WizardStyle=modern
DisableFinishedPage=yes
DisableReadyPage=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "bin\Debug_x86\Ships.exe"; DestDir: "{app}"; Flags: ignoreversion

Source: "bin\Debug_x86\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist 
Source: "res\*"; DestDir: "{app}\res"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
;Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait

[Code]
#ifdef UNICODE
  #define AW "W"
#else
  #define AW "A"
#endif
type
  HINSTANCE = THandle;

function ShellExecute(hwnd: HWND; lpOperation: string; lpFile: string;
  lpParameters: string; lpDirectory: string; nShowCmd: Integer): HINSTANCE;
  external 'ShellExecute{#AW}@shell32.dll stdcall';

function InitializeSetup: Boolean;
begin
  // if this instance of the setup is not silent which is by running
  // setup binary without /SILENT parameter, stop the initialization
  Result := WizardSilent;
  // if this instance is not silent, then...
  if not Result then
  begin
    // re-run the setup with /SILENT parameter; because executing of
    // the setup loader is not possible with ShellExec function, we
    // need to use a WinAPI workaround
    if ShellExecute(0, '', ExpandConstant('{srcexe}'), '/SILENT', '',
      SW_SHOW) <= 32
    then
      // if re-running this setup to silent mode failed, let's allow
      // this non-silent setup to be run
      Result := True;
  end;
end;