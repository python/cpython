# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103
# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=vc40_nt - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to vc40_nt - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "vc40 - Win32 Release" && "$(CFG)" != "vc40 - Win32 Debug" &&\
 "$(CFG)" != "vc40_dll - Win32 Release" && "$(CFG)" != "vc40_dll - Win32 Debug"\
 && "$(CFG)" != "vc40_nt - Win32 Release" && "$(CFG)" != "vc40_nt - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "vc40.mak" CFG="vc40_nt - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vc40 - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "vc40 - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "vc40_dll - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vc40_dll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vc40_nt - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "vc40_nt - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "vc40_nt - Win32 Debug"

!IF  "$(CFG)" == "vc40 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "pc\Release"
# PROP Intermediate_Dir "pc\Release"
# PROP Target_Dir ""
OUTDIR=.\pc\Release
INTDIR=.\pc\Release

ALL : 

CLEAN : 
	-@erase 

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "./PC" /I "./Include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "HAVE_CONFIG_H" /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "./PC" /I "./Include" /D "NDEBUG" /D\
 "WIN32" /D "_CONSOLE" /D "HAVE_CONFIG_H" /Fp"$(INTDIR)/vc40.pch" /YX\
 /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\pc\Release/
CPP_SBRS=

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/vc40.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
LINK32_FLAGS=wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\
 odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/vc40.pdb" /machine:I386 /out:"$(OUTDIR)/vc40.exe" 
LINK32_OBJS=

!ELSEIF  "$(CFG)" == "vc40 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "pc\Debug"
# PROP Intermediate_Dir "pc\Debug"
# PROP Target_Dir ""
OUTDIR=.\pc\Debug
INTDIR=.\pc\Debug

ALL : 

CLEAN : 
	-@erase 

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "./PC" /I "./Include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "HAVE_CONFIG_H" /YX /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "./PC" /I "./Include" /D "_DEBUG"\
 /D "WIN32" /D "_CONSOLE" /D "HAVE_CONFIG_H" /Fp"$(INTDIR)/vc40.pch" /YX\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\pc\Debug/
CPP_SBRS=

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/vc40.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\
 odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/vc40.pdb" /debug /machine:I386 /out:"$(OUTDIR)/vc40.exe" 
LINK32_OBJS=

!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vc40_dll\Release"
# PROP BASE Intermediate_Dir "vc40_dll\Release"
# PROP BASE Target_Dir "vc40_dll"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vc40_dll\Release"
# PROP Intermediate_Dir "vc40_dll\Release"
# PROP Target_Dir "vc40_dll"
OUTDIR=.\vc40_dll\Release
INTDIR=.\vc40_dll\Release

ALL : "$(OUTDIR)\py14an.dll"

CLEAN : 
	-@erase ".\vc40_dll\Release\py14an.dll"
	-@erase ".\vc40_dll\Release\longobject.obj"
	-@erase ".\vc40_dll\Release\listobject.obj"
	-@erase ".\vc40_dll\Release\intobject.obj"
	-@erase ".\vc40_dll\Release\importdl.obj"
	-@erase ".\vc40_dll\Release\import.obj"
	-@erase ".\vc40_dll\Release\imageop.obj"
	-@erase ".\vc40_dll\Release\grammar1.obj"
	-@erase ".\vc40_dll\Release\graminit.obj"
	-@erase ".\vc40_dll\Release\getversion.obj"
	-@erase ".\vc40_dll\Release\getplatform.obj"
	-@erase ".\vc40_dll\Release\getpath.obj"
	-@erase ".\vc40_dll\Release\getmtime.obj"
	-@erase ".\vc40_dll\Release\getcopyright.obj"
	-@erase ".\vc40_dll\Release\getcompiler.obj"
	-@erase ".\vc40_dll\Release\getargs.obj"
	-@erase ".\vc40_dll\Release\funcobject.obj"
	-@erase ".\vc40_dll\Release\frozen.obj"
	-@erase ".\vc40_dll\Release\frameobject.obj"
	-@erase ".\vc40_dll\Release\floatobject.obj"
	-@erase ".\vc40_dll\Release\fileobject.obj"
	-@erase ".\vc40_dll\Release\errors.obj"
	-@erase ".\vc40_dll\Release\environment.obj"
	-@erase ".\vc40_dll\Release\config.obj"
	-@erase ".\vc40_dll\Release\complexobject.obj"
	-@erase ".\vc40_dll\Release\compile.obj"
	-@erase ".\vc40_dll\Release\cobject.obj"
	-@erase ".\vc40_dll\Release\cmathmodule.obj"
	-@erase ".\vc40_dll\Release\classobject.obj"
	-@erase ".\vc40_dll\Release\cgensupport.obj"
	-@erase ".\vc40_dll\Release\ceval.obj"
	-@erase ".\vc40_dll\Release\bltinmodule.obj"
	-@erase ".\vc40_dll\Release\binascii.obj"
	-@erase ".\vc40_dll\Release\audioop.obj"
	-@erase ".\vc40_dll\Release\arraymodule.obj"
	-@erase ".\vc40_dll\Release\accessobject.obj"
	-@erase ".\vc40_dll\Release\acceler.obj"
	-@erase ".\vc40_dll\Release\abstract.obj"
	-@erase ".\vc40_dll\Release\yuvconvert.obj"
	-@erase ".\vc40_dll\Release\typeobject.obj"
	-@erase ".\vc40_dll\Release\tupleobject.obj"
	-@erase ".\vc40_dll\Release\traceback.obj"
	-@erase ".\vc40_dll\Release\tokenizer.obj"
	-@erase ".\vc40_dll\Release\timemodule.obj"
	-@erase ".\vc40_dll\Release\threadmodule.obj"
	-@erase ".\vc40_dll\Release\thread.obj"
	-@erase ".\vc40_dll\Release\sysmodule.obj"
	-@erase ".\vc40_dll\Release\structmodule.obj"
	-@erase ".\vc40_dll\Release\structmember.obj"
	-@erase ".\vc40_dll\Release\stropmodule.obj"
	-@erase ".\vc40_dll\Release\stringobject.obj"
	-@erase ".\vc40_dll\Release\soundex.obj"
	-@erase ".\vc40_dll\Release\socketmodule.obj"
	-@erase ".\vc40_dll\Release\signalmodule.obj"
	-@erase ".\vc40_dll\Release\selectmodule.obj"
	-@erase ".\vc40_dll\Release\rotormodule.obj"
	-@erase ".\vc40_dll\Release\rgbimgmodule.obj"
	-@erase ".\vc40_dll\Release\regexpr.obj"
	-@erase ".\vc40_dll\Release\regexmodule.obj"
	-@erase ".\vc40_dll\Release\rangeobject.obj"
	-@erase ".\vc40_dll\Release\pythonrun.obj"
	-@erase ".\vc40_dll\Release\posixmodule.obj"
	-@erase ".\vc40_dll\Release\parsetok.obj"
	-@erase ".\vc40_dll\Release\parser.obj"
	-@erase ".\vc40_dll\Release\object.obj"
	-@erase ".\vc40_dll\Release\node.obj"
	-@erase ".\vc40_dll\Release\newmodule.obj"
	-@erase ".\vc40_dll\Release\marshal.obj"
	-@erase ".\vc40_dll\Release\mystrtoul.obj"
	-@erase ".\vc40_dll\Release\myreadline.obj"
	-@erase ".\vc40_dll\Release\moduleobject.obj"
	-@erase ".\vc40_dll\Release\modsupport.obj"
	-@erase ".\vc40_dll\Release\methodobject.obj"
	-@erase ".\vc40_dll\Release\md5module.obj"
	-@erase ".\vc40_dll\Release\md5c.obj"
	-@erase ".\vc40_dll\Release\mathmodule.obj"
	-@erase ".\vc40_dll\Release\mappingobject.obj"
	-@erase ".\vc40_dll\Release\py14an.lib"
	-@erase ".\vc40_dll\Release\py14an.exp"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "./PC" /I "./Include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /YX /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "./PC" /I "./Include" /D "NDEBUG" /D\
 "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /Fp"$(INTDIR)/vc40_dll.pch" /YX\
 /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\vc40_dll\Release/
CPP_SBRS=

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/vc40_dll.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"vc40_dll\Release/py14an.dll"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib wsock32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/py14an.pdb" /machine:I386 /def:".\PC\python.def"\
 /out:"$(OUTDIR)/py14an.dll" /implib:"$(OUTDIR)/py14an.lib" 
DEF_FILE= \
	".\PC\python.def"
LINK32_OBJS= \
	".\vc40_dll\Release\longobject.obj" \
	".\vc40_dll\Release\listobject.obj" \
	".\vc40_dll\Release\intobject.obj" \
	".\vc40_dll\Release\importdl.obj" \
	".\vc40_dll\Release\import.obj" \
	".\vc40_dll\Release\imageop.obj" \
	".\vc40_dll\Release\grammar1.obj" \
	".\vc40_dll\Release\graminit.obj" \
	".\vc40_dll\Release\getversion.obj" \
	".\vc40_dll\Release\getplatform.obj" \
	".\vc40_dll\Release\getpath.obj" \
	".\vc40_dll\Release\getmtime.obj" \
	".\vc40_dll\Release\getcopyright.obj" \
	".\vc40_dll\Release\getcompiler.obj" \
	".\vc40_dll\Release\getargs.obj" \
	".\vc40_dll\Release\funcobject.obj" \
	".\vc40_dll\Release\frozen.obj" \
	".\vc40_dll\Release\frameobject.obj" \
	".\vc40_dll\Release\floatobject.obj" \
	".\vc40_dll\Release\fileobject.obj" \
	".\vc40_dll\Release\errors.obj" \
	".\vc40_dll\Release\environment.obj" \
	".\vc40_dll\Release\config.obj" \
	".\vc40_dll\Release\complexobject.obj" \
	".\vc40_dll\Release\compile.obj" \
	".\vc40_dll\Release\cobject.obj" \
	".\vc40_dll\Release\cmathmodule.obj" \
	".\vc40_dll\Release\classobject.obj" \
	".\vc40_dll\Release\cgensupport.obj" \
	".\vc40_dll\Release\ceval.obj" \
	".\vc40_dll\Release\bltinmodule.obj" \
	".\vc40_dll\Release\binascii.obj" \
	".\vc40_dll\Release\audioop.obj" \
	".\vc40_dll\Release\arraymodule.obj" \
	".\vc40_dll\Release\accessobject.obj" \
	".\vc40_dll\Release\acceler.obj" \
	".\vc40_dll\Release\abstract.obj" \
	".\vc40_dll\Release\yuvconvert.obj" \
	".\vc40_dll\Release\typeobject.obj" \
	".\vc40_dll\Release\tupleobject.obj" \
	".\vc40_dll\Release\traceback.obj" \
	".\vc40_dll\Release\tokenizer.obj" \
	".\vc40_dll\Release\timemodule.obj" \
	".\vc40_dll\Release\threadmodule.obj" \
	".\vc40_dll\Release\thread.obj" \
	".\vc40_dll\Release\sysmodule.obj" \
	".\vc40_dll\Release\structmodule.obj" \
	".\vc40_dll\Release\structmember.obj" \
	".\vc40_dll\Release\stropmodule.obj" \
	".\vc40_dll\Release\stringobject.obj" \
	".\vc40_dll\Release\soundex.obj" \
	".\vc40_dll\Release\socketmodule.obj" \
	".\vc40_dll\Release\signalmodule.obj" \
	".\vc40_dll\Release\selectmodule.obj" \
	".\vc40_dll\Release\rotormodule.obj" \
	".\vc40_dll\Release\rgbimgmodule.obj" \
	".\vc40_dll\Release\regexpr.obj" \
	".\vc40_dll\Release\regexmodule.obj" \
	".\vc40_dll\Release\rangeobject.obj" \
	".\vc40_dll\Release\pythonrun.obj" \
	".\vc40_dll\Release\posixmodule.obj" \
	".\vc40_dll\Release\parsetok.obj" \
	".\vc40_dll\Release\parser.obj" \
	".\vc40_dll\Release\object.obj" \
	".\vc40_dll\Release\node.obj" \
	".\vc40_dll\Release\newmodule.obj" \
	".\vc40_dll\Release\marshal.obj" \
	".\vc40_dll\Release\mystrtoul.obj" \
	".\vc40_dll\Release\myreadline.obj" \
	".\vc40_dll\Release\moduleobject.obj" \
	".\vc40_dll\Release\modsupport.obj" \
	".\vc40_dll\Release\methodobject.obj" \
	".\vc40_dll\Release\md5module.obj" \
	".\vc40_dll\Release\md5c.obj" \
	".\vc40_dll\Release\mathmodule.obj" \
	".\vc40_dll\Release\mappingobject.obj"

