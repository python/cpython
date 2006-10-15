# Microsoft Developer Studio Project File - Name="_elementtree" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=_elementtree - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "_elementtree.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "_elementtree.mak" CFG="_elementtree - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "_elementtree - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "_elementtree - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "_elementtree - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ""
# PROP Intermediate_Dir "x86-temp-release\_elementtree"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=df.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ELEMENTTREE_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "..\..\Include" /I "..\..\PC" /I "..\..\Modules\expat" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "XML_NS" /D "XML_DTD" /D BYTEORDER=1234 /D XML_CONTEXT_BYTES=1024 /D "USE_PYEXPAT_CAPI" /D "XML_STATIC" /D "HAVE_MEMMOVE" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"./_elementtree.pyd"

!ELSEIF  "$(CFG)" == "_elementtree - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "_elementtree___Win32_Debug"
# PROP BASE Intermediate_Dir "_elementtree___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir "x86-temp-debug\_elementtree"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=df.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ELEMENTTREE_EXPORTS" /YX /FD /GZ  /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\Include" /I "..\..\PC" /I "..\..\Modules\expat" /D "_DEBUG" /D "HAVE_EXPAT_H" /D "WIN32" /D "_WINDOWS" /D "XML_NS" /D "XML_DTD" /D BYTEORDER=1234 /D XML_CONTEXT_BYTES=1024 /D "USE_PYEXPAT_CAPI" /D "XML_STATIC" /D "HAVE_MEMMOVE" /YX /FD /GZ  /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"./_elementtree_d.pyd" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "_elementtree - Win32 Release"
# Name "_elementtree - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\Modules\_elementtree.c
# End Source File
# Begin Source File

SOURCE=..\..\Modules\expat\xmlparse.c
# End Source File
# Begin Source File

SOURCE=..\..\Modules\expat\xmlrole.c
# End Source File
# Begin Source File

SOURCE=..\..\Modules\expat\xmltok.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\Modules\expat\xmlrole.h
# End Source File
# Begin Source File

SOURCE=..\..\Modules\expat\xmltok.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
