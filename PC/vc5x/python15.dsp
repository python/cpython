# Microsoft Developer Studio Project File - Name="python15" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=python15 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "python15.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "python15.mak" CFG="python15 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "python15 - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "python15 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "python15 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release\temp"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Include" /I "..\PC" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "USE_DL_EXPORT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\Include" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib largeint.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"libc" /libpath:"Release"

!ELSEIF  "$(CFG)" == "python15 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug\temp"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\Include" /I "..\PC" /D "_DEBUG" /D "USE_DL_EXPORT" /D "WIN32" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\Include" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib largeint.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"libc" /pdbtype:sept /libpath:"Debug"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "python15 - Win32 Release"
# Name "python15 - Win32 Debug"
# Begin Source File

SOURCE=..\Objects\abstract.c
# End Source File
# Begin Source File

SOURCE=..\Parser\acceler.c
# End Source File
# Begin Source File

SOURCE=..\Modules\arraymodule.c
# End Source File
# Begin Source File

SOURCE=..\Modules\audioop.c
# End Source File
# Begin Source File

SOURCE=..\Modules\binascii.c
# End Source File
# Begin Source File

SOURCE=..\Parser\bitset.c
# End Source File
# Begin Source File

SOURCE=..\Python\bltinmodule.c
# End Source File
# Begin Source File

SOURCE=..\Python\ceval.c
# End Source File
# Begin Source File

SOURCE=..\Objects\classobject.c
# End Source File
# Begin Source File

SOURCE=..\Modules\cmathmodule.c
# End Source File
# Begin Source File

SOURCE=..\Objects\cobject.c
# End Source File
# Begin Source File

SOURCE=..\Python\compile.c
# End Source File
# Begin Source File

SOURCE=..\Objects\complexobject.c
# End Source File
# Begin Source File

SOURCE=..\PC\config.c
# End Source File
# Begin Source File

SOURCE=..\Modules\cPickle.c
# End Source File
# Begin Source File

SOURCE=..\Modules\cStringIO.c
# End Source File
# Begin Source File

SOURCE=..\Objects\dictobject.c
# End Source File
# Begin Source File

SOURCE=..\PC\dl_nt.c
# End Source File
# Begin Source File

SOURCE=..\Modules\errnomodule.c
# End Source File
# Begin Source File

SOURCE=..\Python\errors.c
# End Source File
# Begin Source File

SOURCE=..\Objects\fileobject.c
# End Source File
# Begin Source File

SOURCE=..\Objects\floatobject.c
# End Source File
# Begin Source File

SOURCE=..\Modules\fpectlmodule.c
# End Source File
# Begin Source File

SOURCE=..\Modules\fpetestmodule.c
# End Source File
# Begin Source File

SOURCE=..\Objects\frameobject.c
# End Source File
# Begin Source File

SOURCE=..\Python\frozen.c
# End Source File
# Begin Source File

SOURCE=..\Objects\funcobject.c
# End Source File
# Begin Source File

SOURCE=..\Python\getargs.c
# End Source File
# Begin Source File

SOURCE=..\Modules\getbuildinfo.c
# End Source File
# Begin Source File

SOURCE=..\Python\getcompiler.c
# End Source File
# Begin Source File

SOURCE=..\Python\getcopyright.c
# End Source File
# Begin Source File

SOURCE=..\Python\getmtime.c
# End Source File
# Begin Source File

SOURCE=..\Python\getopt.c
# End Source File
# Begin Source File

SOURCE=..\PC\getpathp.c
# End Source File
# Begin Source File

SOURCE=..\Python\getplatform.c
# End Source File
# Begin Source File

SOURCE=..\Python\getversion.c
# End Source File
# Begin Source File

SOURCE=..\Python\graminit.c
# End Source File
# Begin Source File

SOURCE=..\Parser\grammar1.c
# End Source File
# Begin Source File

SOURCE=..\Modules\imageop.c
# End Source File
# Begin Source File

SOURCE=..\Python\import.c
# End Source File
# Begin Source File

SOURCE=..\PC\import_nt.c
# ADD CPP /I "..\Python"
# End Source File
# Begin Source File

SOURCE=..\Python\importdl.c
# End Source File
# Begin Source File

SOURCE=..\Objects\intobject.c
# End Source File
# Begin Source File