"$(OUTDIR)\py14an.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vc40_dll\Debug"
# PROP BASE Intermediate_Dir "vc40_dll\Debug"
# PROP BASE Target_Dir "vc40_dll"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vc40_dll\Debug"
# PROP Intermediate_Dir "vc40_dll\Debug"
# PROP Target_Dir "vc40_dll"
OUTDIR=.\vc40_dll\Debug
INTDIR=.\vc40_dll\Debug

ALL : "$(OUTDIR)\py14an.dll"

CLEAN : 
	-@erase ".\vc40_dll\Debug\vc40.pdb"
	-@erase ".\vc40_dll\Debug\vc40.idb"
	-@erase ".\vc40_dll\Debug\py14an.dll"
	-@erase ".\vc40_dll\Debug\longobject.obj"
	-@erase ".\vc40_dll\Debug\listobject.obj"
	-@erase ".\vc40_dll\Debug\intobject.obj"
	-@erase ".\vc40_dll\Debug\importdl.obj"
	-@erase ".\vc40_dll\Debug\import.obj"
	-@erase ".\vc40_dll\Debug\imageop.obj"
	-@erase ".\vc40_dll\Debug\grammar1.obj"
	-@erase ".\vc40_dll\Debug\graminit.obj"
	-@erase ".\vc40_dll\Debug\getversion.obj"
	-@erase ".\vc40_dll\Debug\getplatform.obj"
	-@erase ".\vc40_dll\Debug\getpath.obj"
	-@erase ".\vc40_dll\Debug\getmtime.obj"
	-@erase ".\vc40_dll\Debug\getcopyright.obj"
	-@erase ".\vc40_dll\Debug\getcompiler.obj"
	-@erase ".\vc40_dll\Debug\getargs.obj"
	-@erase ".\vc40_dll\Debug\funcobject.obj"
	-@erase ".\vc40_dll\Debug\frozen.obj"
	-@erase ".\vc40_dll\Debug\frameobject.obj"
	-@erase ".\vc40_dll\Debug\floatobject.obj"
	-@erase ".\vc40_dll\Debug\fileobject.obj"
	-@erase ".\vc40_dll\Debug\errors.obj"
	-@erase ".\vc40_dll\Debug\environment.obj"
	-@erase ".\vc40_dll\Debug\config.obj"
	-@erase ".\vc40_dll\Debug\complexobject.obj"
	-@erase ".\vc40_dll\Debug\compile.obj"
	-@erase ".\vc40_dll\Debug\cobject.obj"
	-@erase ".\vc40_dll\Debug\cmathmodule.obj"
	-@erase ".\vc40_dll\Debug\classobject.obj"
	-@erase ".\vc40_dll\Debug\cgensupport.obj"
	-@erase ".\vc40_dll\Debug\ceval.obj"
	-@erase ".\vc40_dll\Debug\bltinmodule.obj"
	-@erase ".\vc40_dll\Debug\binascii.obj"
	-@erase ".\vc40_dll\Debug\audioop.obj"
	-@erase ".\vc40_dll\Debug\arraymodule.obj"
	-@erase ".\vc40_dll\Debug\accessobject.obj"
	-@erase ".\vc40_dll\Debug\acceler.obj"
	-@erase ".\vc40_dll\Debug\abstract.obj"
	-@erase ".\vc40_dll\Debug\yuvconvert.obj"
	-@erase ".\vc40_dll\Debug\typeobject.obj"
	-@erase ".\vc40_dll\Debug\tupleobject.obj"
	-@erase ".\vc40_dll\Debug\traceback.obj"
	-@erase ".\vc40_dll\Debug\tokenizer.obj"
	-@erase ".\vc40_dll\Debug\timemodule.obj"
	-@erase ".\vc40_dll\Debug\threadmodule.obj"
	-@erase ".\vc40_dll\Debug\thread.obj"
	-@erase ".\vc40_dll\Debug\sysmodule.obj"
	-@erase ".\vc40_dll\Debug\structmodule.obj"
	-@erase ".\vc40_dll\Debug\structmember.obj"
	-@erase ".\vc40_dll\Debug\stropmodule.obj"
	-@erase ".\vc40_dll\Debug\stringobject.obj"
	-@erase ".\vc40_dll\Debug\soundex.obj"
	-@erase ".\vc40_dll\Debug\socketmodule.obj"
	-@erase ".\vc40_dll\Debug\signalmodule.obj"
	-@erase ".\vc40_dll\Debug\selectmodule.obj"
	-@erase ".\vc40_dll\Debug\rotormodule.obj"
	-@erase ".\vc40_dll\Debug\rgbimgmodule.obj"
	-@erase ".\vc40_dll\Debug\regexpr.obj"
	-@erase ".\vc40_dll\Debug\regexmodule.obj"
	-@erase ".\vc40_dll\Debug\rangeobject.obj"
	-@erase ".\vc40_dll\Debug\pythonrun.obj"
	-@erase ".\vc40_dll\Debug\posixmodule.obj"
	-@erase ".\vc40_dll\Debug\parsetok.obj"
	-@erase ".\vc40_dll\Debug\parser.obj"
	-@erase ".\vc40_dll\Debug\object.obj"
	-@erase ".\vc40_dll\Debug\node.obj"
	-@erase ".\vc40_dll\Debug\newmodule.obj"
	-@erase ".\vc40_dll\Debug\marshal.obj"
	-@erase ".\vc40_dll\Debug\mystrtoul.obj"
	-@erase ".\vc40_dll\Debug\myreadline.obj"
	-@erase ".\vc40_dll\Debug\moduleobject.obj"
	-@erase ".\vc40_dll\Debug\modsupport.obj"
	-@erase ".\vc40_dll\Debug\methodobject.obj"
	-@erase ".\vc40_dll\Debug\md5module.obj"
	-@erase ".\vc40_dll\Debug\md5c.obj"
	-@erase ".\vc40_dll\Debug\mathmodule.obj"
	-@erase ".\vc40_dll\Debug\mappingobject.obj"
	-@erase ".\vc40_dll\Debug\py14an.ilk"
	-@erase ".\vc40_dll\Debug\py14an.lib"
	-@erase ".\vc40_dll\Debug\py14an.exp"
	-@erase ".\vc40_dll\Debug\py14an.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "./PC" /I "./Include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /YX /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I "./PC" /I "./Include" /D "_DEBUG"\
 /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /Fp"$(INTDIR)/vc40_dll.pch" /YX\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\vc40_dll\Debug/
CPP_SBRS=

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/vc40_dll.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"vc40_dll\Debug/py14an.dll"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib wsock32.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/py14an.pdb" /debug /machine:I386 /def:".\PC\python.def"\
 /out:"$(OUTDIR)/py14an.dll" /implib:"$(OUTDIR)/py14an.lib" 
DEF_FILE= \
	".\PC\python.def"
LINK32_OBJS= \
	".\vc40_dll\Debug\longobject.obj" \
	".\vc40_dll\Debug\listobject.obj" \
	".\vc40_dll\Debug\intobject.obj" \
	".\vc40_dll\Debug\importdl.obj" \
	".\vc40_dll\Debug\import.obj" \
	".\vc40_dll\Debug\imageop.obj" \
	".\vc40_dll\Debug\grammar1.obj" \
	".\vc40_dll\Debug\graminit.obj" \
	".\vc40_dll\Debug\getversion.obj" \
	".\vc40_dll\Debug\getplatform.obj" \
	".\vc40_dll\Debug\getpath.obj" \
	".\vc40_dll\Debug\getmtime.obj" \
	".\vc40_dll\Debug\getcopyright.obj" \
	".\vc40_dll\Debug\getcompiler.obj" \
	".\vc40_dll\Debug\getargs.obj" \
	".\vc40_dll\Debug\funcobject.obj" \
	".\vc40_dll\Debug\frozen.obj" \
	".\vc40_dll\Debug\frameobject.obj" \
	".\vc40_dll\Debug\floatobject.obj" \
	".\vc40_dll\Debug\fileobject.obj" \
	".\vc40_dll\Debug\errors.obj" \
	".\vc40_dll\Debug\environment.obj" \
	".\vc40_dll\Debug\config.obj" \
	".\vc40_dll\Debug\complexobject.obj" \
	".\vc40_dll\Debug\compile.obj" \
	".\vc40_dll\Debug\cobject.obj" \
	".\vc40_dll\Debug\cmathmodule.obj" \
	".\vc40_dll\Debug\classobject.obj" \
	".\vc40_dll\Debug\cgensupport.obj" \
	".\vc40_dll\Debug\ceval.obj" \
	".\vc40_dll\Debug\bltinmodule.obj" \
	".\vc40_dll\Debug\binascii.obj" \
	".\vc40_dll\Debug\audioop.obj" \
	".\vc40_dll\Debug\arraymodule.obj" \
	".\vc40_dll\Debug\accessobject.obj" \
	".\vc40_dll\Debug\acceler.obj" \
	".\vc40_dll\Debug\abstract.obj" \
	".\vc40_dll\Debug\yuvconvert.obj" \
	".\vc40_dll\Debug\typeobject.obj" \
	".\vc40_dll\Debug\tupleobject.obj" \
	".\vc40_dll\Debug\traceback.obj" \
	".\vc40_dll\Debug\tokenizer.obj" \
	".\vc40_dll\Debug\timemodule.obj" \
	".\vc40_dll\Debug\threadmodule.obj" \
	".\vc40_dll\Debug\thread.obj" \
	".\vc40_dll\Debug\sysmodule.obj" \
	".\vc40_dll\Debug\structmodule.obj" \
	".\vc40_dll\Debug\structmember.obj" \
	".\vc40_dll\Debug\stropmodule.obj" \
	".\vc40_dll\Debug\stringobject.obj" \
	".\vc40_dll\Debug\soundex.obj" \
	".\vc40_dll\Debug\socketmodule.obj" \
	".\vc40_dll\Debug\signalmodule.obj" \
	".\vc40_dll\Debug\selectmodule.obj" \
	".\vc40_dll\Debug\rotormodule.obj" \
	".\vc40_dll\Debug\rgbimgmodule.obj" \
	".\vc40_dll\Debug\regexpr.obj" \
	".\vc40_dll\Debug\regexmodule.obj" \
	".\vc40_dll\Debug\rangeobject.obj" \
	".\vc40_dll\Debug\pythonrun.obj" \
	".\vc40_dll\Debug\posixmodule.obj" \
	".\vc40_dll\Debug\parsetok.obj" \
	".\vc40_dll\Debug\parser.obj" \
	".\vc40_dll\Debug\object.obj" \
	".\vc40_dll\Debug\node.obj" \
	".\vc40_dll\Debug\newmodule.obj" \
	".\vc40_dll\Debug\marshal.obj" \
	".\vc40_dll\Debug\mystrtoul.obj" \
	".\vc40_dll\Debug\myreadline.obj" \
	".\vc40_dll\Debug\moduleobject.obj" \
	".\vc40_dll\Debug\modsupport.obj" \
	".\vc40_dll\Debug\methodobject.obj" \
	".\vc40_dll\Debug\md5module.obj" \
	".\vc40_dll\Debug\md5c.obj" \
	".\vc40_dll\Debug\mathmodule.obj" \
	".\vc40_dll\Debug\mappingobject.obj"

