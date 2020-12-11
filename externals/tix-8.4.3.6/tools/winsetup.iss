; -*- mode: text; fill-column: 75; tab-width: 8; coding: iso-latin-1-dos -*-
;
; $Id: winsetup.iss,v 1.1 2002/01/31 21:35:19 idiscovery Exp $
;
; winsetup.iss --
;
;       Inno Setup Script for creating Win32 installer program for
;       the Tix library.
;
; Copyright (c) 2000-2001 Tix Project Group.
;
; See the file "license.terms" for information on usage and redistribution
; of this file, and for a DISCLAIMER OF ALL WARRANTIES.

[Messages]

[Setup]
MinVersion=4.0.950,4.0.1381
AppCopyright=Copyright © 2001, Tix Project Group
AppName=Tix Widgets 8.2.0
AppVerName=Tix Widgets 8.2.0
AppVersion=8.2.0
EnableDirDoesntExistWarning=true
InfoBeforeFile=WinBin.txt
OutputDir=.
OutputBaseFilename=tix-8.2.0b3
LicenseFile=..\license.terms
AppPublisherURL=http://tix.sourceforge.net
AppSupportURL=http://tix.sourceforge.net
AppUpdatesURL=http://tix.sourceforge.net
DefaultGroupName=Tix 8.2.0
DefaultDirName={pf}\Tcl
UsePreviousAppDir=yes
DirExistsWarning=no
AllowRootDirectory=true
AlwaysShowGroupOnReadyPage=yes
BackSolid=yes
BackColor=$0000FF

[Types] 
Name: "full"; Description: "Full installation" 
Name: "compact"; Description: "Compact installation" 
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components] 
Name: "main"; Description: "Main Files"; Types: full compact custom; Flags: fixed 
Name: "demos"; Description: "Tix Demo Files"; Types: full
;;Name: "html"; Description: "Tix Manual Pages"; Types: full
Name: "pdf"; Description: "Tix User Guide"; Types: full

[Icons]
Name: {group}\tixwish; Filename: {app}\bin\tixwish82.exe; WorkingDir: {app}
Name: {group}\Tix Demo; Filename: "{app}\bin\tixwish82.exe"; Parameters: """{app}\lib\tix8.2\demos\tixwidgets.tcl"""; WorkingDir: {app}\bin; Components: demos
Name: {group}\Tix User Manual;Filename: {app}\docs\pdf\TixUser.pdf; Components: pdf
Name: {group}\Tix Programming Guide; Filename: {app}\docs\pguide-tix4.0.pdf; Components: pdf
Name: {group}\Release Notes; Filename: {app}\lib\tix8.2\Release-8.2.0.txt

[Run]
Filename: "{app}\lib\tix8.2\Release-8.2.0.txt"; Description: "View the Release notes"; Flags: postinstall shellexec skipifsilent unchecked 
Filename: "{app}\bin\tixwish82.exe"; Description: "Launch Demos"; Parameters: """{app}\lib\tix8.2\demos\tixwidgets.tcl"""; WorkingDir: {app}\bin; Flags: postinstall nowait skipifsilent; Components: demos

[LangOptions] 
LanguageName=English 
LanguageID=$0409 
DialogFontName=MS Shell Dlg 
DialogFontSize=8 
DialogFontStandardHeight=13 
TitleFontName=Arial 
TitleFontSize=29 
WelcomeFontName=Verdana 
WelcomeFontSize=12 
CopyrightFontName=Arial 
CopyrightFontSize=8

[Files]
Source: ..\win\Release\tixwish82.exe; DestDir: {app}\bin
Source: ..\win\Release\tixwishc82.exe; DestDir: {app}\bin
Source: ..\win\Release\tix82.dll; DestDir: {app}\bin
Source: ..\win\Release\tix82.lib; DestDir: {app}\lib
Source: ..\library\*.tcl; DestDir: {app}\lib\tix8.2
Source: ..\library\tclIndex; DestDir: {app}\lib\tix8.2
Source: ..\license.terms; DestDir: {app}\lib\tix8.2
Source: ..\docs\Release-8.2.0.txt; DestDir: {app}\lib\tix8.2

;
; This must appear below the line above. We are overridding the
; file ..\library\pkgIndex.tcl with ..\win\Release\pkgIndex.tcl.src
;
Source: ..\win\Release\pkgIndex.tcl.src; DestDir: {app}\lib\tix8.2; DestName: "pkgIndex.tcl"
Source: ..\library\pref\*.*; DestDir: {app}\lib\tix8.2\pref
Source: ..\library\bitmaps\*.*; DestDir: {app}\lib\tix8.2\bitmaps
Source: ..\demos\*.*; DestDir: {app}\lib\tix8.2\demos; Components: demos
Source: ..\demos\bitmaps\*.*; DestDir: {app}\lib\tix8.2\demos\bitmaps; Components: demos
Source: ..\demos\samples\*.*; DestDir: {app}\lib\tix8.2\demos\samples; Components: demos
;
; Docs
;
Source: ..\docs\pdf\TixUser.pdf; DestDir: {app}\doc; Components: pdf
Source: ..\docs\pdf\pguide-tix4.0.pdf; DestDir: {app}\doc; Components: pdf

[Dirs]


