# Microsoft Developer Studio Project File - Name="bsddb" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (ALPHA) Dynamic-Link Library" 0x0602

CFG=bsddb - Win32 Alpha Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "bsddb.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "bsddb.mak" CFG="bsddb - Win32 Alpha Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "bsddb - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "bsddb - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "bsddb - Win32 Alpha Debug" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "bsddb - Win32 Alpha Release" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "bsddb"
# PROP Scc_LocalPath ".."

!IF  "$(CFG)" == "bsddb - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "."
# PROP Intermediate_Dir "x86-temp-release\bsddb"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=df.exe
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "..\Include" /I "..\PC" /I "..\..\bsddb\include" /I "..\..\bsddb\port\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 user32.lib kernel32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\..\bsddb\port\win32\db.lib /nologo /base:"0x1e180000" /subsystem:windows /dll /debug /machine:I386 /out:"./bsddb.pyd" /export:initbsddb
# SUBTRACT LINK32 /verbose /pdb:none

!ELSEIF  "$(CFG)" == "bsddb - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "."
# PROP Intermediate_Dir "x86-temp-debug\bsddb"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=df.exe
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\Include" /I "..\PC" /I "..\..\bsddb\include" /I "..\..\bsddb\port\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 user32.lib kernel32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\..\bsddb\port\win32\db.lib /nologo /base:"0x1e180000" /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"MSVCRT" /out:"./bsddb_d.pyd" /pdbtype:sept /export:initbsddb
# SUBTRACT LINK32 /verbose /pdb:none

!ELSEIF  "$(CFG)" == "bsddb - Win32 Alpha Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "."
# PROP Intermediate_Dir "alpha-temp-debug\bsddb"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /Gt0 /W3 /GX /Zi /Od /I "..\Include" /I "..\PC" /I "..\..\bsddb\include" /I "..\..\bsddb\port\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /Gt0 /W3 /GX /Zi /Od /I "..\Include" /I "..\PC" /I "..\..\bsddb\include" /I "..\..\bsddb\port\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\..\bsddb\port\win32\db.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:ALPHA /nodefaultlib:"MSVCRT" /out:"./bsddb_d.pyd" /pdbtype:sept /export:initbsddb
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 ..\..\bsddb\port\win32\db.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib wsock32.lib /nologo /base:"0x1e180000" /subsystem:windows /dll /debug /machine:ALPHA /nodefaultlib:"MSVCRT" /out:"alpha-temp-debug\bsddb_d.pyd" /pdbtype:sept /export:initbsddb
# SUBTRACT LINK32 /verbose /pdb:none

!ELSEIF  "$(CFG)" == "bsddb - Win32 Alpha Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "."
# PROP Intermediate_Dir "alpha-temp-release\bsddb"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /Gt0 /W3 /GX /Zi /O2 /I "..\Include" /I "..\PC" /I "..\..\bsddb\include" /I "..\..\bsddb\port\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /Gt0 /W3 /GX /O2 /I "..\Include" /I "..\PC" /I "..\..\bsddb\include" /I "..\..\bsddb\port\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\..\bsddb\port\win32\db.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:ALPHA /out:"./bsddb.pyd" /export:initbsddb
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 ..\..\bsddb\port\win32\db.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib wsock32.lib /nologo /base:"0x1e180000" /subsystem:windows /dll /debug /machine:ALPHA /out:"alpha-temp-release\bsddb.pyd" /export:initbsddb
# SUBTRACT LINK32 /verbose /pdb:none

!ENDIF 

# Begin Target

# Name "bsddb - Win32 Release"
# Name "bsddb - Win32 Debug"
# Name "bsddb - Win32 Alpha Debug"
# Name "bsddb - Win32 Alpha Release"
# Begin Source File

SOURCE=..\Modules\bsddbmodule.c

!IF  "$(CFG)" == "bsddb - Win32 Release"

# ADD CPP /I "..\..\bsddb\PORT\win32\include" /I "..\..\bsddb\PORT\include" /D "__DBINTERFACE_PRIVATE"
# SUBTRACT CPP /X /I "..\..\bsddb\port\include"

!ELSEIF  "$(CFG)" == "bsddb - Win32 Debug"

# ADD CPP /I "..\..\bsddb\PORT\win32\include" /I "..\..\bsddb\PORT\include" /D "__DBINTERFACE_PRIVATE"
# SUBTRACT CPP /I "..\..\bsddb\port\include"

!ELSEIF  "$(CFG)" == "bsddb - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "bsddb - Win32 Alpha Release"

!ENDIF 

# End Source File
# End Target
# End Project