"$(OUTDIR)\py14an.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vc40_nt\Release"
# PROP BASE Intermediate_Dir "vc40_nt\Release"
# PROP BASE Target_Dir "vc40_nt"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vc40_nt\Release"
# PROP Intermediate_Dir "vc40_nt\Release"
# PROP Target_Dir "vc40_nt"
OUTDIR=.\vc40_nt\Release
INTDIR=.\vc40_nt\Release

ALL : "$(OUTDIR)\pyth_nt.exe"

CLEAN : 
	-@erase ".\vc40_nt\Release\pyth_nt.exe"
	-@erase ".\vc40_nt\Release\longobject.obj"
	-@erase ".\vc40_nt\Release\listobject.obj"
	-@erase ".\vc40_nt\Release\intobject.obj"
	-@erase ".\vc40_nt\Release\importdl.obj"
	-@erase ".\vc40_nt\Release\import.obj"
	-@erase ".\vc40_nt\Release\imageop.obj"
	-@erase ".\vc40_nt\Release\grammar1.obj"
	-@erase ".\vc40_nt\Release\graminit.obj"
	-@erase ".\vc40_nt\Release\getversion.obj"
	-@erase ".\vc40_nt\Release\getplatform.obj"
	-@erase ".\vc40_nt\Release\getpath.obj"
	-@erase ".\vc40_nt\Release\getmtime.obj"
	-@erase ".\vc40_nt\Release\getcopyright.obj"
	-@erase ".\vc40_nt\Release\getcompiler.obj"
	-@erase ".\vc40_nt\Release\getargs.obj"
	-@erase ".\vc40_nt\Release\funcobject.obj"
	-@erase ".\vc40_nt\Release\frozen.obj"
	-@erase ".\vc40_nt\Release\frameobject.obj"
	-@erase ".\vc40_nt\Release\floatobject.obj"
	-@erase ".\vc40_nt\Release\fileobject.obj"
	-@erase ".\vc40_nt\Release\errors.obj"
	-@erase ".\vc40_nt\Release\environment.obj"
	-@erase ".\vc40_nt\Release\config.obj"
	-@erase ".\vc40_nt\Release\complexobject.obj"
	-@erase ".\vc40_nt\Release\compile.obj"
	-@erase ".\vc40_nt\Release\cobject.obj"
	-@erase ".\vc40_nt\Release\cmathmodule.obj"
	-@erase ".\vc40_nt\Release\classobject.obj"
	-@erase ".\vc40_nt\Release\cgensupport.obj"
	-@erase ".\vc40_nt\Release\ceval.obj"
	-@erase ".\vc40_nt\Release\bltinmodule.obj"
	-@erase ".\vc40_nt\Release\binascii.obj"
	-@erase ".\vc40_nt\Release\audioop.obj"
	-@erase ".\vc40_nt\Release\arraymodule.obj"
	-@erase ".\vc40_nt\Release\accessobject.obj"
	-@erase ".\vc40_nt\Release\acceler.obj"
	-@erase ".\vc40_nt\Release\abstract.obj"
	-@erase ".\vc40_nt\Release\yuvconvert.obj"
	-@erase ".\vc40_nt\Release\typeobject.obj"
	-@erase ".\vc40_nt\Release\tupleobject.obj"
	-@erase ".\vc40_nt\Release\traceback.obj"
	-@erase ".\vc40_nt\Release\tokenizer.obj"
	-@erase ".\vc40_nt\Release\timemodule.obj"
	-@erase ".\vc40_nt\Release\threadmodule.obj"
	-@erase ".\vc40_nt\Release\thread.obj"
	-@erase ".\vc40_nt\Release\sysmodule.obj"
	-@erase ".\vc40_nt\Release\structmodule.obj"
	-@erase ".\vc40_nt\Release\structmember.obj"
	-@erase ".\vc40_nt\Release\stropmodule.obj"
	-@erase ".\vc40_nt\Release\stringobject.obj"
	-@erase ".\vc40_nt\Release\soundex.obj"
	-@erase ".\vc40_nt\Release\socketmodule.obj"
	-@erase ".\vc40_nt\Release\signalmodule.obj"
	-@erase ".\vc40_nt\Release\selectmodule.obj"
	-@erase ".\vc40_nt\Release\rotormodule.obj"
	-@erase ".\vc40_nt\Release\rgbimgmodule.obj"
	-@erase ".\vc40_nt\Release\regexpr.obj"
	-@erase ".\vc40_nt\Release\regexmodule.obj"
	-@erase ".\vc40_nt\Release\rangeobject.obj"
	-@erase ".\vc40_nt\Release\pythonrun.obj"
	-@erase ".\vc40_nt\Release\posixmodule.obj"
	-@erase ".\vc40_nt\Release\parsetok.obj"
	-@erase ".\vc40_nt\Release\parser.obj"
	-@erase ".\vc40_nt\Release\object.obj"
	-@erase ".\vc40_nt\Release\node.obj"
	-@erase ".\vc40_nt\Release\newmodule.obj"
	-@erase ".\vc40_nt\Release\mystrtoul.obj"
	-@erase ".\vc40_nt\Release\myreadline.obj"
	-@erase ".\vc40_nt\Release\moduleobject.obj"
	-@erase ".\vc40_nt\Release\modsupport.obj"
	-@erase ".\vc40_nt\Release\methodobject.obj"
	-@erase ".\vc40_nt\Release\md5module.obj"
	-@erase ".\vc40_nt\Release\md5c.obj"
	-@erase ".\vc40_nt\Release\mathmodule.obj"
	-@erase ".\vc40_nt\Release\marshal.obj"
	-@erase ".\vc40_nt\Release\mappingobject.obj"
	-@erase ".\vc40_nt\Release\main.obj"
	-@erase ".\vc40_nt\Release\getopt.obj"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "./PC" /I "./Include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "HAVE_CONFIG_H" /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "./PC" /I "./Include" /D "NDEBUG" /D\
 "WIN32" /D "_CONSOLE" /D "HAVE_CONFIG_H" /Fp"$(INTDIR)/vc40_nt.pch" /YX\
 /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\vc40_nt\Release/
CPP_SBRS=

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/vc40_nt.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:console /machine:I386 /out:"vc40_nt\Release/pyth_nt.exe"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib wsock32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/pyth_nt.pdb" /machine:I386 /out:"$(OUTDIR)/pyth_nt.exe" 
LINK32_OBJS= \
	".\vc40_nt\Release\longobject.obj" \
	".\vc40_nt\Release\listobject.obj" \
	".\vc40_nt\Release\intobject.obj" \
	".\vc40_nt\Release\importdl.obj" \
	".\vc40_nt\Release\import.obj" \
	".\vc40_nt\Release\imageop.obj" \
	".\vc40_nt\Release\grammar1.obj" \
	".\vc40_nt\Release\graminit.obj" \
	".\vc40_nt\Release\getversion.obj" \
	".\vc40_nt\Release\getplatform.obj" \
	".\vc40_nt\Release\getpath.obj" \
	".\vc40_nt\Release\getmtime.obj" \
	".\vc40_nt\Release\getcopyright.obj" \
	".\vc40_nt\Release\getcompiler.obj" \
	".\vc40_nt\Release\getargs.obj" \
	".\vc40_nt\Release\funcobject.obj" \
	".\vc40_nt\Release\frozen.obj" \
	".\vc40_nt\Release\frameobject.obj" \
	".\vc40_nt\Release\floatobject.obj" \
	".\vc40_nt\Release\fileobject.obj" \
	".\vc40_nt\Release\errors.obj" \
	".\vc40_nt\Release\environment.obj" \
	".\vc40_nt\Release\config.obj" \
	".\vc40_nt\Release\complexobject.obj" \
	".\vc40_nt\Release\compile.obj" \
	".\vc40_nt\Release\cobject.obj" \
	".\vc40_nt\Release\cmathmodule.obj" \
	".\vc40_nt\Release\classobject.obj" \
	".\vc40_nt\Release\cgensupport.obj" \
	".\vc40_nt\Release\ceval.obj" \
	".\vc40_nt\Release\bltinmodule.obj" \
	".\vc40_nt\Release\binascii.obj" \
	".\vc40_nt\Release\audioop.obj" \
	".\vc40_nt\Release\arraymodule.obj" \
	".\vc40_nt\Release\accessobject.obj" \
	".\vc40_nt\Release\acceler.obj" \
	".\vc40_nt\Release\abstract.obj" \
	".\vc40_nt\Release\yuvconvert.obj" \
	".\vc40_nt\Release\typeobject.obj" \
	".\vc40_nt\Release\tupleobject.obj" \
	".\vc40_nt\Release\traceback.obj" \
	".\vc40_nt\Release\tokenizer.obj" \
	".\vc40_nt\Release\timemodule.obj" \
	".\vc40_nt\Release\threadmodule.obj" \
	".\vc40_nt\Release\thread.obj" \
	".\vc40_nt\Release\sysmodule.obj" \
	".\vc40_nt\Release\structmodule.obj" \
	".\vc40_nt\Release\structmember.obj" \
	".\vc40_nt\Release\stropmodule.obj" \
	".\vc40_nt\Release\stringobject.obj" \
	".\vc40_nt\Release\soundex.obj" \
	".\vc40_nt\Release\socketmodule.obj" \
	".\vc40_nt\Release\signalmodule.obj" \
	".\vc40_nt\Release\selectmodule.obj" \
	".\vc40_nt\Release\rotormodule.obj" \
	".\vc40_nt\Release\rgbimgmodule.obj" \
	".\vc40_nt\Release\regexpr.obj" \
	".\vc40_nt\Release\regexmodule.obj" \
	".\vc40_nt\Release\rangeobject.obj" \
	".\vc40_nt\Release\pythonrun.obj" \
	".\vc40_nt\Release\posixmodule.obj" \
	".\vc40_nt\Release\parsetok.obj" \
	".\vc40_nt\Release\parser.obj" \
	".\vc40_nt\Release\object.obj" \
	".\vc40_nt\Release\node.obj" \
	".\vc40_nt\Release\newmodule.obj" \
	".\vc40_nt\Release\mystrtoul.obj" \
	".\vc40_nt\Release\myreadline.obj" \
	".\vc40_nt\Release\moduleobject.obj" \
	".\vc40_nt\Release\modsupport.obj" \
	".\vc40_nt\Release\methodobject.obj" \
	".\vc40_nt\Release\md5module.obj" \
	".\vc40_nt\Release\md5c.obj" \
	".\vc40_nt\Release\mathmodule.obj" \
	".\vc40_nt\Release\marshal.obj" \
	".\vc40_nt\Release\mappingobject.obj" \
	".\vc40_nt\Release\main.obj" \
	".\vc40_nt\Release\getopt.obj"

"$(OUTDIR)\pyth_nt.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vc40_nt\Debug"
# PROP BASE Intermediate_Dir "vc40_nt\Debug"
# PROP BASE Target_Dir "vc40_nt"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vc40_nt\Debug"
# PROP Intermediate_Dir "vc40_nt\Debug"
# PROP Target_Dir "vc40_nt"
OUTDIR=.\vc40_nt\Debug
INTDIR=.\vc40_nt\Debug

ALL : "$(OUTDIR)\pyth_nt.exe"

