# Microsoft Developer Studio Project File - Name="_tkinter" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (ALPHA) Dynamic-Link Library" 0x0602

CFG=_tkinter - Win32 Alpha Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "_tkinter.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "_tkinter.mak" CFG="_tkinter - Win32 Alpha Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "_tkinter - Win32 Alpha Debug" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "_tkinter - Win32 Alpha Release" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "_tkinter - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "_tkinter - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "_tkinter"
# PROP Scc_LocalPath "..\..\.."

!IF  "$(CFG)" == "_tkinter - Win32 Alpha Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "."
# PROP Intermediate_Dir "alpha-temp-debug\_tkinter"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /Gt0 /W3 /GX /Zi /Od /I "..\Include" /I "..\PC" /I "C:\Program Files\Tcl\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "WITH_APPINIT" /YX /FD /c
# ADD CPP /nologo /MDd /Gt0 /W3 /GX /Zi /Od /I "..\Include" /I "..\PC" /I "C:\Program Files\Tcl\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "WITH_APPINIT" /YX /FD /c
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
# ADD BASE LINK32 tcl80.lib tk80.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /base:"0x1e190000" /subsystem:windows /dll /debug /machine:ALPHA /out:"./_tkinter_d.pyd" /pdbtype:sept /libpath:"C:\Program Files\Tcl\lib" /export:init_tkinter
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 ..\..\tcl\lib\tcl83.lib ..\..\tcl\lib\tk83.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib wsock32.lib /nologo /base:"0x1e190000" /subsystem:windows /dll /debug /machine:ALPHA /out:"alpha-temp-debug/_tkinter_d.pyd" /pdbtype:sept /libpath:"C:\Program Files\Tcl\lib" /export:init_tkinter
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "_tkinter - Win32 Alpha Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "."
# PROP Intermediate_Dir "alpha-temp-release\_tkinter"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /Gt0 /W3 /GX /Zi /O2 /I "..\Include" /I "..\PC" /I "C:\Program Files\Tcl\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "WITH_APPINIT" /YX /FD /c
# ADD CPP /nologo /MD /Gt0 /W3 /GX /O2 /I "..\Include" /I "..\PC" /I "..\..\Tcl\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "WITH_APPINIT" /YX /FD /c
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
# ADD BASE LINK32 tcl80.lib tk80.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /base:"0x1e190000" /subsystem:windows /dll /debug /machine:ALPHA /out:"./_tkinter.pyd" /libpath:"C:\Program Files\Tcl\lib" /export:init_tkinter
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 ..\..\tcl\lib\tcl83.lib ..\..\tcl\lib\tk83.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib wsock32.lib /nologo /base:"0x1e190000" /subsystem:windows /dll /debug /machine:ALPHA /out:"alpha-temp-release/_tkinter.pyd" /libpath:"C:\Program Files\Tcl\lib" /export:init_tkinter
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "_tkinter - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "."
# PROP Intermediate_Dir "x86-temp-debug\_tkinter"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=df.exe
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\tcl\include" /I "..\Include" /I "..\PC" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "WITH_APPINIT" /YX /FD /c
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
# ADD LINK32 ..\..\tcl\lib\tk83.lib ..\..\tcl\lib\tcl83.lib odbc32.lib odbccp32.lib user32.lib kernel32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /base:"0x1e190000" /subsystem:windows /dll /debug /machine:I386 /out:"./_tkinter_d.pyd" /pdbtype:sept /libpath:"C:\Program Files\Tcl\lib" /export:init_tkinter
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "_tkinter - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "."
# PROP Intermediate_Dir "x86-temp-release\_tkinter"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=df.exe
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "..\..\tcl\include" /I "..\Include" /I "..\PC" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "WITH_APPINIT" /YX /FD /c
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
# ADD LINK32 ..\..\tcl\lib\tk83.lib ..\..\tcl\lib\tcl83.lib odbc32.lib odbccp32.lib user32.lib kernel32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /base:"0x1e190000" /subsystem:windows /dll /debug /machine:I386 /out:"./_tkinter.pyd" /libpath:"C:\Program Files\Tcl\lib" /export:init_tkinter
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "_tkinter - Win32 Alpha Debug"
# Name "_tkinter - Win32 Alpha Release"
# Name "_tkinter - Win32 Debug"
# Name "_tkinter - Win32 Release"
# Begin Source File

SOURCE=..\Modules\_tkinter.c

!IF  "$(CFG)" == "_tkinter - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "_tkinter - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "_tkinter - Win32 Debug"

!ELSEIF  "$(CFG)" == "_tkinter - Win32 Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Modules\tkappinit.c

!IF  "$(CFG)" == "_tkinter - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "_tkinter - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "_tkinter - Win32 Debug"

!ELSEIF  "$(CFG)" == "_tkinter - Win32 Release"

!ENDIF 

# End Source File
# End Target
# End Project
