; This script for Inno Setup (http://www.jrsoftware.org/isinfo.php)
; creates a windows installer for the debugging binaries of Python.
;
; It installs a debug python exe, a debug python dll, debug versions
; of the Python extensions, and debug libraries.
;
; An existing standard Python installation is required, the debug
; files are copied alongside the standard python files, the
; installation directory is read from the registry.
;

; XXX The python dll is copied to the installation directory, *not*
; into the system directory.

[Setup]
AppName=Python Debug Runtime
AppVerName=Python 2.3 Debug Runtime

DisableDirPage=yes
DefaultGroupName={reg:HKLM\SOFTWARE\Python\PythonCore\2.3\InstallPath\InstallGroup,|}
DisableProgramGroupPage=yes

SourceDir=.
OutputDir=.
OutputBaseFilename=Python-Debug-2.3.4
DefaultDirName={reg:HKLM\SOFTWARE\Python\PythonCore\2.3\InstallPath,|}

[Code]
function InitializeSetup(): Boolean;
begin
  Result := RegKeyExists(HKLM,'SOFTWARE\Python\PythonCore\2.3\InstallPath');
  if Result = False then
    MsgBox('Error: Python 2.3 not installed.', mbInformation, MB_OK);
end;

[Icons]
Name: "{group}\Python Debug (command line)"; Filename: "{app}\python_d.exe"

[Files]
; exe-files
Source: "python_d.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "w9xpopen_d.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "python_d.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "pythonw_d.exe"; DestDir: "{app}"; Flags: ignoreversion

; dlls
Source: "python23_d.dll"; DestDir: "{app}"; Flags: ignoreversion

; extension modules
Source: "zlib_d.pyd"; DestDir: "{app}\DLLs"; Flags: ignoreversion
Source: "_bsddb_d.pyd"; DestDir: "{app}\DLLs"; Flags: ignoreversion
Source: "_csv_d.pyd"; DestDir: "{app}\DLLs"; Flags: ignoreversion
Source: "_socket_d.pyd"; DestDir: "{app}\DLLs"; Flags: ignoreversion
Source: "_sre_d.pyd"; DestDir: "{app}\DLLs"; Flags: ignoreversion
Source: "_ssl_d.pyd"; DestDir: "{app}\DLLs"; Flags: ignoreversion
Source: "_symtable_d.pyd"; DestDir: "{app}\DLLs"; Flags: ignoreversion
Source: "_testcapi_d.pyd"; DestDir: "{app}\DLLs"; Flags: ignoreversion
Source: "_tkinter_d.pyd"; DestDir: "{app}\DLLs"; Flags: ignoreversion
Source: "_winreg_d.pyd"; DestDir: "{app}\DLLs"; Flags: ignoreversion
Source: "bz2_d.pyd"; DestDir: "{app}\DLLs"; Flags: ignoreversion
Source: "datetime_d.pyd"; DestDir: "{app}\DLLs"; Flags: ignoreversion
Source: "mmap_d.pyd"; DestDir: "{app}\DLLs"; Flags: ignoreversion
Source: "parser_d.pyd"; DestDir: "{app}\DLLs"; Flags: ignoreversion
Source: "pyexpat_d.pyd"; DestDir: "{app}\DLLs"; Flags: ignoreversion
Source: "select_d.pyd"; DestDir: "{app}\DLLs"; Flags: ignoreversion
Source: "unicodedata_d.pyd"; DestDir: "{app}\DLLs"; Flags: ignoreversion
Source: "winsound_d.pyd"; DestDir: "{app}\DLLs"; Flags: ignoreversion

; libraries
Source: "zlib_d.lib"; DestDir: "{app}\libs"; Flags: ignoreversion
Source: "_bsddb_d.lib"; DestDir: "{app}\libs"; Flags: ignoreversion
Source: "_csv_d.lib"; DestDir: "{app}\libs"; Flags: ignoreversion
Source: "_socket_d.lib"; DestDir: "{app}\libs"; Flags: ignoreversion
Source: "_sre_d.lib"; DestDir: "{app}\libs"; Flags: ignoreversion
Source: "_ssl_d.lib"; DestDir: "{app}\libs"; Flags: ignoreversion
Source: "_symtable_d.lib"; DestDir: "{app}\libs"; Flags: ignoreversion
Source: "_testcapi_d.lib"; DestDir: "{app}\libs"; Flags: ignoreversion
Source: "_tkinter_d.lib"; DestDir: "{app}\libs"; Flags: ignoreversion
Source: "_winreg_d.lib"; DestDir: "{app}\libs"; Flags: ignoreversion
Source: "bz2_d.lib"; DestDir: "{app}\libs"; Flags: ignoreversion
Source: "datetime_d.lib"; DestDir: "{app}\libs"; Flags: ignoreversion
Source: "mmap_d.lib"; DestDir: "{app}\libs"; Flags: ignoreversion
Source: "parser_d.lib"; DestDir: "{app}\libs"; Flags: ignoreversion
Source: "pyexpat_d.lib"; DestDir: "{app}\libs"; Flags: ignoreversion
Source: "python23_d.lib"; DestDir: "{app}\libs"; Flags: ignoreversion
Source: "select_d.lib"; DestDir: "{app}\libs"; Flags: ignoreversion
Source: "unicodedata_d.lib"; DestDir: "{app}\libs"; Flags: ignoreversion
Source: "winsound_d.lib"; DestDir: "{app}\libs"; Flags: ignoreversion