CLEAN : 
	-@erase ".\vc40_nt\Debug\vc40.pdb"
	-@erase ".\vc40_nt\Debug\vc40.idb"
	-@erase ".\vc40_nt\Debug\pyth_nt.exe"
	-@erase ".\vc40_nt\Debug\longobject.obj"
	-@erase ".\vc40_nt\Debug\listobject.obj"
	-@erase ".\vc40_nt\Debug\intobject.obj"
	-@erase ".\vc40_nt\Debug\importdl.obj"
	-@erase ".\vc40_nt\Debug\import.obj"
	-@erase ".\vc40_nt\Debug\imageop.obj"
	-@erase ".\vc40_nt\Debug\grammar1.obj"
	-@erase ".\vc40_nt\Debug\graminit.obj"
	-@erase ".\vc40_nt\Debug\getversion.obj"
	-@erase ".\vc40_nt\Debug\getplatform.obj"
	-@erase ".\vc40_nt\Debug\getpath.obj"
	-@erase ".\vc40_nt\Debug\getmtime.obj"
	-@erase ".\vc40_nt\Debug\getcopyright.obj"
	-@erase ".\vc40_nt\Debug\getcompiler.obj"
	-@erase ".\vc40_nt\Debug\getargs.obj"
	-@erase ".\vc40_nt\Debug\funcobject.obj"
	-@erase ".\vc40_nt\Debug\frozen.obj"
	-@erase ".\vc40_nt\Debug\frameobject.obj"
	-@erase ".\vc40_nt\Debug\floatobject.obj"
	-@erase ".\vc40_nt\Debug\fileobject.obj"
	-@erase ".\vc40_nt\Debug\errors.obj"
	-@erase ".\vc40_nt\Debug\environment.obj"
	-@erase ".\vc40_nt\Debug\config.obj"
	-@erase ".\vc40_nt\Debug\complexobject.obj"
	-@erase ".\vc40_nt\Debug\compile.obj"
	-@erase ".\vc40_nt\Debug\cobject.obj"
	-@erase ".\vc40_nt\Debug\cmathmodule.obj"
	-@erase ".\vc40_nt\Debug\classobject.obj"
	-@erase ".\vc40_nt\Debug\cgensupport.obj"
	-@erase ".\vc40_nt\Debug\ceval.obj"
	-@erase ".\vc40_nt\Debug\bltinmodule.obj"
	-@erase ".\vc40_nt\Debug\binascii.obj"
	-@erase ".\vc40_nt\Debug\audioop.obj"
	-@erase ".\vc40_nt\Debug\arraymodule.obj"
	-@erase ".\vc40_nt\Debug\accessobject.obj"
	-@erase ".\vc40_nt\Debug\acceler.obj"
	-@erase ".\vc40_nt\Debug\abstract.obj"
	-@erase ".\vc40_nt\Debug\yuvconvert.obj"
	-@erase ".\vc40_nt\Debug\typeobject.obj"
	-@erase ".\vc40_nt\Debug\tupleobject.obj"
	-@erase ".\vc40_nt\Debug\traceback.obj"
	-@erase ".\vc40_nt\Debug\tokenizer.obj"
	-@erase ".\vc40_nt\Debug\timemodule.obj"
	-@erase ".\vc40_nt\Debug\threadmodule.obj"
	-@erase ".\vc40_nt\Debug\thread.obj"
	-@erase ".\vc40_nt\Debug\sysmodule.obj"
	-@erase ".\vc40_nt\Debug\structmodule.obj"
	-@erase ".\vc40_nt\Debug\structmember.obj"
	-@erase ".\vc40_nt\Debug\stropmodule.obj"
	-@erase ".\vc40_nt\Debug\stringobject.obj"
	-@erase ".\vc40_nt\Debug\soundex.obj"
	-@erase ".\vc40_nt\Debug\socketmodule.obj"
	-@erase ".\vc40_nt\Debug\signalmodule.obj"
	-@erase ".\vc40_nt\Debug\selectmodule.obj"
	-@erase ".\vc40_nt\Debug\rotormodule.obj"
	-@erase ".\vc40_nt\Debug\rgbimgmodule.obj"
	-@erase ".\vc40_nt\Debug\regexpr.obj"
	-@erase ".\vc40_nt\Debug\regexmodule.obj"
	-@erase ".\vc40_nt\Debug\rangeobject.obj"
	-@erase ".\vc40_nt\Debug\pythonrun.obj"
	-@erase ".\vc40_nt\Debug\posixmodule.obj"
	-@erase ".\vc40_nt\Debug\parsetok.obj"
	-@erase ".\vc40_nt\Debug\parser.obj"
	-@erase ".\vc40_nt\Debug\object.obj"
	-@erase ".\vc40_nt\Debug\node.obj"
	-@erase ".\vc40_nt\Debug\newmodule.obj"
	-@erase ".\vc40_nt\Debug\mystrtoul.obj"
	-@erase ".\vc40_nt\Debug\myreadline.obj"
	-@erase ".\vc40_nt\Debug\moduleobject.obj"
	-@erase ".\vc40_nt\Debug\modsupport.obj"
	-@erase ".\vc40_nt\Debug\methodobject.obj"
	-@erase ".\vc40_nt\Debug\md5module.obj"
	-@erase ".\vc40_nt\Debug\md5c.obj"
	-@erase ".\vc40_nt\Debug\mathmodule.obj"
	-@erase ".\vc40_nt\Debug\marshal.obj"
	-@erase ".\vc40_nt\Debug\mappingobject.obj"
	-@erase ".\vc40_nt\Debug\main.obj"
	-@erase ".\vc40_nt\Debug\getopt.obj"
	-@erase ".\vc40_nt\Debug\pyth_nt.ilk"
	-@erase ".\vc40_nt\Debug\pyth_nt.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "./PC" /I "./Include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "HAVE_CONFIG_H" /YX /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "./PC" /I "./Include" /D "_DEBUG"\
 /D "WIN32" /D "_CONSOLE" /D "HAVE_CONFIG_H" /Fp"$(INTDIR)/vc40_nt.pch" /YX\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\vc40_nt\Debug/
CPP_SBRS=

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/vc40_nt.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:console /debug /machine:I386 /out:"vc40_nt\Debug/pyth_nt.exe"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib wsock32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/pyth_nt.pdb" /debug /machine:I386 /out:"$(OUTDIR)/pyth_nt.exe" 
LINK32_OBJS= \
	".\vc40_nt\Debug\longobject.obj" \
	".\vc40_nt\Debug\listobject.obj" \
	".\vc40_nt\Debug\intobject.obj" \
	".\vc40_nt\Debug\importdl.obj" \
	".\vc40_nt\Debug\import.obj" \
	".\vc40_nt\Debug\imageop.obj" \
	".\vc40_nt\Debug\grammar1.obj" \
	".\vc40_nt\Debug\graminit.obj" \
	".\vc40_nt\Debug\getversion.obj" \
	".\vc40_nt\Debug\getplatform.obj" \
	".\vc40_nt\Debug\getpath.obj" \
	".\vc40_nt\Debug\getmtime.obj" \
	".\vc40_nt\Debug\getcopyright.obj" \
	".\vc40_nt\Debug\getcompiler.obj" \
	".\vc40_nt\Debug\getargs.obj" \
	".\vc40_nt\Debug\funcobject.obj" \
	".\vc40_nt\Debug\frozen.obj" \
	".\vc40_nt\Debug\frameobject.obj" \
	".\vc40_nt\Debug\floatobject.obj" \
	".\vc40_nt\Debug\fileobject.obj" \
	".\vc40_nt\Debug\errors.obj" \
	".\vc40_nt\Debug\environment.obj" \
	".\vc40_nt\Debug\config.obj" \
	".\vc40_nt\Debug\complexobject.obj" \
	".\vc40_nt\Debug\compile.obj" \
	".\vc40_nt\Debug\cobject.obj" \
	".\vc40_nt\Debug\cmathmodule.obj" \
	".\vc40_nt\Debug\classobject.obj" \
	".\vc40_nt\Debug\cgensupport.obj" \
	".\vc40_nt\Debug\ceval.obj" \
	".\vc40_nt\Debug\bltinmodule.obj" \
	".\vc40_nt\Debug\binascii.obj" \
	".\vc40_nt\Debug\audioop.obj" \
	".\vc40_nt\Debug\arraymodule.obj" \
	".\vc40_nt\Debug\accessobject.obj" \
	".\vc40_nt\Debug\acceler.obj" \
	".\vc40_nt\Debug\abstract.obj" \
	".\vc40_nt\Debug\yuvconvert.obj" \
	".\vc40_nt\Debug\typeobject.obj" \
	".\vc40_nt\Debug\tupleobject.obj" \
	".\vc40_nt\Debug\traceback.obj" \
	".\vc40_nt\Debug\tokenizer.obj" \
	".\vc40_nt\Debug\timemodule.obj" \
	".\vc40_nt\Debug\threadmodule.obj" \
	".\vc40_nt\Debug\thread.obj" \
	".\vc40_nt\Debug\sysmodule.obj" \
	".\vc40_nt\Debug\structmodule.obj" \
	".\vc40_nt\Debug\structmember.obj" \
	".\vc40_nt\Debug\stropmodule.obj" \
	".\vc40_nt\Debug\stringobject.obj" \
	".\vc40_nt\Debug\soundex.obj" \
	".\vc40_nt\Debug\socketmodule.obj" \
	".\vc40_nt\Debug\signalmodule.obj" \
	".\vc40_nt\Debug\selectmodule.obj" \
	".\vc40_nt\Debug\rotormodule.obj" \
	".\vc40_nt\Debug\rgbimgmodule.obj" \
	".\vc40_nt\Debug\regexpr.obj" \
	".\vc40_nt\Debug\regexmodule.obj" \
	".\vc40_nt\Debug\rangeobject.obj" \
	".\vc40_nt\Debug\pythonrun.obj" \
	".\vc40_nt\Debug\posixmodule.obj" \
	".\vc40_nt\Debug\parsetok.obj" \
	".\vc40_nt\Debug\parser.obj" \
	".\vc40_nt\Debug\object.obj" \
	".\vc40_nt\Debug\node.obj" \
	".\vc40_nt\Debug\newmodule.obj" \
	".\vc40_nt\Debug\mystrtoul.obj" \
	".\vc40_nt\Debug\myreadline.obj" \
	".\vc40_nt\Debug\moduleobject.obj" \
	".\vc40_nt\Debug\modsupport.obj" \
	".\vc40_nt\Debug\methodobject.obj" \
	".\vc40_nt\Debug\md5module.obj" \
	".\vc40_nt\Debug\md5c.obj" \
	".\vc40_nt\Debug\mathmodule.obj" \
	".\vc40_nt\Debug\marshal.obj" \
	".\vc40_nt\Debug\mappingobject.obj" \
	".\vc40_nt\Debug\main.obj" \
	".\vc40_nt\Debug\getopt.obj"

"$(OUTDIR)\pyth_nt.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

################################################################################
# Begin Target

# Name "vc40 - Win32 Release"
# Name "vc40 - Win32 Debug"

!IF  "$(CFG)" == "vc40 - Win32 Release"

!ELSEIF  "$(CFG)" == "vc40 - Win32 Debug"

!ENDIF 

# End Target
################################################################################
# Begin Target

