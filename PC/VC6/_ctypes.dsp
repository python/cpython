# Microsoft Developer Studio Project File - Name="_ctypes" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=_ctypes - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "_ctypes.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "_ctypes.mak" CFG="_ctypes - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "_ctypes - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "_ctypes - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "_ctypes - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ""
# PROP Intermediate_Dir "x86-temp-release\_ctypes"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=df.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_CTYPES_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "..\..\Include" /I "..\..\PC" /I "..\..\Modules\_ctypes\libffi_msvc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"./_ctypes.pyd"

!ELSEIF  "$(CFG)" == "_ctypes - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir "x86-temp-debug\_ctypes"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=df.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_CTYPES_EXPORTS" /YX /FD /GZ  /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\..\Include" /I "..\..\PC" /I "..\..\Modules\_ctypes\libffi_msvc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /YX /FD /GZ  /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"./_ctypes_d.pyd" /pdbtype:sept
# SUBTRACT LINK32 /incremental:no

!ENDIF 

# Begin Target

# Name "_ctypes - Win32 Release"
# Name "_ctypes - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\Modules\_ctypes\_ctypes.c
# End Source File
# Begin Source File

SOURCE=..\..\Modules\_ctypes\callbacks.c
# End Source File
# Begin Source File

SOURCE=..\..\Modules\_ctypes\callproc.c
# End Source File
# Begin Source File

SOURCE=..\..\Modules\_ctypes\cfield.c
# End Source File
# Begin Source File

SOURCE=..\..\Modules\_ctypes\libffi_msvc\ffi.c
# End Source File
# Begin Source File

SOURCE=..\..\Modules\_ctypes\malloc_closure.c
# End Source File
# Begin Source File

SOURCE=..\..\Modules\_ctypes\libffi_msvc\prep_cif.c
# End Source File
# Begin Source File

SOURCE=..\..\Modules\_ctypes\stgdict.c
# End Source File
# Begin Source File

SOURCE=..\..\Modules\_ctypes\libffi_msvc\win32.c

!IF  "$(CFG)" == "_ctypes - Win32 Release"

# SUBTRACT CPP /I "..\..\Include"

!ELSEIF  "$(CFG)" == "_ctypes - Win32 Debug"

# ADD CPP /I "..\Include" /I "..\PC" /I "..\Modules\_ctypes\libffi_msvc"
# SUBTRACT CPP /I "..\..\Include"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\Modules\_ctypes\_ctypes_test.h
# End Source File
# Begin Source File

SOURCE=..\..\Modules\_ctypes\ctypes.h
# End Source File
# Begin Source File

SOURCE=..\..\Modules\_ctypes\ctypes_dlfcn.h
# End Source File
# Begin Source File

SOURCE=..\..\Modules\_ctypes\libffi_msvc\ffi.h
# End Source File
# Begin Source File

SOURCE=..\..\Modules\_ctypes\libffi_msvc\ffi_common.h
# End Source File
# Begin Source File

SOURCE=..\..\Modules\_ctypes\libffi_msvc\fficonfig.h
# End Source File
# Begin Source File

SOURCE=..\..\Modules\_ctypes\libffi_msvc\ffitarget.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