SOURCE=..\Parser\listnode.c
# End Source File
# Begin Source File

SOURCE=..\Objects\listobject.c
# End Source File
# Begin Source File

SOURCE=..\Objects\longobject.c
# End Source File
# Begin Source File

SOURCE=..\Modules\main.c
# End Source File
# Begin Source File

SOURCE=..\Python\marshal.c
# End Source File
# Begin Source File

SOURCE=..\Modules\mathmodule.c
# End Source File
# Begin Source File

SOURCE=..\Modules\md5c.c
# End Source File
# Begin Source File

SOURCE=..\Modules\md5module.c
# End Source File
# Begin Source File

SOURCE=..\Parser\metagrammar.c
# End Source File
# Begin Source File

SOURCE=..\Objects\methodobject.c
# End Source File
# Begin Source File

SOURCE=..\Python\modsupport.c
# End Source File
# Begin Source File

SOURCE=..\Objects\moduleobject.c
# End Source File
# Begin Source File

SOURCE=..\PC\msvcrtmodule.c
# End Source File
# Begin Source File

SOURCE=..\Parser\myreadline.c
# End Source File
# Begin Source File

SOURCE=..\Python\mystrtoul.c
# End Source File
# Begin Source File

SOURCE=..\Modules\newmodule.c
# End Source File
# Begin Source File

SOURCE=..\Parser\node.c
# End Source File
# Begin Source File

SOURCE=..\Objects\object.c
# End Source File
# Begin Source File

SOURCE=..\Modules\operator.c
# End Source File
# Begin Source File

SOURCE=..\Parser\parser.c
# End Source File
# Begin Source File

SOURCE=..\Parser\parsetok.c
# End Source File
# Begin Source File

SOURCE=..\Modules\posixmodule.c
# End Source File
# Begin Source File

SOURCE=..\Python\pyfpe.c
# End Source File
# Begin Source File

SOURCE=..\Python\pystate.c
# End Source File
# Begin Source File

SOURCE=..\PC\python_nt.def
# End Source File
# Begin Source File

SOURCE=..\PC\python_nt.rc
# End Source File
# Begin Source File

SOURCE=..\Python\pythonrun.c
# End Source File
# Begin Source File

SOURCE=..\Objects\rangeobject.c
# End Source File
# Begin Source File

SOURCE=..\Modules\regexmodule.c
# End Source File
# Begin Source File

SOURCE=..\Modules\regexpr.c
# End Source File
# Begin Source File

SOURCE=..\Modules\reopmodule.c
# End Source File
# Begin Source File

SOURCE=..\Modules\rgbimgmodule.c
# End Source File
# Begin Source File

SOURCE=..\Modules\rotormodule.c
# End Source File
# Begin Source File

SOURCE=..\Modules\selectmodule.c
# End Source File
# Begin Source File

SOURCE=..\Modules\signalmodule.c
# End Source File
# Begin Source File

SOURCE=..\Objects\sliceobject.c
# End Source File
# Begin Source File

SOURCE=..\Modules\socketmodule.c
# End Source File
# Begin Source File

SOURCE=..\Modules\soundex.c
# End Source File
# Begin Source File

SOURCE=..\Objects\stringobject.c
# End Source File
# Begin Source File

SOURCE=..\Modules\stropmodule.c
# End Source File
# Begin Source File

SOURCE=..\Python\structmember.c
# End Source File
# Begin Source File

SOURCE=..\Modules\structmodule.c
# End Source File
# Begin Source File

SOURCE=..\Python\sysmodule.c
# End Source File
# Begin Source File

SOURCE=..\Python\thread.c
# End Source File
# Begin Source File

SOURCE=..\Modules\threadmodule.c
# End Source File
# Begin Source File

SOURCE=..\Modules\timemodule.c
# End Source File
# Begin Source File

SOURCE=..\Parser\tokenizer.c
# End Source File
# Begin Source File

SOURCE=..\Python\traceback.c
# End Source File
# Begin Source File

SOURCE=..\Objects\tupleobject.c
# End Source File
# Begin Source File

SOURCE=..\Objects\typeobject.c
# End Source File
# Begin Source File

SOURCE=..\Modules\yuvconvert.c
# End Source File
# End Target
# End Project