# Name "vc40_dll - Win32 Release"
# Name "vc40_dll - Win32 Debug"

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\Objects\longobject.c
DEP_CPP_LONGO=\
	".\./Include\allobjects.h"\
	".\./Include\longintrepr.h"\
	".\./Include\mymath.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\longobject.obj" : $(SOURCE) $(DEP_CPP_LONGO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\longobject.obj" : $(SOURCE) $(DEP_CPP_LONGO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\listobject.c
DEP_CPP_LISTO=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\listobject.obj" : $(SOURCE) $(DEP_CPP_LISTO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\listobject.obj" : $(SOURCE) $(DEP_CPP_LISTO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\intobject.c
DEP_CPP_INTOB=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\intobject.obj" : $(SOURCE) $(DEP_CPP_INTOB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\intobject.obj" : $(SOURCE) $(DEP_CPP_INTOB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\importdl.c
DEP_CPP_IMPOR=\
	".\./Include\allobjects.h"\
	".\./Include\osdefs.h"\
	".\Python\importdl.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	{$(INCLUDE)}"\sys\STAT.H"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	
NODEP_CPP_IMPOR=\
	".\Python\dl.h"\
	".\Python\macdefs.h"\
	".\Python\macglue.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\importdl.obj" : $(SOURCE) $(DEP_CPP_IMPOR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\importdl.obj" : $(SOURCE) $(DEP_CPP_IMPOR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\import.c
DEP_CPP_IMPORT=\
	".\./Include\allobjects.h"\
	".\./Include\node.h"\
	".\./Include\token.h"\
	".\./Include\graminit.h"\
	".\./Include\import.h"\
	".\./Include\errcode.h"\
	".\./Include\sysmodule.h"\
	".\./Include\bltinmodule.h"\
	".\./Include\pythonrun.h"\
	".\./Include\marshal.h"\
	".\./Include\compile.h"\
	".\./Include\eval.h"\
	".\./Include\osdefs.h"\
	".\pc\importdl.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\intrcheck.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	
NODEP_CPP_IMPORT=\
	".\pc\macglue.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\import.obj" : $(SOURCE) $(DEP_CPP_IMPORT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\import.obj" : $(SOURCE) $(DEP_CPP_IMPORT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\imageop.c
DEP_CPP_IMAGE=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\imageop.obj" : $(SOURCE) $(DEP_CPP_IMAGE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\imageop.obj" : $(SOURCE) $(DEP_CPP_IMAGE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\grammar1.c
DEP_CPP_GRAMM=\
	".\./Include\pgenheaders.h"\
	".\./Include\grammar.h"\
	".\./Include\token.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\./Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\Include\bitset.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\grammar1.obj" : $(SOURCE) $(DEP_CPP_GRAMM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\grammar1.obj" : $(SOURCE) $(DEP_CPP_GRAMM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\graminit.c
DEP_CPP_GRAMI=\
	".\./Include\pgenheaders.h"\
	".\./Include\grammar.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\./Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\Include\bitset.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\graminit.obj" : $(SOURCE) $(DEP_CPP_GRAMI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\graminit.obj" : $(SOURCE) $(DEP_CPP_GRAMI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getversion.c
DEP_CPP_GETVE=\
	".\./Include\Python.h"\
	".\./Include\patchlevel.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\getversion.obj" : $(SOURCE) $(DEP_CPP_GETVE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\getversion.obj" : $(SOURCE) $(DEP_CPP_GETVE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getplatform.c
DEP_CPP_GETPL=\
	".\./Include\Python.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\getplatform.obj" : $(SOURCE) $(DEP_CPP_GETPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\getplatform.obj" : $(SOURCE) $(DEP_CPP_GETPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\getpath.c
DEP_CPP_GETPA=\
	".\./Include\Python.h"\
	".\./Include\osdefs.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\getpath.obj" : $(SOURCE) $(DEP_CPP_GETPA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\getpath.obj" : $(SOURCE) $(DEP_CPP_GETPA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getmtime.c
DEP_CPP_GETMT=\
	".\./PC\config.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	{$(INCLUDE)}"\sys\STAT.H"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\getmtime.obj" : $(SOURCE) $(DEP_CPP_GETMT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\getmtime.obj" : $(SOURCE) $(DEP_CPP_GETMT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getcopyright.c
DEP_CPP_GETCO=\
	".\./Include\Python.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\getcopyright.obj" : $(SOURCE) $(DEP_CPP_GETCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\getcopyright.obj" : $(SOURCE) $(DEP_CPP_GETCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getcompiler.c
DEP_CPP_GETCOM=\
	".\./Include\Python.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\getcompiler.obj" : $(SOURCE) $(DEP_CPP_GETCOM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\getcompiler.obj" : $(SOURCE) $(DEP_CPP_GETCOM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getargs.c
DEP_CPP_GETAR=\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\getargs.obj" : $(SOURCE) $(DEP_CPP_GETAR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\getargs.obj" : $(SOURCE) $(DEP_CPP_GETAR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\funcobject.c
DEP_CPP_FUNCO=\
	".\./Include\allobjects.h"\
	".\./Include\compile.h"\
	".\./Include\structmember.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\funcobject.obj" : $(SOURCE) $(DEP_CPP_FUNCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\funcobject.obj" : $(SOURCE) $(DEP_CPP_FUNCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\frozen.c
DEP_CPP_FROZE=\
	".\./Include\Python.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\frozen.obj" : $(SOURCE) $(DEP_CPP_FROZE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\frozen.obj" : $(SOURCE) $(DEP_CPP_FROZE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\frameobject.c
DEP_CPP_FRAME=\
	".\./Include\allobjects.h"\
	".\./Include\compile.h"\
	".\./Include\frameobject.h"\
	".\./Include\opcode.h"\
	".\./Include\structmember.h"\
	".\./Include\bltinmodule.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\frameobject.obj" : $(SOURCE) $(DEP_CPP_FRAME) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\frameobject.obj" : $(SOURCE) $(DEP_CPP_FRAME) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\floatobject.c
DEP_CPP_FLOAT=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\mymath.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\floatobject.obj" : $(SOURCE) $(DEP_CPP_FLOAT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\floatobject.obj" : $(SOURCE) $(DEP_CPP_FLOAT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\fileobject.c
DEP_CPP_FILEO=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\structmember.h"\
	".\./Include\ceval.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\fileobject.obj" : $(SOURCE) $(DEP_CPP_FILEO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\fileobject.obj" : $(SOURCE) $(DEP_CPP_FILEO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\errors.c
DEP_CPP_ERROR=\
	".\./Include\allobjects.h"\
	".\./Include\traceback.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\errors.obj" : $(SOURCE) $(DEP_CPP_ERROR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\errors.obj" : $(SOURCE) $(DEP_CPP_ERROR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\environment.c
DEP_CPP_ENVIR=\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\environment.obj" : $(SOURCE) $(DEP_CPP_ENVIR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\environment.obj" : $(SOURCE) $(DEP_CPP_ENVIR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\config.c
DEP_CPP_CONFI=\
	".\./Include\Python.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\config.obj" : $(SOURCE) $(DEP_CPP_CONFI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\config.obj" : $(SOURCE) $(DEP_CPP_CONFI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\complexobject.c
DEP_CPP_COMPL=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\mymath.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\complexobject.obj" : $(SOURCE) $(DEP_CPP_COMPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\complexobject.obj" : $(SOURCE) $(DEP_CPP_COMPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\compile.c
DEP_CPP_COMPI=\
	".\./Include\allobjects.h"\
	".\./Include\node.h"\
	".\./Include\token.h"\
	".\./Include\graminit.h"\
	".\./Include\compile.h"\
	".\./Include\opcode.h"\
	".\./Include\structmember.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\compile.obj" : $(SOURCE) $(DEP_CPP_COMPI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\compile.obj" : $(SOURCE) $(DEP_CPP_COMPI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\cobject.c
DEP_CPP_COBJE=\
	".\./Include\Python.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\cobject.obj" : $(SOURCE) $(DEP_CPP_COBJE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\cobject.obj" : $(SOURCE) $(DEP_CPP_COBJE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\cmathmodule.c
DEP_CPP_CMATH=\
	".\./Include\allobjects.h"\
	".\./Include\complexobject.h"\
	".\./Include\mymath.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\cmathmodule.obj" : $(SOURCE) $(DEP_CPP_CMATH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\cmathmodule.obj" : $(SOURCE) $(DEP_CPP_CMATH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\classobject.c
DEP_CPP_CLASS=\
	".\./Include\allobjects.h"\
	".\./Include\structmember.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\classobject.obj" : $(SOURCE) $(DEP_CPP_CLASS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\classobject.obj" : $(SOURCE) $(DEP_CPP_CLASS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\cgensupport.c
DEP_CPP_CGENS=\
	".\./Include\allobjects.h"\
	".\./Include\cgensupport.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\cgensupport.obj" : $(SOURCE) $(DEP_CPP_CGENS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\cgensupport.obj" : $(SOURCE) $(DEP_CPP_CGENS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\ceval.c
DEP_CPP_CEVAL=\
	".\./Include\allobjects.h"\
	".\./Include\compile.h"\
	".\./Include\frameobject.h"\
	".\./Include\eval.h"\
	".\./Include\opcode.h"\
	".\./Include\graminit.h"\
	".\./Include\thread.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\ceval.obj" : $(SOURCE) $(DEP_CPP_CEVAL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\ceval.obj" : $(SOURCE) $(DEP_CPP_CEVAL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\bltinmodule.c
DEP_CPP_BLTIN=\
	".\./Include\allobjects.h"\
	".\./Include\node.h"\
	".\./Include\graminit.h"\
	".\./Include\bltinmodule.h"\
	".\./Include\import.h"\
	".\./Include\compile.h"\
	".\./Include\eval.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\bltinmodule.obj" : $(SOURCE) $(DEP_CPP_BLTIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\bltinmodule.obj" : $(SOURCE) $(DEP_CPP_BLTIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\binascii.c
DEP_CPP_BINAS=\
	".\./Include\Python.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\binascii.obj" : $(SOURCE) $(DEP_CPP_BINAS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\binascii.obj" : $(SOURCE) $(DEP_CPP_BINAS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\audioop.c
DEP_CPP_AUDIO=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\mymath.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\audioop.obj" : $(SOURCE) $(DEP_CPP_AUDIO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\audioop.obj" : $(SOURCE) $(DEP_CPP_AUDIO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\arraymodule.c
DEP_CPP_ARRAY=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\arraymodule.obj" : $(SOURCE) $(DEP_CPP_ARRAY) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\arraymodule.obj" : $(SOURCE) $(DEP_CPP_ARRAY) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\accessobject.c
DEP_CPP_ACCES=\
	".\./Include\allobjects.h"\
	".\./Include\ceval.h"\
	".\./Include\structmember.h"\
	".\./Include\modsupport.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\accessobject.obj" : $(SOURCE) $(DEP_CPP_ACCES) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\accessobject.obj" : $(SOURCE) $(DEP_CPP_ACCES) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\acceler.c
DEP_CPP_ACCEL=\
	".\./Include\pgenheaders.h"\
	".\./Include\grammar.h"\
	".\./Include\node.h"\
	".\./Include\token.h"\
	".\Parser\parser.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\./Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\Include\bitset.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\acceler.obj" : $(SOURCE) $(DEP_CPP_ACCEL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\acceler.obj" : $(SOURCE) $(DEP_CPP_ACCEL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\abstract.c
DEP_CPP_ABSTR=\
	".\./Include\Python.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\abstract.obj" : $(SOURCE) $(DEP_CPP_ABSTR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\abstract.obj" : $(SOURCE) $(DEP_CPP_ABSTR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\yuvconvert.c
DEP_CPP_YUVCO=\
	".\Modules\yuv.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\yuvconvert.obj" : $(SOURCE) $(DEP_CPP_YUVCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\yuvconvert.obj" : $(SOURCE) $(DEP_CPP_YUVCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\typeobject.c
DEP_CPP_TYPEO=\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\typeobject.obj" : $(SOURCE) $(DEP_CPP_TYPEO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\typeobject.obj" : $(SOURCE) $(DEP_CPP_TYPEO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\tupleobject.c
DEP_CPP_TUPLE=\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\tupleobject.obj" : $(SOURCE) $(DEP_CPP_TUPLE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\tupleobject.obj" : $(SOURCE) $(DEP_CPP_TUPLE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\traceback.c
DEP_CPP_TRACE=\
	".\./Include\allobjects.h"\
	".\./Include\sysmodule.h"\
	".\./Include\compile.h"\
	".\./Include\frameobject.h"\
	".\./Include\traceback.h"\
	".\./Include\structmember.h"\
	".\./Include\osdefs.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\traceback.obj" : $(SOURCE) $(DEP_CPP_TRACE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\traceback.obj" : $(SOURCE) $(DEP_CPP_TRACE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\tokenizer.c
DEP_CPP_TOKEN=\
	".\./Include\pgenheaders.h"\
	".\Parser\tokenizer.h"\
	".\./Include\errcode.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\./Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\./Include\token.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\tokenizer.obj" : $(SOURCE) $(DEP_CPP_TOKEN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\tokenizer.obj" : $(SOURCE) $(DEP_CPP_TOKEN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\timemodule.c
DEP_CPP_TIMEM=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	".\./Include\myselect.h"\
	".\./Include\mytime.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\timemodule.obj" : $(SOURCE) $(DEP_CPP_TIMEM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\timemodule.obj" : $(SOURCE) $(DEP_CPP_TIMEM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\threadmodule.c
DEP_CPP_THREA=\
	".\./Include\allobjects.h"\
	".\./Include\thread.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\threadmodule.obj" : $(SOURCE) $(DEP_CPP_THREA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\threadmodule.obj" : $(SOURCE) $(DEP_CPP_THREA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\thread.c
DEP_CPP_THREAD=\
	".\./PC\config.h"\
	".\./Include\thread.h"\
	".\Python\thread_sgi.h"\
	".\Python\thread_solaris.h"\
	".\Python\thread_lwp.h"\
	".\Python\thread_pthread.h"\
	".\Python\thread_cthread.h"\
	".\Python\thread_nt.h"\
	".\Python\thread_foobar.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_THREAD=\
	"..\..\usr\include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\thread.obj" : $(SOURCE) $(DEP_CPP_THREAD) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\thread.obj" : $(SOURCE) $(DEP_CPP_THREAD) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\sysmodule.c
DEP_CPP_SYSMO=\
	".\./Include\allobjects.h"\
	".\./Include\sysmodule.h"\
	".\./Include\import.h"\
	".\./Include\modsupport.h"\
	".\./Include\osdefs.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\intrcheck.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\sysmodule.obj" : $(SOURCE) $(DEP_CPP_SYSMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\sysmodule.obj" : $(SOURCE) $(DEP_CPP_SYSMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\structmodule.c
DEP_CPP_STRUC=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\structmodule.obj" : $(SOURCE) $(DEP_CPP_STRUC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\structmodule.obj" : $(SOURCE) $(DEP_CPP_STRUC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\structmember.c
DEP_CPP_STRUCT=\
	".\./Include\allobjects.h"\
	".\./Include\structmember.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\structmember.obj" : $(SOURCE) $(DEP_CPP_STRUCT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\structmember.obj" : $(SOURCE) $(DEP_CPP_STRUCT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\stropmodule.c
DEP_CPP_STROP=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\stropmodule.obj" : $(SOURCE) $(DEP_CPP_STROP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\stropmodule.obj" : $(SOURCE) $(DEP_CPP_STROP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\stringobject.c
DEP_CPP_STRIN=\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\stringobject.obj" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\stringobject.obj" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\soundex.c
DEP_CPP_SOUND=\
	".\./Include\Python.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\soundex.obj" : $(SOURCE) $(DEP_CPP_SOUND) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\soundex.obj" : $(SOURCE) $(DEP_CPP_SOUND) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\socketmodule.c
DEP_CPP_SOCKE=\
	".\./Include\Python.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	".\./Include\mytime.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\socketmodule.obj" : $(SOURCE) $(DEP_CPP_SOCKE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\socketmodule.obj" : $(SOURCE) $(DEP_CPP_SOCKE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\signalmodule.c
DEP_CPP_SIGNA=\
	".\./Include\Python.h"\
	".\./Include\intrcheck.h"\
	".\./Include\thread.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\signalmodule.obj" : $(SOURCE) $(DEP_CPP_SIGNA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\signalmodule.obj" : $(SOURCE) $(DEP_CPP_SIGNA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\selectmodule.c
DEP_CPP_SELEC=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	".\./Include\myselect.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\mytime.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\selectmodule.obj" : $(SOURCE) $(DEP_CPP_SELEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\selectmodule.obj" : $(SOURCE) $(DEP_CPP_SELEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\rotormodule.c
DEP_CPP_ROTOR=\
	".\./Include\Python.h"\
	".\./Include\mymath.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\rotormodule.obj" : $(SOURCE) $(DEP_CPP_ROTOR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\rotormodule.obj" : $(SOURCE) $(DEP_CPP_ROTOR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\rgbimgmodule.c
DEP_CPP_RGBIM=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\rgbimgmodule.obj" : $(SOURCE) $(DEP_CPP_RGBIM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\rgbimgmodule.obj" : $(SOURCE) $(DEP_CPP_RGBIM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\regexpr.c
DEP_CPP_REGEX=\
	".\./Include\myproto.h"\
	".\Modules\regexpr.h"\
	".\./PC\config.h"\
	".\./Include\rename2.h"\
	
NODEP_CPP_REGEX=\
	".\Modules\lisp.h"\
	".\Modules\buffer.h"\
	".\Modules\syntax.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\regexpr.obj" : $(SOURCE) $(DEP_CPP_REGEX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\regexpr.obj" : $(SOURCE) $(DEP_CPP_REGEX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\regexmodule.c
DEP_CPP_REGEXM=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\Modules\regexpr.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\regexmodule.obj" : $(SOURCE) $(DEP_CPP_REGEXM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\regexmodule.obj" : $(SOURCE) $(DEP_CPP_REGEXM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\rangeobject.c
DEP_CPP_RANGE=\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\rangeobject.obj" : $(SOURCE) $(DEP_CPP_RANGE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\rangeobject.obj" : $(SOURCE) $(DEP_CPP_RANGE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\pythonrun.c
DEP_CPP_PYTHO=\
	".\./Include\allobjects.h"\
	".\./Include\grammar.h"\
	".\./Include\node.h"\
	".\./Include\parsetok.h"\
	".\./Include\graminit.h"\
	".\./Include\errcode.h"\
	".\./Include\sysmodule.h"\
	".\./Include\bltinmodule.h"\
	".\./Include\compile.h"\
	".\./Include\eval.h"\
	".\./Include\ceval.h"\
	".\./Include\import.h"\
	".\./Include\marshal.h"\
	".\./Include\thread.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\pythonrun.h"\
	".\./Include\intrcheck.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\Include\bitset.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\pythonrun.obj" : $(SOURCE) $(DEP_CPP_PYTHO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\pythonrun.obj" : $(SOURCE) $(DEP_CPP_PYTHO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\posixmodule.c
DEP_CPP_POSIX=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	{$(INCLUDE)}"\sys\STAT.H"\
	".\./Include\mytime.h"\
	{$(INCLUDE)}"\sys\UTIME.H"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\posixmodule.obj" : $(SOURCE) $(DEP_CPP_POSIX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\posixmodule.obj" : $(SOURCE) $(DEP_CPP_POSIX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\parsetok.c
DEP_CPP_PARSE=\
	".\./Include\pgenheaders.h"\
	".\Parser\tokenizer.h"\
	".\./Include\node.h"\
	".\./Include\grammar.h"\
	".\Parser\parser.h"\
	".\./Include\parsetok.h"\
	".\./Include\errcode.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\./Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\./Include\token.h"\
	".\Include\bitset.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\parsetok.obj" : $(SOURCE) $(DEP_CPP_PARSE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\parsetok.obj" : $(SOURCE) $(DEP_CPP_PARSE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\parser.c
DEP_CPP_PARSER=\
	".\./Include\pgenheaders.h"\
	".\./Include\token.h"\
	".\./Include\grammar.h"\
	".\./Include\node.h"\
	".\Parser\parser.h"\
	".\./Include\errcode.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\./Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\Include\bitset.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\parser.obj" : $(SOURCE) $(DEP_CPP_PARSER) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\parser.obj" : $(SOURCE) $(DEP_CPP_PARSER) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\object.c
DEP_CPP_OBJEC=\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\object.obj" : $(SOURCE) $(DEP_CPP_OBJEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\object.obj" : $(SOURCE) $(DEP_CPP_OBJEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\node.c
DEP_CPP_NODE_=\
	".\./Include\pgenheaders.h"\
	".\./Include\node.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\./Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\node.obj" : $(SOURCE) $(DEP_CPP_NODE_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\node.obj" : $(SOURCE) $(DEP_CPP_NODE_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\newmodule.c
DEP_CPP_NEWMO=\
	".\./Include\allobjects.h"\
	".\./Include\compile.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\newmodule.obj" : $(SOURCE) $(DEP_CPP_NEWMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\newmodule.obj" : $(SOURCE) $(DEP_CPP_NEWMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\marshal.c
DEP_CPP_MARSH=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\longintrepr.h"\
	".\./Include\compile.h"\
	".\./Include\marshal.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\marshal.obj" : $(SOURCE) $(DEP_CPP_MARSH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\marshal.obj" : $(SOURCE) $(DEP_CPP_MARSH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\mystrtoul.c
DEP_CPP_MYSTR=\
	".\./PC\config.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\mystrtoul.obj" : $(SOURCE) $(DEP_CPP_MYSTR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\mystrtoul.obj" : $(SOURCE) $(DEP_CPP_MYSTR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\myreadline.c
DEP_CPP_MYREA=\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\./Include\mymalloc.h"\
	".\./Include\intrcheck.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\myreadline.obj" : $(SOURCE) $(DEP_CPP_MYREA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\myreadline.obj" : $(SOURCE) $(DEP_CPP_MYREA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\moduleobject.c
DEP_CPP_MODUL=\
	".\./Include\allobjects.h"\
	".\./Include\ceval.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\moduleobject.obj" : $(SOURCE) $(DEP_CPP_MODUL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\moduleobject.obj" : $(SOURCE) $(DEP_CPP_MODUL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\modsupport.c
DEP_CPP_MODSU=\
	".\./Include\allobjects.h"\
	".\./Include\import.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\modsupport.obj" : $(SOURCE) $(DEP_CPP_MODSU) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\modsupport.obj" : $(SOURCE) $(DEP_CPP_MODSU) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\methodobject.c
DEP_CPP_METHO=\
	".\./Include\allobjects.h"\
	".\./Include\token.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\methodobject.obj" : $(SOURCE) $(DEP_CPP_METHO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\methodobject.obj" : $(SOURCE) $(DEP_CPP_METHO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\md5module.c
DEP_CPP_MD5MO=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\Modules\md5.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\md5module.obj" : $(SOURCE) $(DEP_CPP_MD5MO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\md5module.obj" : $(SOURCE) $(DEP_CPP_MD5MO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\md5c.c
DEP_CPP_MD5C_=\
	".\./PC\config.h"\
	".\Modules\md5.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\md5c.obj" : $(SOURCE) $(DEP_CPP_MD5C_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\md5c.obj" : $(SOURCE) $(DEP_CPP_MD5C_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\mathmodule.c
DEP_CPP_MATHM=\
	".\./Include\allobjects.h"\
	".\./Include\mymath.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\mathmodule.obj" : $(SOURCE) $(DEP_CPP_MATHM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\mathmodule.obj" : $(SOURCE) $(DEP_CPP_MATHM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\mappingobject.c
DEP_CPP_MAPPI=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_dll - Win32 Release"


"$(INTDIR)\mappingobject.obj" : $(SOURCE) $(DEP_CPP_MAPPI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"


"$(INTDIR)\mappingobject.obj" : $(SOURCE) $(DEP_CPP_MAPPI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\python.def

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

!ENDIF 

# End Source File
# End Target
################################################################################
# Begin Target

# Name "vc40_nt - Win32 Release"
# Name "vc40_nt - Win32 Debug"

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\Objects\longobject.c
DEP_CPP_LONGO=\
	".\./Include\allobjects.h"\
	".\./Include\longintrepr.h"\
	".\./Include\mymath.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\longobject.obj" : $(SOURCE) $(DEP_CPP_LONGO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\longobject.obj" : $(SOURCE) $(DEP_CPP_LONGO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\listobject.c
DEP_CPP_LISTO=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\listobject.obj" : $(SOURCE) $(DEP_CPP_LISTO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\listobject.obj" : $(SOURCE) $(DEP_CPP_LISTO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\intobject.c
DEP_CPP_INTOB=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\intobject.obj" : $(SOURCE) $(DEP_CPP_INTOB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\intobject.obj" : $(SOURCE) $(DEP_CPP_INTOB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\importdl.c
DEP_CPP_IMPOR=\
	".\./Include\allobjects.h"\
	".\./Include\osdefs.h"\
	".\Python\importdl.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	{$(INCLUDE)}"\sys\STAT.H"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	
NODEP_CPP_IMPOR=\
	".\Python\dl.h"\
	".\Python\macdefs.h"\
	".\Python\macglue.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\importdl.obj" : $(SOURCE) $(DEP_CPP_IMPOR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\importdl.obj" : $(SOURCE) $(DEP_CPP_IMPOR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\import.c
DEP_CPP_IMPORT=\
	".\./Include\allobjects.h"\
	".\./Include\node.h"\
	".\./Include\token.h"\
	".\./Include\graminit.h"\
	".\./Include\import.h"\
	".\./Include\errcode.h"\
	".\./Include\sysmodule.h"\
	".\./Include\bltinmodule.h"\
	".\./Include\pythonrun.h"\
	".\./Include\marshal.h"\
	".\./Include\compile.h"\
	".\./Include\eval.h"\
	".\./Include\osdefs.h"\
	".\pc\importdl.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\intrcheck.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	
NODEP_CPP_IMPORT=\
	".\pc\macglue.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\import.obj" : $(SOURCE) $(DEP_CPP_IMPORT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\import.obj" : $(SOURCE) $(DEP_CPP_IMPORT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\imageop.c
DEP_CPP_IMAGE=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\imageop.obj" : $(SOURCE) $(DEP_CPP_IMAGE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\imageop.obj" : $(SOURCE) $(DEP_CPP_IMAGE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\grammar1.c
DEP_CPP_GRAMM=\
	".\./Include\pgenheaders.h"\
	".\./Include\grammar.h"\
	".\./Include\token.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\./Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\Include\bitset.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\grammar1.obj" : $(SOURCE) $(DEP_CPP_GRAMM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\grammar1.obj" : $(SOURCE) $(DEP_CPP_GRAMM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\graminit.c
DEP_CPP_GRAMI=\
	".\./Include\pgenheaders.h"\
	".\./Include\grammar.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\./Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\Include\bitset.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\graminit.obj" : $(SOURCE) $(DEP_CPP_GRAMI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\graminit.obj" : $(SOURCE) $(DEP_CPP_GRAMI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getversion.c
DEP_CPP_GETVE=\
	".\./Include\Python.h"\
	".\./Include\patchlevel.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\getversion.obj" : $(SOURCE) $(DEP_CPP_GETVE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\getversion.obj" : $(SOURCE) $(DEP_CPP_GETVE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getplatform.c
DEP_CPP_GETPL=\
	".\./Include\Python.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\getplatform.obj" : $(SOURCE) $(DEP_CPP_GETPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\getplatform.obj" : $(SOURCE) $(DEP_CPP_GETPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\getpath.c
DEP_CPP_GETPA=\
	".\./Include\Python.h"\
	".\./Include\osdefs.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\getpath.obj" : $(SOURCE) $(DEP_CPP_GETPA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\getpath.obj" : $(SOURCE) $(DEP_CPP_GETPA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getmtime.c
DEP_CPP_GETMT=\
	".\./PC\config.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	{$(INCLUDE)}"\sys\STAT.H"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\getmtime.obj" : $(SOURCE) $(DEP_CPP_GETMT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\getmtime.obj" : $(SOURCE) $(DEP_CPP_GETMT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getcopyright.c
DEP_CPP_GETCO=\
	".\./Include\Python.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\getcopyright.obj" : $(SOURCE) $(DEP_CPP_GETCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\getcopyright.obj" : $(SOURCE) $(DEP_CPP_GETCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getcompiler.c
DEP_CPP_GETCOM=\
	".\./Include\Python.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\getcompiler.obj" : $(SOURCE) $(DEP_CPP_GETCOM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\getcompiler.obj" : $(SOURCE) $(DEP_CPP_GETCOM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getargs.c
DEP_CPP_GETAR=\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\getargs.obj" : $(SOURCE) $(DEP_CPP_GETAR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\getargs.obj" : $(SOURCE) $(DEP_CPP_GETAR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\funcobject.c
DEP_CPP_FUNCO=\
	".\./Include\allobjects.h"\
	".\./Include\compile.h"\
	".\./Include\structmember.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\funcobject.obj" : $(SOURCE) $(DEP_CPP_FUNCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\funcobject.obj" : $(SOURCE) $(DEP_CPP_FUNCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\frozen.c
DEP_CPP_FROZE=\
	".\./Include\Python.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\frozen.obj" : $(SOURCE) $(DEP_CPP_FROZE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\frozen.obj" : $(SOURCE) $(DEP_CPP_FROZE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\frameobject.c
DEP_CPP_FRAME=\
	".\./Include\allobjects.h"\
	".\./Include\compile.h"\
	".\./Include\frameobject.h"\
	".\./Include\opcode.h"\
	".\./Include\structmember.h"\
	".\./Include\bltinmodule.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\frameobject.obj" : $(SOURCE) $(DEP_CPP_FRAME) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\frameobject.obj" : $(SOURCE) $(DEP_CPP_FRAME) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\floatobject.c
DEP_CPP_FLOAT=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\mymath.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\floatobject.obj" : $(SOURCE) $(DEP_CPP_FLOAT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\floatobject.obj" : $(SOURCE) $(DEP_CPP_FLOAT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\fileobject.c
DEP_CPP_FILEO=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\structmember.h"\
	".\./Include\ceval.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\fileobject.obj" : $(SOURCE) $(DEP_CPP_FILEO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\fileobject.obj" : $(SOURCE) $(DEP_CPP_FILEO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\errors.c
DEP_CPP_ERROR=\
	".\./Include\allobjects.h"\
	".\./Include\traceback.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\errors.obj" : $(SOURCE) $(DEP_CPP_ERROR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\errors.obj" : $(SOURCE) $(DEP_CPP_ERROR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\environment.c
DEP_CPP_ENVIR=\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\environment.obj" : $(SOURCE) $(DEP_CPP_ENVIR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\environment.obj" : $(SOURCE) $(DEP_CPP_ENVIR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\config.c
DEP_CPP_CONFI=\
	".\./Include\Python.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\config.obj" : $(SOURCE) $(DEP_CPP_CONFI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\config.obj" : $(SOURCE) $(DEP_CPP_CONFI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\complexobject.c
DEP_CPP_COMPL=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\mymath.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\complexobject.obj" : $(SOURCE) $(DEP_CPP_COMPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\complexobject.obj" : $(SOURCE) $(DEP_CPP_COMPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\compile.c
DEP_CPP_COMPI=\
	".\./Include\allobjects.h"\
	".\./Include\node.h"\
	".\./Include\token.h"\
	".\./Include\graminit.h"\
	".\./Include\compile.h"\
	".\./Include\opcode.h"\
	".\./Include\structmember.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\compile.obj" : $(SOURCE) $(DEP_CPP_COMPI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\compile.obj" : $(SOURCE) $(DEP_CPP_COMPI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\cobject.c
DEP_CPP_COBJE=\
	".\./Include\Python.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\cobject.obj" : $(SOURCE) $(DEP_CPP_COBJE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\cobject.obj" : $(SOURCE) $(DEP_CPP_COBJE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\cmathmodule.c
DEP_CPP_CMATH=\
	".\./Include\allobjects.h"\
	".\./Include\complexobject.h"\
	".\./Include\mymath.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\cmathmodule.obj" : $(SOURCE) $(DEP_CPP_CMATH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\cmathmodule.obj" : $(SOURCE) $(DEP_CPP_CMATH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\classobject.c
DEP_CPP_CLASS=\
	".\./Include\allobjects.h"\
	".\./Include\structmember.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\classobject.obj" : $(SOURCE) $(DEP_CPP_CLASS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\classobject.obj" : $(SOURCE) $(DEP_CPP_CLASS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\cgensupport.c
DEP_CPP_CGENS=\
	".\./Include\allobjects.h"\
	".\./Include\cgensupport.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\cgensupport.obj" : $(SOURCE) $(DEP_CPP_CGENS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\cgensupport.obj" : $(SOURCE) $(DEP_CPP_CGENS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\ceval.c
DEP_CPP_CEVAL=\
	".\./Include\allobjects.h"\
	".\./Include\compile.h"\
	".\./Include\frameobject.h"\
	".\./Include\eval.h"\
	".\./Include\opcode.h"\
	".\./Include\graminit.h"\
	".\./Include\thread.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\ceval.obj" : $(SOURCE) $(DEP_CPP_CEVAL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\ceval.obj" : $(SOURCE) $(DEP_CPP_CEVAL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\bltinmodule.c
DEP_CPP_BLTIN=\
	".\./Include\allobjects.h"\
	".\./Include\node.h"\
	".\./Include\graminit.h"\
	".\./Include\bltinmodule.h"\
	".\./Include\import.h"\
	".\./Include\compile.h"\
	".\./Include\eval.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\bltinmodule.obj" : $(SOURCE) $(DEP_CPP_BLTIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\bltinmodule.obj" : $(SOURCE) $(DEP_CPP_BLTIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\binascii.c
DEP_CPP_BINAS=\
	".\./Include\Python.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\binascii.obj" : $(SOURCE) $(DEP_CPP_BINAS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\binascii.obj" : $(SOURCE) $(DEP_CPP_BINAS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\audioop.c
DEP_CPP_AUDIO=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\mymath.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\audioop.obj" : $(SOURCE) $(DEP_CPP_AUDIO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\audioop.obj" : $(SOURCE) $(DEP_CPP_AUDIO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\arraymodule.c
DEP_CPP_ARRAY=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\arraymodule.obj" : $(SOURCE) $(DEP_CPP_ARRAY) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\arraymodule.obj" : $(SOURCE) $(DEP_CPP_ARRAY) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\accessobject.c
DEP_CPP_ACCES=\
	".\./Include\allobjects.h"\
	".\./Include\ceval.h"\
	".\./Include\structmember.h"\
	".\./Include\modsupport.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\accessobject.obj" : $(SOURCE) $(DEP_CPP_ACCES) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\accessobject.obj" : $(SOURCE) $(DEP_CPP_ACCES) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\acceler.c
DEP_CPP_ACCEL=\
	".\./Include\pgenheaders.h"\
	".\./Include\grammar.h"\
	".\./Include\node.h"\
	".\./Include\token.h"\
	".\Parser\parser.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\./Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\Include\bitset.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\acceler.obj" : $(SOURCE) $(DEP_CPP_ACCEL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\acceler.obj" : $(SOURCE) $(DEP_CPP_ACCEL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\abstract.c
DEP_CPP_ABSTR=\
	".\./Include\Python.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\abstract.obj" : $(SOURCE) $(DEP_CPP_ABSTR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\abstract.obj" : $(SOURCE) $(DEP_CPP_ABSTR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\yuvconvert.c
DEP_CPP_YUVCO=\
	".\Modules\yuv.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\yuvconvert.obj" : $(SOURCE) $(DEP_CPP_YUVCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\yuvconvert.obj" : $(SOURCE) $(DEP_CPP_YUVCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\typeobject.c
DEP_CPP_TYPEO=\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\typeobject.obj" : $(SOURCE) $(DEP_CPP_TYPEO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\typeobject.obj" : $(SOURCE) $(DEP_CPP_TYPEO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\tupleobject.c
DEP_CPP_TUPLE=\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\tupleobject.obj" : $(SOURCE) $(DEP_CPP_TUPLE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\tupleobject.obj" : $(SOURCE) $(DEP_CPP_TUPLE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\traceback.c
DEP_CPP_TRACE=\
	".\./Include\allobjects.h"\
	".\./Include\sysmodule.h"\
	".\./Include\compile.h"\
	".\./Include\frameobject.h"\
	".\./Include\traceback.h"\
	".\./Include\structmember.h"\
	".\./Include\osdefs.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\traceback.obj" : $(SOURCE) $(DEP_CPP_TRACE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\traceback.obj" : $(SOURCE) $(DEP_CPP_TRACE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\tokenizer.c
DEP_CPP_TOKEN=\
	".\./Include\pgenheaders.h"\
	".\Parser\tokenizer.h"\
	".\./Include\errcode.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\./Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\./Include\token.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\tokenizer.obj" : $(SOURCE) $(DEP_CPP_TOKEN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\tokenizer.obj" : $(SOURCE) $(DEP_CPP_TOKEN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\timemodule.c
DEP_CPP_TIMEM=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	".\./Include\myselect.h"\
	".\./Include\mytime.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\timemodule.obj" : $(SOURCE) $(DEP_CPP_TIMEM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\timemodule.obj" : $(SOURCE) $(DEP_CPP_TIMEM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\threadmodule.c
DEP_CPP_THREA=\
	".\./Include\allobjects.h"\
	".\./Include\thread.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\threadmodule.obj" : $(SOURCE) $(DEP_CPP_THREA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\threadmodule.obj" : $(SOURCE) $(DEP_CPP_THREA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\thread.c
DEP_CPP_THREAD=\
	".\./PC\config.h"\
	".\./Include\thread.h"\
	".\Python\thread_sgi.h"\
	".\Python\thread_solaris.h"\
	".\Python\thread_lwp.h"\
	".\Python\thread_pthread.h"\
	".\Python\thread_cthread.h"\
	".\Python\thread_nt.h"\
	".\Python\thread_foobar.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_THREAD=\
	"..\..\usr\include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\thread.obj" : $(SOURCE) $(DEP_CPP_THREAD) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\thread.obj" : $(SOURCE) $(DEP_CPP_THREAD) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\sysmodule.c
DEP_CPP_SYSMO=\
	".\./Include\allobjects.h"\
	".\./Include\sysmodule.h"\
	".\./Include\import.h"\
	".\./Include\modsupport.h"\
	".\./Include\osdefs.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\intrcheck.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\sysmodule.obj" : $(SOURCE) $(DEP_CPP_SYSMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\sysmodule.obj" : $(SOURCE) $(DEP_CPP_SYSMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\structmodule.c
DEP_CPP_STRUC=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\structmodule.obj" : $(SOURCE) $(DEP_CPP_STRUC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\structmodule.obj" : $(SOURCE) $(DEP_CPP_STRUC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\structmember.c
DEP_CPP_STRUCT=\
	".\./Include\allobjects.h"\
	".\./Include\structmember.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\structmember.obj" : $(SOURCE) $(DEP_CPP_STRUCT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\structmember.obj" : $(SOURCE) $(DEP_CPP_STRUCT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\stropmodule.c
DEP_CPP_STROP=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\stropmodule.obj" : $(SOURCE) $(DEP_CPP_STROP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\stropmodule.obj" : $(SOURCE) $(DEP_CPP_STROP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\stringobject.c
DEP_CPP_STRIN=\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\stringobject.obj" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\stringobject.obj" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\soundex.c
DEP_CPP_SOUND=\
	".\./Include\Python.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\soundex.obj" : $(SOURCE) $(DEP_CPP_SOUND) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\soundex.obj" : $(SOURCE) $(DEP_CPP_SOUND) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\socketmodule.c
DEP_CPP_SOCKE=\
	".\./Include\Python.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	".\./Include\mytime.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\socketmodule.obj" : $(SOURCE) $(DEP_CPP_SOCKE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\socketmodule.obj" : $(SOURCE) $(DEP_CPP_SOCKE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\signalmodule.c
DEP_CPP_SIGNA=\
	".\./Include\Python.h"\
	".\./Include\intrcheck.h"\
	".\./Include\thread.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\signalmodule.obj" : $(SOURCE) $(DEP_CPP_SIGNA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\signalmodule.obj" : $(SOURCE) $(DEP_CPP_SIGNA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\selectmodule.c
DEP_CPP_SELEC=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	".\./Include\myselect.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\mytime.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\selectmodule.obj" : $(SOURCE) $(DEP_CPP_SELEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\selectmodule.obj" : $(SOURCE) $(DEP_CPP_SELEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\rotormodule.c
DEP_CPP_ROTOR=\
	".\./Include\Python.h"\
	".\./Include\mymath.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\rotormodule.obj" : $(SOURCE) $(DEP_CPP_ROTOR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\rotormodule.obj" : $(SOURCE) $(DEP_CPP_ROTOR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\rgbimgmodule.c
DEP_CPP_RGBIM=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\rgbimgmodule.obj" : $(SOURCE) $(DEP_CPP_RGBIM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\rgbimgmodule.obj" : $(SOURCE) $(DEP_CPP_RGBIM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\regexpr.c
DEP_CPP_REGEX=\
	".\./Include\myproto.h"\
	".\Modules\regexpr.h"\
	".\./PC\config.h"\
	".\./Include\rename2.h"\
	
NODEP_CPP_REGEX=\
	".\Modules\lisp.h"\
	".\Modules\buffer.h"\
	".\Modules\syntax.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\regexpr.obj" : $(SOURCE) $(DEP_CPP_REGEX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\regexpr.obj" : $(SOURCE) $(DEP_CPP_REGEX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\regexmodule.c
DEP_CPP_REGEXM=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\Modules\regexpr.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\regexmodule.obj" : $(SOURCE) $(DEP_CPP_REGEXM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\regexmodule.obj" : $(SOURCE) $(DEP_CPP_REGEXM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\rangeobject.c
DEP_CPP_RANGE=\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\rangeobject.obj" : $(SOURCE) $(DEP_CPP_RANGE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\rangeobject.obj" : $(SOURCE) $(DEP_CPP_RANGE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\pythonrun.c
DEP_CPP_PYTHO=\
	".\./Include\allobjects.h"\
	".\./Include\grammar.h"\
	".\./Include\node.h"\
	".\./Include\parsetok.h"\
	".\./Include\graminit.h"\
	".\./Include\errcode.h"\
	".\./Include\sysmodule.h"\
	".\./Include\bltinmodule.h"\
	".\./Include\compile.h"\
	".\./Include\eval.h"\
	".\./Include\ceval.h"\
	".\./Include\import.h"\
	".\./Include\marshal.h"\
	".\./Include\thread.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\pythonrun.h"\
	".\./Include\intrcheck.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\Include\bitset.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\pythonrun.obj" : $(SOURCE) $(DEP_CPP_PYTHO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\pythonrun.obj" : $(SOURCE) $(DEP_CPP_PYTHO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\posixmodule.c
DEP_CPP_POSIX=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	{$(INCLUDE)}"\sys\STAT.H"\
	".\./Include\mytime.h"\
	{$(INCLUDE)}"\sys\UTIME.H"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\posixmodule.obj" : $(SOURCE) $(DEP_CPP_POSIX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\posixmodule.obj" : $(SOURCE) $(DEP_CPP_POSIX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\parsetok.c
DEP_CPP_PARSE=\
	".\./Include\pgenheaders.h"\
	".\Parser\tokenizer.h"\
	".\./Include\node.h"\
	".\./Include\grammar.h"\
	".\Parser\parser.h"\
	".\./Include\parsetok.h"\
	".\./Include\errcode.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\./Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\./Include\token.h"\
	".\Include\bitset.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\parsetok.obj" : $(SOURCE) $(DEP_CPP_PARSE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\parsetok.obj" : $(SOURCE) $(DEP_CPP_PARSE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\parser.c
DEP_CPP_PARSER=\
	".\./Include\pgenheaders.h"\
	".\./Include\token.h"\
	".\./Include\grammar.h"\
	".\./Include\node.h"\
	".\Parser\parser.h"\
	".\./Include\errcode.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\./Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\Include\bitset.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\parser.obj" : $(SOURCE) $(DEP_CPP_PARSER) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\parser.obj" : $(SOURCE) $(DEP_CPP_PARSER) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\object.c
DEP_CPP_OBJEC=\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\object.obj" : $(SOURCE) $(DEP_CPP_OBJEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\object.obj" : $(SOURCE) $(DEP_CPP_OBJEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\node.c
DEP_CPP_NODE_=\
	".\./Include\pgenheaders.h"\
	".\./Include\node.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\./Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\node.obj" : $(SOURCE) $(DEP_CPP_NODE_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\node.obj" : $(SOURCE) $(DEP_CPP_NODE_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\newmodule.c
DEP_CPP_NEWMO=\
	".\./Include\allobjects.h"\
	".\./Include\compile.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\newmodule.obj" : $(SOURCE) $(DEP_CPP_NEWMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\newmodule.obj" : $(SOURCE) $(DEP_CPP_NEWMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\mystrtoul.c
DEP_CPP_MYSTR=\
	".\./PC\config.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\mystrtoul.obj" : $(SOURCE) $(DEP_CPP_MYSTR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\mystrtoul.obj" : $(SOURCE) $(DEP_CPP_MYSTR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\myreadline.c
DEP_CPP_MYREA=\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\./Include\mymalloc.h"\
	".\./Include\intrcheck.h"\
	".\./Include\rename2.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\myreadline.obj" : $(SOURCE) $(DEP_CPP_MYREA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\myreadline.obj" : $(SOURCE) $(DEP_CPP_MYREA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\moduleobject.c
DEP_CPP_MODUL=\
	".\./Include\allobjects.h"\
	".\./Include\ceval.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\moduleobject.obj" : $(SOURCE) $(DEP_CPP_MODUL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\moduleobject.obj" : $(SOURCE) $(DEP_CPP_MODUL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\modsupport.c
DEP_CPP_MODSU=\
	".\./Include\allobjects.h"\
	".\./Include\import.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\modsupport.obj" : $(SOURCE) $(DEP_CPP_MODSU) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\modsupport.obj" : $(SOURCE) $(DEP_CPP_MODSU) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\methodobject.c
DEP_CPP_METHO=\
	".\./Include\allobjects.h"\
	".\./Include\token.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\methodobject.obj" : $(SOURCE) $(DEP_CPP_METHO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\methodobject.obj" : $(SOURCE) $(DEP_CPP_METHO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\md5module.c
DEP_CPP_MD5MO=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\Modules\md5.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\md5module.obj" : $(SOURCE) $(DEP_CPP_MD5MO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\md5module.obj" : $(SOURCE) $(DEP_CPP_MD5MO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\md5c.c
DEP_CPP_MD5C_=\
	".\./PC\config.h"\
	".\Modules\md5.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\md5c.obj" : $(SOURCE) $(DEP_CPP_MD5C_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\md5c.obj" : $(SOURCE) $(DEP_CPP_MD5C_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\mathmodule.c
DEP_CPP_MATHM=\
	".\./Include\allobjects.h"\
	".\./Include\mymath.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\mathmodule.obj" : $(SOURCE) $(DEP_CPP_MATHM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\mathmodule.obj" : $(SOURCE) $(DEP_CPP_MATHM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\marshal.c
DEP_CPP_MARSH=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\longintrepr.h"\
	".\./Include\compile.h"\
	".\./Include\marshal.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\marshal.obj" : $(SOURCE) $(DEP_CPP_MARSH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\marshal.obj" : $(SOURCE) $(DEP_CPP_MARSH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\mappingobject.c
DEP_CPP_MAPPI=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

!IF  "$(CFG)" == "vc40_nt - Win32 Release"


"$(INTDIR)\mappingobject.obj" : $(SOURCE) $(DEP_CPP_MAPPI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"


"$(INTDIR)\mappingobject.obj" : $(SOURCE) $(DEP_CPP_MAPPI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\main.c
DEP_CPP_MAIN_=\
	".\./Include\Python.h"\
	".\./Include\allobjects.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\./Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\./Include\traceback.h"\
	".\Include\errors.h"\
	".\./Include\mymalloc.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	".\./Include\pythonrun.h"\
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getopt.c

"$(INTDIR)\getopt.obj" : $(SOURCE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
# End Target
# End Project
################################################################################
