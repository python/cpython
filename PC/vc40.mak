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
	-@erase ".\vc40_dll\Release\classobject.obj"
	-@erase ".\vc40_dll\Release\stropmodule.obj"
	-@erase ".\vc40_dll\Release\getargs.obj"
	-@erase ".\vc40_dll\Release\bltinmodule.obj"
	-@erase ".\vc40_dll\Release\structmember.obj"
	-@erase ".\vc40_dll\Release\rangeobject.obj"
	-@erase ".\vc40_dll\Release\socketmodule.obj"
	-@erase ".\vc40_dll\Release\audioop.obj"
	-@erase ".\vc40_dll\Release\frameobject.obj"
	-@erase ".\vc40_dll\Release\regexmodule.obj"
	-@erase ".\vc40_dll\Release\funcobject.obj"
	-@erase ".\vc40_dll\Release\getmtime.obj"
	-@erase ".\vc40_dll\Release\operator.obj"
	-@erase ".\vc40_dll\Release\stringobject.obj"
	-@erase ".\vc40_dll\Release\posixmodule.obj"
	-@erase ".\vc40_dll\Release\myreadline.obj"
	-@erase ".\vc40_dll\Release\cmathmodule.obj"
	-@erase ".\vc40_dll\Release\getcopyright.obj"
	-@erase ".\vc40_dll\Release\cgensupport.obj"
	-@erase ".\vc40_dll\Release\parsetok.obj"
	-@erase ".\vc40_dll\Release\newmodule.obj"
	-@erase ".\vc40_dll\Release\mappingobject.obj"
	-@erase ".\vc40_dll\Release\acceler.obj"
	-@erase ".\vc40_dll\Release\modsupport.obj"
	-@erase ".\vc40_dll\Release\threadmodule.obj"
	-@erase ".\vc40_dll\Release\soundex.obj"
	-@erase ".\vc40_dll\Release\parser.obj"
	-@erase ".\vc40_dll\Release\mathmodule.obj"
	-@erase ".\vc40_dll\Release\importdl.obj"
	-@erase ".\vc40_dll\Release\methodobject.obj"
	-@erase ".\vc40_dll\Release\getpath.obj"
	-@erase ".\vc40_dll\Release\mystrtoul.obj"
	-@erase ".\vc40_dll\Release\rgbimgmodule.obj"
	-@erase ".\vc40_dll\Release\rotormodule.obj"
	-@erase ".\vc40_dll\Release\binascii.obj"
	-@erase ".\vc40_dll\Release\node.obj"
	-@erase ".\vc40_dll\Release\config.obj"
	-@erase ".\vc40_dll\Release\selectmodule.obj"
	-@erase ".\vc40_dll\Release\signalmodule.obj"
	-@erase ".\vc40_dll\Release\marshal.obj"
	-@erase ".\vc40_dll\Release\fileobject.obj"
	-@erase ".\vc40_dll\Release\longobject.obj"
	-@erase ".\vc40_dll\Release\accessobject.obj"
	-@erase ".\vc40_dll\Release\ceval.obj"
	-@erase ".\vc40_dll\Release\compile.obj"
	-@erase ".\vc40_dll\Release\thread.obj"
	-@erase ".\vc40_dll\Release\cobject.obj"
	-@erase ".\vc40_dll\Release\getcompiler.obj"
	-@erase ".\vc40_dll\Release\grammar1.obj"
	-@erase ".\vc40_dll\Release\abstract.obj"
	-@erase ".\vc40_dll\Release\object.obj"
	-@erase ".\vc40_dll\Release\errnomodule.obj"
	-@erase ".\vc40_dll\Release\timemodule.obj"
	-@erase ".\vc40_dll\Release\complexobject.obj"
	-@erase ".\vc40_dll\Release\environment.obj"
	-@erase ".\vc40_dll\Release\floatobject.obj"
	-@erase ".\vc40_dll\Release\getplatform.obj"
	-@erase ".\vc40_dll\Release\tokenizer.obj"
	-@erase ".\vc40_dll\Release\intobject.obj"
	-@erase ".\vc40_dll\Release\import.obj"
	-@erase ".\vc40_dll\Release\frozen.obj"
	-@erase ".\vc40_dll\Release\structmodule.obj"
	-@erase ".\vc40_dll\Release\errors.obj"
	-@erase ".\vc40_dll\Release\arraymodule.obj"
	-@erase ".\vc40_dll\Release\md5module.obj"
	-@erase ".\vc40_dll\Release\traceback.obj"
	-@erase ".\vc40_dll\Release\sysmodule.obj"
	-@erase ".\vc40_dll\Release\listobject.obj"
	-@erase ".\vc40_dll\Release\typeobject.obj"
	-@erase ".\vc40_dll\Release\graminit.obj"
	-@erase ".\vc40_dll\Release\imageop.obj"
	-@erase ".\vc40_dll\Release\regexpr.obj"
	-@erase ".\vc40_dll\Release\pythonrun.obj"
	-@erase ".\vc40_dll\Release\md5c.obj"
	-@erase ".\vc40_dll\Release\tupleobject.obj"
	-@erase ".\vc40_dll\Release\yuvconvert.obj"
	-@erase ".\vc40_dll\Release\getversion.obj"
	-@erase ".\vc40_dll\Release\moduleobject.obj"
	-@erase ".\vc40_dll\Release\py14an.lib"
	-@erase ".\vc40_dll\Release\py14an.exp"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "./PC" /I "./Include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /D "USE_DL_EXPORT" /YX /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "./PC" /I "./Include" /D "NDEBUG" /D\
 "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /D "USE_DL_EXPORT"\
 /Fp"$(INTDIR)/vc40_dll.pch" /YX /Fo"$(INTDIR)/" /c 
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
	"$(INTDIR)/classobject.obj" \
	"$(INTDIR)/stropmodule.obj" \
	"$(INTDIR)/getargs.obj" \
	"$(INTDIR)/bltinmodule.obj" \
	"$(INTDIR)/structmember.obj" \
	"$(INTDIR)/rangeobject.obj" \
	"$(INTDIR)/socketmodule.obj" \
	"$(INTDIR)/audioop.obj" \
	"$(INTDIR)/frameobject.obj" \
	"$(INTDIR)/regexmodule.obj" \
	"$(INTDIR)/funcobject.obj" \
	"$(INTDIR)/getmtime.obj" \
	"$(INTDIR)/operator.obj" \
	"$(INTDIR)/stringobject.obj" \
	"$(INTDIR)/posixmodule.obj" \
	"$(INTDIR)/myreadline.obj" \
	"$(INTDIR)/cmathmodule.obj" \
	"$(INTDIR)/getcopyright.obj" \
	"$(INTDIR)/cgensupport.obj" \
	"$(INTDIR)/parsetok.obj" \
	"$(INTDIR)/newmodule.obj" \
	"$(INTDIR)/mappingobject.obj" \
	"$(INTDIR)/acceler.obj" \
	"$(INTDIR)/modsupport.obj" \
	"$(INTDIR)/threadmodule.obj" \
	"$(INTDIR)/soundex.obj" \
	"$(INTDIR)/parser.obj" \
	"$(INTDIR)/mathmodule.obj" \
	"$(INTDIR)/importdl.obj" \
	"$(INTDIR)/methodobject.obj" \
	"$(INTDIR)/getpath.obj" \
	"$(INTDIR)/mystrtoul.obj" \
	"$(INTDIR)/rgbimgmodule.obj" \
	"$(INTDIR)/rotormodule.obj" \
	"$(INTDIR)/binascii.obj" \
	"$(INTDIR)/node.obj" \
	"$(INTDIR)/config.obj" \
	"$(INTDIR)/selectmodule.obj" \
	"$(INTDIR)/signalmodule.obj" \
	"$(INTDIR)/marshal.obj" \
	"$(INTDIR)/fileobject.obj" \
	"$(INTDIR)/longobject.obj" \
	"$(INTDIR)/accessobject.obj" \
	"$(INTDIR)/ceval.obj" \
	"$(INTDIR)/compile.obj" \
	"$(INTDIR)/thread.obj" \
	"$(INTDIR)/cobject.obj" \
	"$(INTDIR)/getcompiler.obj" \
	"$(INTDIR)/grammar1.obj" \
	"$(INTDIR)/abstract.obj" \
	"$(INTDIR)/object.obj" \
	"$(INTDIR)/errnomodule.obj" \
	"$(INTDIR)/timemodule.obj" \
	"$(INTDIR)/complexobject.obj" \
	"$(INTDIR)/environment.obj" \
	"$(INTDIR)/floatobject.obj" \
	"$(INTDIR)/getplatform.obj" \
	"$(INTDIR)/tokenizer.obj" \
	"$(INTDIR)/intobject.obj" \
	"$(INTDIR)/import.obj" \
	"$(INTDIR)/frozen.obj" \
	"$(INTDIR)/structmodule.obj" \
	"$(INTDIR)/errors.obj" \
	"$(INTDIR)/arraymodule.obj" \
	"$(INTDIR)/md5module.obj" \
	"$(INTDIR)/traceback.obj" \
	"$(INTDIR)/sysmodule.obj" \
	"$(INTDIR)/listobject.obj" \
	"$(INTDIR)/typeobject.obj" \
	"$(INTDIR)/graminit.obj" \
	"$(INTDIR)/imageop.obj" \
	"$(INTDIR)/regexpr.obj" \
	"$(INTDIR)/pythonrun.obj" \
	"$(INTDIR)/md5c.obj" \
	"$(INTDIR)/tupleobject.obj" \
	"$(INTDIR)/yuvconvert.obj" \
	"$(INTDIR)/getversion.obj" \
	"$(INTDIR)/moduleobject.obj"

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
	-@erase ".\vc40_dll\Debug\node.obj"
	-@erase ".\vc40_dll\Debug\arraymodule.obj"
	-@erase ".\vc40_dll\Debug\funcobject.obj"
	-@erase ".\vc40_dll\Debug\structmember.obj"
	-@erase ".\vc40_dll\Debug\soundex.obj"
	-@erase ".\vc40_dll\Debug\tokenizer.obj"
	-@erase ".\vc40_dll\Debug\myreadline.obj"
	-@erase ".\vc40_dll\Debug\intobject.obj"
	-@erase ".\vc40_dll\Debug\tupleobject.obj"
	-@erase ".\vc40_dll\Debug\frozen.obj"
	-@erase ".\vc40_dll\Debug\socketmodule.obj"
	-@erase ".\vc40_dll\Debug\imageop.obj"
	-@erase ".\vc40_dll\Debug\graminit.obj"
	-@erase ".\vc40_dll\Debug\stropmodule.obj"
	-@erase ".\vc40_dll\Debug\modsupport.obj"
	-@erase ".\vc40_dll\Debug\md5module.obj"
	-@erase ".\vc40_dll\Debug\traceback.obj"
	-@erase ".\vc40_dll\Debug\sysmodule.obj"
	-@erase ".\vc40_dll\Debug\marshal.obj"
	-@erase ".\vc40_dll\Debug\bltinmodule.obj"
	-@erase ".\vc40_dll\Debug\getcopyright.obj"
	-@erase ".\vc40_dll\Debug\pythonrun.obj"
	-@erase ".\vc40_dll\Debug\ceval.obj"
	-@erase ".\vc40_dll\Debug\config.obj"
	-@erase ".\vc40_dll\Debug\rangeobject.obj"
	-@erase ".\vc40_dll\Debug\mappingobject.obj"
	-@erase ".\vc40_dll\Debug\getargs.obj"
	-@erase ".\vc40_dll\Debug\frameobject.obj"
	-@erase ".\vc40_dll\Debug\abstract.obj"
	-@erase ".\vc40_dll\Debug\regexmodule.obj"
	-@erase ".\vc40_dll\Debug\threadmodule.obj"
	-@erase ".\vc40_dll\Debug\fileobject.obj"
	-@erase ".\vc40_dll\Debug\longobject.obj"
	-@erase ".\vc40_dll\Debug\posixmodule.obj"
	-@erase ".\vc40_dll\Debug\cmathmodule.obj"
	-@erase ".\vc40_dll\Debug\cgensupport.obj"
	-@erase ".\vc40_dll\Debug\getplatform.obj"
	-@erase ".\vc40_dll\Debug\selectmodule.obj"
	-@erase ".\vc40_dll\Debug\signalmodule.obj"
	-@erase ".\vc40_dll\Debug\parsetok.obj"
	-@erase ".\vc40_dll\Debug\accessobject.obj"
	-@erase ".\vc40_dll\Debug\import.obj"
	-@erase ".\vc40_dll\Debug\acceler.obj"
	-@erase ".\vc40_dll\Debug\errors.obj"
	-@erase ".\vc40_dll\Debug\timemodule.obj"
	-@erase ".\vc40_dll\Debug\regexpr.obj"
	-@erase ".\vc40_dll\Debug\newmodule.obj"
	-@erase ".\vc40_dll\Debug\stringobject.obj"
	-@erase ".\vc40_dll\Debug\rotormodule.obj"
	-@erase ".\vc40_dll\Debug\parser.obj"
	-@erase ".\vc40_dll\Debug\complexobject.obj"
	-@erase ".\vc40_dll\Debug\getpath.obj"
	-@erase ".\vc40_dll\Debug\importdl.obj"
	-@erase ".\vc40_dll\Debug\classobject.obj"
	-@erase ".\vc40_dll\Debug\listobject.obj"
	-@erase ".\vc40_dll\Debug\typeobject.obj"
	-@erase ".\vc40_dll\Debug\binascii.obj"
	-@erase ".\vc40_dll\Debug\mystrtoul.obj"
	-@erase ".\vc40_dll\Debug\getcompiler.obj"
	-@erase ".\vc40_dll\Debug\yuvconvert.obj"
	-@erase ".\vc40_dll\Debug\getversion.obj"
	-@erase ".\vc40_dll\Debug\structmodule.obj"
	-@erase ".\vc40_dll\Debug\audioop.obj"
	-@erase ".\vc40_dll\Debug\mathmodule.obj"
	-@erase ".\vc40_dll\Debug\compile.obj"
	-@erase ".\vc40_dll\Debug\errnomodule.obj"
	-@erase ".\vc40_dll\Debug\cobject.obj"
	-@erase ".\vc40_dll\Debug\methodobject.obj"
	-@erase ".\vc40_dll\Debug\md5c.obj"
	-@erase ".\vc40_dll\Debug\grammar1.obj"
	-@erase ".\vc40_dll\Debug\getmtime.obj"
	-@erase ".\vc40_dll\Debug\rgbimgmodule.obj"
	-@erase ".\vc40_dll\Debug\thread.obj"
	-@erase ".\vc40_dll\Debug\operator.obj"
	-@erase ".\vc40_dll\Debug\environment.obj"
	-@erase ".\vc40_dll\Debug\floatobject.obj"
	-@erase ".\vc40_dll\Debug\moduleobject.obj"
	-@erase ".\vc40_dll\Debug\object.obj"
	-@erase ".\vc40_dll\Debug\py14an.ilk"
	-@erase ".\vc40_dll\Debug\py14an.lib"
	-@erase ".\vc40_dll\Debug\py14an.exp"
	-@erase ".\vc40_dll\Debug\py14an.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "./PC" /I "./Include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /D "USE_DL_EXPORT" /YX /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I "./PC" /I "./Include" /D "_DEBUG"\
 /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /D "USE_DL_EXPORT"\
 /Fp"$(INTDIR)/vc40_dll.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
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
	"$(INTDIR)/node.obj" \
	"$(INTDIR)/arraymodule.obj" \
	"$(INTDIR)/funcobject.obj" \
	"$(INTDIR)/structmember.obj" \
	"$(INTDIR)/soundex.obj" \
	"$(INTDIR)/tokenizer.obj" \
	"$(INTDIR)/myreadline.obj" \
	"$(INTDIR)/intobject.obj" \
	"$(INTDIR)/tupleobject.obj" \
	"$(INTDIR)/frozen.obj" \
	"$(INTDIR)/socketmodule.obj" \
	"$(INTDIR)/imageop.obj" \
	"$(INTDIR)/graminit.obj" \
	"$(INTDIR)/stropmodule.obj" \
	"$(INTDIR)/modsupport.obj" \
	"$(INTDIR)/md5module.obj" \
	"$(INTDIR)/traceback.obj" \
	"$(INTDIR)/sysmodule.obj" \
	"$(INTDIR)/marshal.obj" \
	"$(INTDIR)/bltinmodule.obj" \
	"$(INTDIR)/getcopyright.obj" \
	"$(INTDIR)/pythonrun.obj" \
	"$(INTDIR)/ceval.obj" \
	"$(INTDIR)/config.obj" \
	"$(INTDIR)/rangeobject.obj" \
	"$(INTDIR)/mappingobject.obj" \
	"$(INTDIR)/getargs.obj" \
	"$(INTDIR)/frameobject.obj" \
	"$(INTDIR)/abstract.obj" \
	"$(INTDIR)/regexmodule.obj" \
	"$(INTDIR)/threadmodule.obj" \
	"$(INTDIR)/fileobject.obj" \
	"$(INTDIR)/longobject.obj" \
	"$(INTDIR)/posixmodule.obj" \
	"$(INTDIR)/cmathmodule.obj" \
	"$(INTDIR)/cgensupport.obj" \
	"$(INTDIR)/getplatform.obj" \
	"$(INTDIR)/selectmodule.obj" \
	"$(INTDIR)/signalmodule.obj" \
	"$(INTDIR)/parsetok.obj" \
	"$(INTDIR)/accessobject.obj" \
	"$(INTDIR)/import.obj" \
	"$(INTDIR)/acceler.obj" \
	"$(INTDIR)/errors.obj" \
	"$(INTDIR)/timemodule.obj" \
	"$(INTDIR)/regexpr.obj" \
	"$(INTDIR)/newmodule.obj" \
	"$(INTDIR)/stringobject.obj" \
	"$(INTDIR)/rotormodule.obj" \
	"$(INTDIR)/parser.obj" \
	"$(INTDIR)/complexobject.obj" \
	"$(INTDIR)/getpath.obj" \
	"$(INTDIR)/importdl.obj" \
	"$(INTDIR)/classobject.obj" \
	"$(INTDIR)/listobject.obj" \
	"$(INTDIR)/typeobject.obj" \
	"$(INTDIR)/binascii.obj" \
	"$(INTDIR)/mystrtoul.obj" \
	"$(INTDIR)/getcompiler.obj" \
	"$(INTDIR)/yuvconvert.obj" \
	"$(INTDIR)/getversion.obj" \
	"$(INTDIR)/structmodule.obj" \
	"$(INTDIR)/audioop.obj" \
	"$(INTDIR)/mathmodule.obj" \
	"$(INTDIR)/compile.obj" \
	"$(INTDIR)/errnomodule.obj" \
	"$(INTDIR)/cobject.obj" \
	"$(INTDIR)/methodobject.obj" \
	"$(INTDIR)/md5c.obj" \
	"$(INTDIR)/grammar1.obj" \
	"$(INTDIR)/getmtime.obj" \
	"$(INTDIR)/rgbimgmodule.obj" \
	"$(INTDIR)/thread.obj" \
	"$(INTDIR)/operator.obj" \
	"$(INTDIR)/environment.obj" \
	"$(INTDIR)/floatobject.obj" \
	"$(INTDIR)/moduleobject.obj" \
	"$(INTDIR)/object.obj"

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
	-@erase ".\vc40_nt\Release\binascii.obj"
	-@erase ".\vc40_nt\Release\getpath.obj"
	-@erase ".\vc40_nt\Release\import.obj"
	-@erase ".\vc40_nt\Release\getplatform.obj"
	-@erase ".\vc40_nt\Release\getcopyright.obj"
	-@erase ".\vc40_nt\Release\yuvconvert.obj"
	-@erase ".\vc40_nt\Release\getversion.obj"
	-@erase ".\vc40_nt\Release\marshal.obj"
	-@erase ".\vc40_nt\Release\mathmodule.obj"
	-@erase ".\vc40_nt\Release\md5c.obj"
	-@erase ".\vc40_nt\Release\grammar1.obj"
	-@erase ".\vc40_nt\Release\getmtime.obj"
	-@erase ".\vc40_nt\Release\parser.obj"
	-@erase ".\vc40_nt\Release\operator.obj"
	-@erase ".\vc40_nt\Release\compile.obj"
	-@erase ".\vc40_nt\Release\threadmodule.obj"
	-@erase ".\vc40_nt\Release\cobject.obj"
	-@erase ".\vc40_nt\Release\rotormodule.obj"
	-@erase ".\vc40_nt\Release\tupleobject.obj"
	-@erase ".\vc40_nt\Release\newmodule.obj"
	-@erase ".\vc40_nt\Release\config.obj"
	-@erase ".\vc40_nt\Release\rgbimgmodule.obj"
	-@erase ".\vc40_nt\Release\classobject.obj"
	-@erase ".\vc40_nt\Release\intobject.obj"
	-@erase ".\vc40_nt\Release\funcobject.obj"
	-@erase ".\vc40_nt\Release\mappingobject.obj"
	-@erase ".\vc40_nt\Release\selectmodule.obj"
	-@erase ".\vc40_nt\Release\signalmodule.obj"
	-@erase ".\vc40_nt\Release\bltinmodule.obj"
	-@erase ".\vc40_nt\Release\getcompiler.obj"
	-@erase ".\vc40_nt\Release\myreadline.obj"
	-@erase ".\vc40_nt\Release\main.obj"
	-@erase ".\vc40_nt\Release\accessobject.obj"
	-@erase ".\vc40_nt\Release\thread.obj"
	-@erase ".\vc40_nt\Release\mystrtoul.obj"
	-@erase ".\vc40_nt\Release\graminit.obj"
	-@erase ".\vc40_nt\Release\errnomodule.obj"
	-@erase ".\vc40_nt\Release\frameobject.obj"
	-@erase ".\vc40_nt\Release\object.obj"
	-@erase ".\vc40_nt\Release\regexmodule.obj"
	-@erase ".\vc40_nt\Release\environment.obj"
	-@erase ".\vc40_nt\Release\floatobject.obj"
	-@erase ".\vc40_nt\Release\imageop.obj"
	-@erase ".\vc40_nt\Release\regexpr.obj"
	-@erase ".\vc40_nt\Release\cmathmodule.obj"
	-@erase ".\vc40_nt\Release\arraymodule.obj"
	-@erase ".\vc40_nt\Release\frozen.obj"
	-@erase ".\vc40_nt\Release\abstract.obj"
	-@erase ".\vc40_nt\Release\getopt.obj"
	-@erase ".\vc40_nt\Release\getargs.obj"
	-@erase ".\vc40_nt\Release\errors.obj"
	-@erase ".\vc40_nt\Release\structmodule.obj"
	-@erase ".\vc40_nt\Release\fileobject.obj"
	-@erase ".\vc40_nt\Release\longobject.obj"
	-@erase ".\vc40_nt\Release\ceval.obj"
	-@erase ".\vc40_nt\Release\methodobject.obj"
	-@erase ".\vc40_nt\Release\node.obj"
	-@erase ".\vc40_nt\Release\tokenizer.obj"
	-@erase ".\vc40_nt\Release\audioop.obj"
	-@erase ".\vc40_nt\Release\complexobject.obj"
	-@erase ".\vc40_nt\Release\stropmodule.obj"
	-@erase ".\vc40_nt\Release\moduleobject.obj"
	-@erase ".\vc40_nt\Release\parsetok.obj"
	-@erase ".\vc40_nt\Release\md5module.obj"
	-@erase ".\vc40_nt\Release\traceback.obj"
	-@erase ".\vc40_nt\Release\sysmodule.obj"
	-@erase ".\vc40_nt\Release\rangeobject.obj"
	-@erase ".\vc40_nt\Release\timemodule.obj"
	-@erase ".\vc40_nt\Release\pythonrun.obj"
	-@erase ".\vc40_nt\Release\structmember.obj"
	-@erase ".\vc40_nt\Release\acceler.obj"
	-@erase ".\vc40_nt\Release\socketmodule.obj"
	-@erase ".\vc40_nt\Release\posixmodule.obj"
	-@erase ".\vc40_nt\Release\soundex.obj"
	-@erase ".\vc40_nt\Release\importdl.obj"
	-@erase ".\vc40_nt\Release\cgensupport.obj"
	-@erase ".\vc40_nt\Release\modsupport.obj"
	-@erase ".\vc40_nt\Release\stringobject.obj"
	-@erase ".\vc40_nt\Release\listobject.obj"
	-@erase ".\vc40_nt\Release\typeobject.obj"

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
	"$(INTDIR)/binascii.obj" \
	"$(INTDIR)/getpath.obj" \
	"$(INTDIR)/import.obj" \
	"$(INTDIR)/getplatform.obj" \
	"$(INTDIR)/getcopyright.obj" \
	"$(INTDIR)/yuvconvert.obj" \
	"$(INTDIR)/getversion.obj" \
	"$(INTDIR)/marshal.obj" \
	"$(INTDIR)/mathmodule.obj" \
	"$(INTDIR)/md5c.obj" \
	"$(INTDIR)/grammar1.obj" \
	"$(INTDIR)/getmtime.obj" \
	"$(INTDIR)/parser.obj" \
	"$(INTDIR)/operator.obj" \
	"$(INTDIR)/compile.obj" \
	"$(INTDIR)/threadmodule.obj" \
	"$(INTDIR)/cobject.obj" \
	"$(INTDIR)/rotormodule.obj" \
	"$(INTDIR)/tupleobject.obj" \
	"$(INTDIR)/newmodule.obj" \
	"$(INTDIR)/config.obj" \
	"$(INTDIR)/rgbimgmodule.obj" \
	"$(INTDIR)/classobject.obj" \
	"$(INTDIR)/intobject.obj" \
	"$(INTDIR)/funcobject.obj" \
	"$(INTDIR)/mappingobject.obj" \
	"$(INTDIR)/selectmodule.obj" \
	"$(INTDIR)/signalmodule.obj" \
	"$(INTDIR)/bltinmodule.obj" \
	"$(INTDIR)/getcompiler.obj" \
	"$(INTDIR)/myreadline.obj" \
	"$(INTDIR)/main.obj" \
	"$(INTDIR)/accessobject.obj" \
	"$(INTDIR)/thread.obj" \
	"$(INTDIR)/mystrtoul.obj" \
	"$(INTDIR)/graminit.obj" \
	"$(INTDIR)/errnomodule.obj" \
	"$(INTDIR)/frameobject.obj" \
	"$(INTDIR)/object.obj" \
	"$(INTDIR)/regexmodule.obj" \
	"$(INTDIR)/environment.obj" \
	"$(INTDIR)/floatobject.obj" \
	"$(INTDIR)/imageop.obj" \
	"$(INTDIR)/regexpr.obj" \
	"$(INTDIR)/cmathmodule.obj" \
	"$(INTDIR)/arraymodule.obj" \
	"$(INTDIR)/frozen.obj" \
	"$(INTDIR)/abstract.obj" \
	"$(INTDIR)/getopt.obj" \
	"$(INTDIR)/getargs.obj" \
	"$(INTDIR)/errors.obj" \
	"$(INTDIR)/structmodule.obj" \
	"$(INTDIR)/fileobject.obj" \
	"$(INTDIR)/longobject.obj" \
	"$(INTDIR)/ceval.obj" \
	"$(INTDIR)/methodobject.obj" \
	"$(INTDIR)/node.obj" \
	"$(INTDIR)/tokenizer.obj" \
	"$(INTDIR)/audioop.obj" \
	"$(INTDIR)/complexobject.obj" \
	"$(INTDIR)/stropmodule.obj" \
	"$(INTDIR)/moduleobject.obj" \
	"$(INTDIR)/parsetok.obj" \
	"$(INTDIR)/md5module.obj" \
	"$(INTDIR)/traceback.obj" \
	"$(INTDIR)/sysmodule.obj" \
	"$(INTDIR)/rangeobject.obj" \
	"$(INTDIR)/timemodule.obj" \
	"$(INTDIR)/pythonrun.obj" \
	"$(INTDIR)/structmember.obj" \
	"$(INTDIR)/acceler.obj" \
	"$(INTDIR)/socketmodule.obj" \
	"$(INTDIR)/posixmodule.obj" \
	"$(INTDIR)/soundex.obj" \
	"$(INTDIR)/importdl.obj" \
	"$(INTDIR)/cgensupport.obj" \
	"$(INTDIR)/modsupport.obj" \
	"$(INTDIR)/stringobject.obj" \
	"$(INTDIR)/listobject.obj" \
	"$(INTDIR)/typeobject.obj"

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
	-@erase ".\vc40_nt\Debug\config.obj"
	-@erase ".\vc40_nt\Debug\myreadline.obj"
	-@erase ".\vc40_nt\Debug\moduleobject.obj"
	-@erase ".\vc40_nt\Debug\grammar1.obj"
	-@erase ".\vc40_nt\Debug\abstract.obj"
	-@erase ".\vc40_nt\Debug\acceler.obj"
	-@erase ".\vc40_nt\Debug\thread.obj"
	-@erase ".\vc40_nt\Debug\stropmodule.obj"
	-@erase ".\vc40_nt\Debug\soundex.obj"
	-@erase ".\vc40_nt\Debug\object.obj"
	-@erase ".\vc40_nt\Debug\structmember.obj"
	-@erase ".\vc40_nt\Debug\newmodule.obj"
	-@erase ".\vc40_nt\Debug\rangeobject.obj"
	-@erase ".\vc40_nt\Debug\md5module.obj"
	-@erase ".\vc40_nt\Debug\socketmodule.obj"
	-@erase ".\vc40_nt\Debug\getpath.obj"
	-@erase ".\vc40_nt\Debug\typeobject.obj"
	-@erase ".\vc40_nt\Debug\parsetok.obj"
	-@erase ".\vc40_nt\Debug\stringobject.obj"
	-@erase ".\vc40_nt\Debug\marshal.obj"
	-@erase ".\vc40_nt\Debug\frozen.obj"
	-@erase ".\vc40_nt\Debug\getversion.obj"
	-@erase ".\vc40_nt\Debug\getopt.obj"
	-@erase ".\vc40_nt\Debug\posixmodule.obj"
	-@erase ".\vc40_nt\Debug\md5c.obj"
	-@erase ".\vc40_nt\Debug\getcopyright.obj"
	-@erase ".\vc40_nt\Debug\mystrtoul.obj"
	-@erase ".\vc40_nt\Debug\longobject.obj"
	-@erase ".\vc40_nt\Debug\cgensupport.obj"
	-@erase ".\vc40_nt\Debug\compile.obj"
	-@erase ".\vc40_nt\Debug\cobject.obj"
	-@erase ".\vc40_nt\Debug\getplatform.obj"
	-@erase ".\vc40_nt\Debug\importdl.obj"
	-@erase ".\vc40_nt\Debug\threadmodule.obj"
	-@erase ".\vc40_nt\Debug\binascii.obj"
	-@erase ".\vc40_nt\Debug\ceval.obj"
	-@erase ".\vc40_nt\Debug\timemodule.obj"
	-@erase ".\vc40_nt\Debug\main.obj"
	-@erase ".\vc40_nt\Debug\methodobject.obj"
	-@erase ".\vc40_nt\Debug\mappingobject.obj"
	-@erase ".\vc40_nt\Debug\funcobject.obj"
	-@erase ".\vc40_nt\Debug\rgbimgmodule.obj"
	-@erase ".\vc40_nt\Debug\rotormodule.obj"
	-@erase ".\vc40_nt\Debug\selectmodule.obj"
	-@erase ".\vc40_nt\Debug\tupleobject.obj"
	-@erase ".\vc40_nt\Debug\signalmodule.obj"
	-@erase ".\vc40_nt\Debug\getmtime.obj"
	-@erase ".\vc40_nt\Debug\operator.obj"
	-@erase ".\vc40_nt\Debug\tokenizer.obj"
	-@erase ".\vc40_nt\Debug\accessobject.obj"
	-@erase ".\vc40_nt\Debug\classobject.obj"
	-@erase ".\vc40_nt\Debug\imageop.obj"
	-@erase ".\vc40_nt\Debug\intobject.obj"
	-@erase ".\vc40_nt\Debug\regexpr.obj"
	-@erase ".\vc40_nt\Debug\modsupport.obj"
	-@erase ".\vc40_nt\Debug\listobject.obj"
	-@erase ".\vc40_nt\Debug\bltinmodule.obj"
	-@erase ".\vc40_nt\Debug\getcompiler.obj"
	-@erase ".\vc40_nt\Debug\import.obj"
	-@erase ".\vc40_nt\Debug\yuvconvert.obj"
	-@erase ".\vc40_nt\Debug\traceback.obj"
	-@erase ".\vc40_nt\Debug\sysmodule.obj"
	-@erase ".\vc40_nt\Debug\errors.obj"
	-@erase ".\vc40_nt\Debug\mathmodule.obj"
	-@erase ".\vc40_nt\Debug\errnomodule.obj"
	-@erase ".\vc40_nt\Debug\pythonrun.obj"
	-@erase ".\vc40_nt\Debug\getargs.obj"
	-@erase ".\vc40_nt\Debug\frameobject.obj"
	-@erase ".\vc40_nt\Debug\regexmodule.obj"
	-@erase ".\vc40_nt\Debug\environment.obj"
	-@erase ".\vc40_nt\Debug\floatobject.obj"
	-@erase ".\vc40_nt\Debug\parser.obj"
	-@erase ".\vc40_nt\Debug\node.obj"
	-@erase ".\vc40_nt\Debug\cmathmodule.obj"
	-@erase ".\vc40_nt\Debug\fileobject.obj"
	-@erase ".\vc40_nt\Debug\graminit.obj"
	-@erase ".\vc40_nt\Debug\audioop.obj"
	-@erase ".\vc40_nt\Debug\structmodule.obj"
	-@erase ".\vc40_nt\Debug\arraymodule.obj"
	-@erase ".\vc40_nt\Debug\complexobject.obj"
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
	"$(INTDIR)/config.obj" \
	"$(INTDIR)/myreadline.obj" \
	"$(INTDIR)/moduleobject.obj" \
	"$(INTDIR)/grammar1.obj" \
	"$(INTDIR)/abstract.obj" \
	"$(INTDIR)/acceler.obj" \
	"$(INTDIR)/thread.obj" \
	"$(INTDIR)/stropmodule.obj" \
	"$(INTDIR)/soundex.obj" \
	"$(INTDIR)/object.obj" \
	"$(INTDIR)/structmember.obj" \
	"$(INTDIR)/newmodule.obj" \
	"$(INTDIR)/rangeobject.obj" \
	"$(INTDIR)/md5module.obj" \
	"$(INTDIR)/socketmodule.obj" \
	"$(INTDIR)/getpath.obj" \
	"$(INTDIR)/typeobject.obj" \
	"$(INTDIR)/parsetok.obj" \
	"$(INTDIR)/stringobject.obj" \
	"$(INTDIR)/marshal.obj" \
	"$(INTDIR)/frozen.obj" \
	"$(INTDIR)/getversion.obj" \
	"$(INTDIR)/getopt.obj" \
	"$(INTDIR)/posixmodule.obj" \
	"$(INTDIR)/md5c.obj" \
	"$(INTDIR)/getcopyright.obj" \
	"$(INTDIR)/mystrtoul.obj" \
	"$(INTDIR)/longobject.obj" \
	"$(INTDIR)/cgensupport.obj" \
	"$(INTDIR)/compile.obj" \
	"$(INTDIR)/cobject.obj" \
	"$(INTDIR)/getplatform.obj" \
	"$(INTDIR)/importdl.obj" \
	"$(INTDIR)/threadmodule.obj" \
	"$(INTDIR)/binascii.obj" \
	"$(INTDIR)/ceval.obj" \
	"$(INTDIR)/timemodule.obj" \
	"$(INTDIR)/main.obj" \
	"$(INTDIR)/methodobject.obj" \
	"$(INTDIR)/mappingobject.obj" \
	"$(INTDIR)/funcobject.obj" \
	"$(INTDIR)/rgbimgmodule.obj" \
	"$(INTDIR)/rotormodule.obj" \
	"$(INTDIR)/selectmodule.obj" \
	"$(INTDIR)/tupleobject.obj" \
	"$(INTDIR)/signalmodule.obj" \
	"$(INTDIR)/getmtime.obj" \
	"$(INTDIR)/operator.obj" \
	"$(INTDIR)/tokenizer.obj" \
	"$(INTDIR)/accessobject.obj" \
	"$(INTDIR)/classobject.obj" \
	"$(INTDIR)/imageop.obj" \
	"$(INTDIR)/intobject.obj" \
	"$(INTDIR)/regexpr.obj" \
	"$(INTDIR)/modsupport.obj" \
	"$(INTDIR)/listobject.obj" \
	"$(INTDIR)/bltinmodule.obj" \
	"$(INTDIR)/getcompiler.obj" \
	"$(INTDIR)/import.obj" \
	"$(INTDIR)/yuvconvert.obj" \
	"$(INTDIR)/traceback.obj" \
	"$(INTDIR)/sysmodule.obj" \
	"$(INTDIR)/errors.obj" \
	"$(INTDIR)/mathmodule.obj" \
	"$(INTDIR)/errnomodule.obj" \
	"$(INTDIR)/pythonrun.obj" \
	"$(INTDIR)/getargs.obj" \
	"$(INTDIR)/frameobject.obj" \
	"$(INTDIR)/regexmodule.obj" \
	"$(INTDIR)/environment.obj" \
	"$(INTDIR)/floatobject.obj" \
	"$(INTDIR)/parser.obj" \
	"$(INTDIR)/node.obj" \
	"$(INTDIR)/cmathmodule.obj" \
	"$(INTDIR)/fileobject.obj" \
	"$(INTDIR)/graminit.obj" \
	"$(INTDIR)/audioop.obj" \
	"$(INTDIR)/structmodule.obj" \
	"$(INTDIR)/arraymodule.obj" \
	"$(INTDIR)/complexobject.obj"

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

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\longobject.obj" : $(SOURCE) $(DEP_CPP_LONGO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\longobject.obj" : $(SOURCE) $(DEP_CPP_LONGO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\listobject.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\listobject.obj" : $(SOURCE) $(DEP_CPP_LISTO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\listobject.obj" : $(SOURCE) $(DEP_CPP_LISTO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\intobject.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\intobject.obj" : $(SOURCE) $(DEP_CPP_INTOB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\intobject.obj" : $(SOURCE) $(DEP_CPP_INTOB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\importdl.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\importdl.obj" : $(SOURCE) $(DEP_CPP_IMPOR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	
NODEP_CPP_IMPOR=\
	".\Python\dl.h"\
	".\Python\macdefs.h"\
	".\Python\macglue.h"\
	

"$(INTDIR)\importdl.obj" : $(SOURCE) $(DEP_CPP_IMPOR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\imageop.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\imageop.obj" : $(SOURCE) $(DEP_CPP_IMAGE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\imageop.obj" : $(SOURCE) $(DEP_CPP_IMAGE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\grammar1.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\grammar1.obj" : $(SOURCE) $(DEP_CPP_GRAMM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

DEP_CPP_GRAMM=\
	".\./Include\pgenheaders.h"\
	".\./Include\grammar.h"\
	".\./Include\token.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\./Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\./Include\rename2.h"\
	".\Include\bitset.h"\
	

"$(INTDIR)\grammar1.obj" : $(SOURCE) $(DEP_CPP_GRAMM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\graminit.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

DEP_CPP_GRAMI=\
	".\./Include\pgenheaders.h"\
	".\./Include\grammar.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\./Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\Include\bitset.h"\
	".\./Include\rename2.h"\
	

"$(INTDIR)\graminit.obj" : $(SOURCE) $(DEP_CPP_GRAMI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

DEP_CPP_GRAMI=\
	".\./Include\pgenheaders.h"\
	".\./Include\grammar.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\./Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\./Include\rename2.h"\
	".\Include\bitset.h"\
	

"$(INTDIR)\graminit.obj" : $(SOURCE) $(DEP_CPP_GRAMI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getversion.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\getversion.obj" : $(SOURCE) $(DEP_CPP_GETVE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\getversion.obj" : $(SOURCE) $(DEP_CPP_GETVE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getplatform.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\getplatform.obj" : $(SOURCE) $(DEP_CPP_GETPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\getplatform.obj" : $(SOURCE) $(DEP_CPP_GETPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\getpath.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\getpath.obj" : $(SOURCE) $(DEP_CPP_GETPA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

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
	

"$(INTDIR)\getmtime.obj" : $(SOURCE) $(DEP_CPP_GETMT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getcopyright.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\getcopyright.obj" : $(SOURCE) $(DEP_CPP_GETCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\getcopyright.obj" : $(SOURCE) $(DEP_CPP_GETCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getcompiler.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\getcompiler.obj" : $(SOURCE) $(DEP_CPP_GETCOM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\getcompiler.obj" : $(SOURCE) $(DEP_CPP_GETCOM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getargs.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\getargs.obj" : $(SOURCE) $(DEP_CPP_GETAR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\getargs.obj" : $(SOURCE) $(DEP_CPP_GETAR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\funcobject.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\funcobject.obj" : $(SOURCE) $(DEP_CPP_FUNCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\funcobject.obj" : $(SOURCE) $(DEP_CPP_FUNCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\frozen.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\frozen.obj" : $(SOURCE) $(DEP_CPP_FROZE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\frozen.obj" : $(SOURCE) $(DEP_CPP_FROZE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\frameobject.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\frameobject.obj" : $(SOURCE) $(DEP_CPP_FRAME) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\frameobject.obj" : $(SOURCE) $(DEP_CPP_FRAME) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\floatobject.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\floatobject.obj" : $(SOURCE) $(DEP_CPP_FLOAT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\floatobject.obj" : $(SOURCE) $(DEP_CPP_FLOAT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\fileobject.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\fileobject.obj" : $(SOURCE) $(DEP_CPP_FILEO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\fileobject.obj" : $(SOURCE) $(DEP_CPP_FILEO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\errors.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\errors.obj" : $(SOURCE) $(DEP_CPP_ERROR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\errors.obj" : $(SOURCE) $(DEP_CPP_ERROR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\environment.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\environment.obj" : $(SOURCE) $(DEP_CPP_ENVIR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\environment.obj" : $(SOURCE) $(DEP_CPP_ENVIR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\config.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\config.obj" : $(SOURCE) $(DEP_CPP_CONFI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\config.obj" : $(SOURCE) $(DEP_CPP_CONFI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\complexobject.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\complexobject.obj" : $(SOURCE) $(DEP_CPP_COMPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\complexobject.obj" : $(SOURCE) $(DEP_CPP_COMPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\compile.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\compile.obj" : $(SOURCE) $(DEP_CPP_COMPI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\compile.obj" : $(SOURCE) $(DEP_CPP_COMPI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\cobject.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\cobject.obj" : $(SOURCE) $(DEP_CPP_COBJE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\cobject.obj" : $(SOURCE) $(DEP_CPP_COBJE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\cmathmodule.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

DEP_CPP_CMATH=\
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
	
NODEP_CPP_CMATH=\
	".\Modules\complexobject.h"\
	

"$(INTDIR)\cmathmodule.obj" : $(SOURCE) $(DEP_CPP_CMATH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\cmathmodule.obj" : $(SOURCE) $(DEP_CPP_CMATH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\classobject.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\classobject.obj" : $(SOURCE) $(DEP_CPP_CLASS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\classobject.obj" : $(SOURCE) $(DEP_CPP_CLASS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\cgensupport.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\cgensupport.obj" : $(SOURCE) $(DEP_CPP_CGENS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\cgensupport.obj" : $(SOURCE) $(DEP_CPP_CGENS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\ceval.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\ceval.obj" : $(SOURCE) $(DEP_CPP_CEVAL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	

"$(INTDIR)\ceval.obj" : $(SOURCE) $(DEP_CPP_CEVAL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\bltinmodule.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\bltinmodule.obj" : $(SOURCE) $(DEP_CPP_BLTIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\bltinmodule.obj" : $(SOURCE) $(DEP_CPP_BLTIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\binascii.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\binascii.obj" : $(SOURCE) $(DEP_CPP_BINAS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\binascii.obj" : $(SOURCE) $(DEP_CPP_BINAS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\audioop.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\audioop.obj" : $(SOURCE) $(DEP_CPP_AUDIO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\audioop.obj" : $(SOURCE) $(DEP_CPP_AUDIO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\arraymodule.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\arraymodule.obj" : $(SOURCE) $(DEP_CPP_ARRAY) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\arraymodule.obj" : $(SOURCE) $(DEP_CPP_ARRAY) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\accessobject.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\accessobject.obj" : $(SOURCE) $(DEP_CPP_ACCES) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

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
	".\./Include\rename2.h"\
	".\Include\bitset.h"\
	

"$(INTDIR)\acceler.obj" : $(SOURCE) $(DEP_CPP_ACCEL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\abstract.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\abstract.obj" : $(SOURCE) $(DEP_CPP_ABSTR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\abstract.obj" : $(SOURCE) $(DEP_CPP_ABSTR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\yuvconvert.c
DEP_CPP_YUVCO=\
	".\Modules\yuv.h"\
	

"$(INTDIR)\yuvconvert.obj" : $(SOURCE) $(DEP_CPP_YUVCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\typeobject.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\typeobject.obj" : $(SOURCE) $(DEP_CPP_TYPEO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\typeobject.obj" : $(SOURCE) $(DEP_CPP_TYPEO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\tupleobject.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\tupleobject.obj" : $(SOURCE) $(DEP_CPP_TUPLE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\tupleobject.obj" : $(SOURCE) $(DEP_CPP_TUPLE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\traceback.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

DEP_CPP_TRACE=\
	".\./Include\allobjects.h"\
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
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	
NODEP_CPP_TRACE=\
	".\Python\sysmodule.h"\
	

"$(INTDIR)\traceback.obj" : $(SOURCE) $(DEP_CPP_TRACE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

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
	".\./Include\rename2.h"\
	".\./Include\token.h"\
	

"$(INTDIR)\tokenizer.obj" : $(SOURCE) $(DEP_CPP_TOKEN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\timemodule.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

DEP_CPP_TIMEM=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	{$(INCLUDE)}"\unistd.h"\
	".\./Include\myselect.h"\
	".\./Include\mytime.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\i86.h"\
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
	

"$(INTDIR)\timemodule.obj" : $(SOURCE) $(DEP_CPP_TIMEM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

DEP_CPP_TIMEM=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	{$(INCLUDE)}"\unistd.h"\
	".\./Include\myselect.h"\
	".\./Include\mytime.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\i86.h"\
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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\timemodule.obj" : $(SOURCE) $(DEP_CPP_TIMEM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\threadmodule.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

DEP_CPP_THREA=\
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
	
NODEP_CPP_THREA=\
	".\Modules\thread.h"\
	

"$(INTDIR)\threadmodule.obj" : $(SOURCE) $(DEP_CPP_THREA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	

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
	{$(INCLUDE)}"\sys\TYPES.H"\
	{$(INCLUDE)}"\unistd.h"\
	

"$(INTDIR)\thread.obj" : $(SOURCE) $(DEP_CPP_THREAD) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\structmodule.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\structmodule.obj" : $(SOURCE) $(DEP_CPP_STRUC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\structmodule.obj" : $(SOURCE) $(DEP_CPP_STRUC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\structmember.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\structmember.obj" : $(SOURCE) $(DEP_CPP_STRUCT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\structmember.obj" : $(SOURCE) $(DEP_CPP_STRUCT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\stropmodule.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\stropmodule.obj" : $(SOURCE) $(DEP_CPP_STROP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\stropmodule.obj" : $(SOURCE) $(DEP_CPP_STROP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\stringobject.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\stringobject.obj" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\stringobject.obj" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\soundex.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\soundex.obj" : $(SOURCE) $(DEP_CPP_SOUND) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\soundex.obj" : $(SOURCE) $(DEP_CPP_SOUND) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\signalmodule.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

DEP_CPP_SIGNA=\
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
	
NODEP_CPP_SIGNA=\
	".\Modules\intrcheck.h"\
	".\Modules\thread.h"\
	

"$(INTDIR)\signalmodule.obj" : $(SOURCE) $(DEP_CPP_SIGNA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	

"$(INTDIR)\signalmodule.obj" : $(SOURCE) $(DEP_CPP_SIGNA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\rotormodule.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\rotormodule.obj" : $(SOURCE) $(DEP_CPP_ROTOR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\rotormodule.obj" : $(SOURCE) $(DEP_CPP_ROTOR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\rgbimgmodule.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

DEP_CPP_RGBIM=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	{$(INCLUDE)}"\unistd.h"\
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
	

"$(INTDIR)\rgbimgmodule.obj" : $(SOURCE) $(DEP_CPP_RGBIM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

DEP_CPP_RGBIM=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	{$(INCLUDE)}"\unistd.h"\
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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

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
	

"$(INTDIR)\regexpr.obj" : $(SOURCE) $(DEP_CPP_REGEX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\regexmodule.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

DEP_CPP_REGEXM=\
	".\./Include\Python.h"\
	".\Modules\regexpr.h"\
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
	

"$(INTDIR)\regexmodule.obj" : $(SOURCE) $(DEP_CPP_REGEXM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

DEP_CPP_REGEXM=\
	".\./Include\Python.h"\
	".\Modules\regexpr.h"\
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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\regexmodule.obj" : $(SOURCE) $(DEP_CPP_REGEXM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\rangeobject.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\rangeobject.obj" : $(SOURCE) $(DEP_CPP_RANGE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\rangeobject.obj" : $(SOURCE) $(DEP_CPP_RANGE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\pythonrun.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\pythonrun.obj" : $(SOURCE) $(DEP_CPP_PYTHO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\Include\bitset.h"\
	

"$(INTDIR)\pythonrun.obj" : $(SOURCE) $(DEP_CPP_PYTHO) "$(INTDIR)"
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
	".\./Include\rename2.h"\
	".\./Include\token.h"\
	".\Include\bitset.h"\
	

"$(INTDIR)\parsetok.obj" : $(SOURCE) $(DEP_CPP_PARSE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


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
	".\./Include\rename2.h"\
	".\Include\bitset.h"\
	

"$(INTDIR)\parser.obj" : $(SOURCE) $(DEP_CPP_PARSER) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\object.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\object.obj" : $(SOURCE) $(DEP_CPP_OBJEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

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
	

"$(INTDIR)\node.obj" : $(SOURCE) $(DEP_CPP_NODE_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\newmodule.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\newmodule.obj" : $(SOURCE) $(DEP_CPP_NEWMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\newmodule.obj" : $(SOURCE) $(DEP_CPP_NEWMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\marshal.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\marshal.obj" : $(SOURCE) $(DEP_CPP_MARSH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

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
	

"$(INTDIR)\mystrtoul.obj" : $(SOURCE) $(DEP_CPP_MYSTR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


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
	

"$(INTDIR)\myreadline.obj" : $(SOURCE) $(DEP_CPP_MYREA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\moduleobject.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\moduleobject.obj" : $(SOURCE) $(DEP_CPP_MODUL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\moduleobject.obj" : $(SOURCE) $(DEP_CPP_MODUL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\modsupport.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

DEP_CPP_MODSU=\
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
	
NODEP_CPP_MODSU=\
	".\Python\import.h"\
	

"$(INTDIR)\modsupport.obj" : $(SOURCE) $(DEP_CPP_MODSU) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\modsupport.obj" : $(SOURCE) $(DEP_CPP_MODSU) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\methodobject.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\methodobject.obj" : $(SOURCE) $(DEP_CPP_METHO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\methodobject.obj" : $(SOURCE) $(DEP_CPP_METHO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\md5module.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\md5module.obj" : $(SOURCE) $(DEP_CPP_MD5MO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

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
	

"$(INTDIR)\md5c.obj" : $(SOURCE) $(DEP_CPP_MD5C_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\mathmodule.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\mathmodule.obj" : $(SOURCE) $(DEP_CPP_MATHM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\mathmodule.obj" : $(SOURCE) $(DEP_CPP_MATHM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\mappingobject.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\mappingobject.obj" : $(SOURCE) $(DEP_CPP_MAPPI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

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
################################################################################
# Begin Source File

SOURCE=.\Modules\socketmodule.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	

"$(INTDIR)\socketmodule.obj" : $(SOURCE) $(DEP_CPP_SOCKE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\socketmodule.obj" : $(SOURCE) $(DEP_CPP_SOCKE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\selectmodule.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	".\./Include\thread.h"\
	".\./Include\mytime.h"\
	

"$(INTDIR)\selectmodule.obj" : $(SOURCE) $(DEP_CPP_SELEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	".\./Include\mytime.h"\
	

"$(INTDIR)\selectmodule.obj" : $(SOURCE) $(DEP_CPP_SELEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\sysmodule.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

DEP_CPP_SYSMO=\
	".\./Include\allobjects.h"\
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
	".\./Include\sysmodule.h"\
	".\./Include\intrcheck.h"\
	".\./Include\import.h"\
	".\./Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	
NODEP_CPP_SYSMO=\
	".\Python\sysmodule.h"\
	".\Python\import.h"\
	

"$(INTDIR)\sysmodule.obj" : $(SOURCE) $(DEP_CPP_SYSMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\sysmodule.obj" : $(SOURCE) $(DEP_CPP_SYSMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\import.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

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
	".\Python\importdl.h"\
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
	".\Python\macglue.h"\
	

"$(INTDIR)\import.obj" : $(SOURCE) $(DEP_CPP_IMPORT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

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
	".\Python\importdl.h"\
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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	
NODEP_CPP_IMPORT=\
	".\Python\macglue.h"\
	

"$(INTDIR)\import.obj" : $(SOURCE) $(DEP_CPP_IMPORT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\posixmodule.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

DEP_CPP_POSIX=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	{$(INCLUDE)}"\sys\STAT.H"\
	".\./Include\mytime.h"\
	{$(INCLUDE)}"\unistd.h"\
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
	

"$(INTDIR)\posixmodule.obj" : $(SOURCE) $(DEP_CPP_POSIX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

DEP_CPP_POSIX=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	{$(INCLUDE)}"\sys\STAT.H"\
	".\./Include\mytime.h"\
	{$(INCLUDE)}"\unistd.h"\
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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\posixmodule.obj" : $(SOURCE) $(DEP_CPP_POSIX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\operator.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

DEP_CPP_OPERA=\
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
	

"$(INTDIR)\operator.obj" : $(SOURCE) $(DEP_CPP_OPERA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

DEP_CPP_OPERA=\
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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\operator.obj" : $(SOURCE) $(DEP_CPP_OPERA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\errnomodule.c

!IF  "$(CFG)" == "vc40_dll - Win32 Release"

DEP_CPP_ERRNO=\
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
	

"$(INTDIR)\errnomodule.obj" : $(SOURCE) $(DEP_CPP_ERRNO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_dll - Win32 Debug"

NODEP_CPP_ERRNO=\
	".\Modules\Python.h"\
	

"$(INTDIR)\errnomodule.obj" : $(SOURCE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


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

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\longobject.obj" : $(SOURCE) $(DEP_CPP_LONGO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\longobject.obj" : $(SOURCE) $(DEP_CPP_LONGO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\listobject.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\listobject.obj" : $(SOURCE) $(DEP_CPP_LISTO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\listobject.obj" : $(SOURCE) $(DEP_CPP_LISTO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\intobject.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\intobject.obj" : $(SOURCE) $(DEP_CPP_INTOB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\intobject.obj" : $(SOURCE) $(DEP_CPP_INTOB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\importdl.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\importdl.obj" : $(SOURCE) $(DEP_CPP_IMPOR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	
NODEP_CPP_IMPOR=\
	".\Python\dl.h"\
	".\Python\macdefs.h"\
	".\Python\macglue.h"\
	

"$(INTDIR)\importdl.obj" : $(SOURCE) $(DEP_CPP_IMPOR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\imageop.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\imageop.obj" : $(SOURCE) $(DEP_CPP_IMAGE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

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
	

"$(INTDIR)\grammar1.obj" : $(SOURCE) $(DEP_CPP_GRAMM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


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
	

"$(INTDIR)\graminit.obj" : $(SOURCE) $(DEP_CPP_GRAMI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getversion.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\getversion.obj" : $(SOURCE) $(DEP_CPP_GETVE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\getversion.obj" : $(SOURCE) $(DEP_CPP_GETVE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getplatform.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\getplatform.obj" : $(SOURCE) $(DEP_CPP_GETPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\getplatform.obj" : $(SOURCE) $(DEP_CPP_GETPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\getpath.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\getpath.obj" : $(SOURCE) $(DEP_CPP_GETPA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

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
	

"$(INTDIR)\getmtime.obj" : $(SOURCE) $(DEP_CPP_GETMT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getcopyright.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\getcopyright.obj" : $(SOURCE) $(DEP_CPP_GETCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\getcopyright.obj" : $(SOURCE) $(DEP_CPP_GETCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getcompiler.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\getcompiler.obj" : $(SOURCE) $(DEP_CPP_GETCOM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\getcompiler.obj" : $(SOURCE) $(DEP_CPP_GETCOM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getargs.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\getargs.obj" : $(SOURCE) $(DEP_CPP_GETAR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\getargs.obj" : $(SOURCE) $(DEP_CPP_GETAR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\funcobject.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\funcobject.obj" : $(SOURCE) $(DEP_CPP_FUNCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\funcobject.obj" : $(SOURCE) $(DEP_CPP_FUNCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\frozen.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\frozen.obj" : $(SOURCE) $(DEP_CPP_FROZE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\frozen.obj" : $(SOURCE) $(DEP_CPP_FROZE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\frameobject.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\frameobject.obj" : $(SOURCE) $(DEP_CPP_FRAME) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\frameobject.obj" : $(SOURCE) $(DEP_CPP_FRAME) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\floatobject.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\floatobject.obj" : $(SOURCE) $(DEP_CPP_FLOAT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\floatobject.obj" : $(SOURCE) $(DEP_CPP_FLOAT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\fileobject.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\fileobject.obj" : $(SOURCE) $(DEP_CPP_FILEO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\fileobject.obj" : $(SOURCE) $(DEP_CPP_FILEO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\errors.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\errors.obj" : $(SOURCE) $(DEP_CPP_ERROR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\errors.obj" : $(SOURCE) $(DEP_CPP_ERROR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\environment.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\environment.obj" : $(SOURCE) $(DEP_CPP_ENVIR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\environment.obj" : $(SOURCE) $(DEP_CPP_ENVIR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\config.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\config.obj" : $(SOURCE) $(DEP_CPP_CONFI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\config.obj" : $(SOURCE) $(DEP_CPP_CONFI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\complexobject.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\complexobject.obj" : $(SOURCE) $(DEP_CPP_COMPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\complexobject.obj" : $(SOURCE) $(DEP_CPP_COMPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\compile.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\compile.obj" : $(SOURCE) $(DEP_CPP_COMPI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\compile.obj" : $(SOURCE) $(DEP_CPP_COMPI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\cobject.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\cobject.obj" : $(SOURCE) $(DEP_CPP_COBJE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\cobject.obj" : $(SOURCE) $(DEP_CPP_COBJE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\cmathmodule.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

DEP_CPP_CMATH=\
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
	
NODEP_CPP_CMATH=\
	".\Modules\complexobject.h"\
	

"$(INTDIR)\cmathmodule.obj" : $(SOURCE) $(DEP_CPP_CMATH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\cmathmodule.obj" : $(SOURCE) $(DEP_CPP_CMATH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\classobject.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\classobject.obj" : $(SOURCE) $(DEP_CPP_CLASS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\classobject.obj" : $(SOURCE) $(DEP_CPP_CLASS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\cgensupport.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\cgensupport.obj" : $(SOURCE) $(DEP_CPP_CGENS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\cgensupport.obj" : $(SOURCE) $(DEP_CPP_CGENS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\ceval.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\ceval.obj" : $(SOURCE) $(DEP_CPP_CEVAL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	

"$(INTDIR)\ceval.obj" : $(SOURCE) $(DEP_CPP_CEVAL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\bltinmodule.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\bltinmodule.obj" : $(SOURCE) $(DEP_CPP_BLTIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\bltinmodule.obj" : $(SOURCE) $(DEP_CPP_BLTIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\binascii.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\binascii.obj" : $(SOURCE) $(DEP_CPP_BINAS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\binascii.obj" : $(SOURCE) $(DEP_CPP_BINAS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\audioop.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\audioop.obj" : $(SOURCE) $(DEP_CPP_AUDIO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\audioop.obj" : $(SOURCE) $(DEP_CPP_AUDIO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\arraymodule.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\arraymodule.obj" : $(SOURCE) $(DEP_CPP_ARRAY) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\arraymodule.obj" : $(SOURCE) $(DEP_CPP_ARRAY) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\accessobject.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\accessobject.obj" : $(SOURCE) $(DEP_CPP_ACCES) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\accessobject.obj" : $(SOURCE) $(DEP_CPP_ACCES) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\acceler.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\acceler.obj" : $(SOURCE) $(DEP_CPP_ACCEL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\Include\bitset.h"\
	

"$(INTDIR)\acceler.obj" : $(SOURCE) $(DEP_CPP_ACCEL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\abstract.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\abstract.obj" : $(SOURCE) $(DEP_CPP_ABSTR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\abstract.obj" : $(SOURCE) $(DEP_CPP_ABSTR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\yuvconvert.c
DEP_CPP_YUVCO=\
	".\Modules\yuv.h"\
	

"$(INTDIR)\yuvconvert.obj" : $(SOURCE) $(DEP_CPP_YUVCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\typeobject.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\typeobject.obj" : $(SOURCE) $(DEP_CPP_TYPEO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\typeobject.obj" : $(SOURCE) $(DEP_CPP_TYPEO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\tupleobject.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\tupleobject.obj" : $(SOURCE) $(DEP_CPP_TUPLE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\tupleobject.obj" : $(SOURCE) $(DEP_CPP_TUPLE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\traceback.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\traceback.obj" : $(SOURCE) $(DEP_CPP_TRACE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\traceback.obj" : $(SOURCE) $(DEP_CPP_TRACE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\tokenizer.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\tokenizer.obj" : $(SOURCE) $(DEP_CPP_TOKEN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

DEP_CPP_TOKEN=\
	".\./Include\pgenheaders.h"\
	".\Parser\tokenizer.h"\
	".\./Include\errcode.h"\
	".\./PC\config.h"\
	".\./Include\myproto.h"\
	".\./Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\./Include\rename2.h"\
	".\./Include\token.h"\
	

"$(INTDIR)\tokenizer.obj" : $(SOURCE) $(DEP_CPP_TOKEN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\timemodule.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

DEP_CPP_TIMEM=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	{$(INCLUDE)}"\unistd.h"\
	".\./Include\myselect.h"\
	".\./Include\mytime.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\i86.h"\
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
	

"$(INTDIR)\timemodule.obj" : $(SOURCE) $(DEP_CPP_TIMEM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

DEP_CPP_TIMEM=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	{$(INCLUDE)}"\unistd.h"\
	".\./Include\myselect.h"\
	".\./Include\mytime.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\i86.h"\
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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\timemodule.obj" : $(SOURCE) $(DEP_CPP_TIMEM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\threadmodule.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\threadmodule.obj" : $(SOURCE) $(DEP_CPP_THREA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	

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
	{$(INCLUDE)}"\sys\TYPES.H"\
	{$(INCLUDE)}"\unistd.h"\
	

"$(INTDIR)\thread.obj" : $(SOURCE) $(DEP_CPP_THREAD) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\structmodule.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\structmodule.obj" : $(SOURCE) $(DEP_CPP_STRUC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\structmodule.obj" : $(SOURCE) $(DEP_CPP_STRUC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\structmember.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\structmember.obj" : $(SOURCE) $(DEP_CPP_STRUCT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\structmember.obj" : $(SOURCE) $(DEP_CPP_STRUCT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\stropmodule.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\stropmodule.obj" : $(SOURCE) $(DEP_CPP_STROP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\stropmodule.obj" : $(SOURCE) $(DEP_CPP_STROP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\stringobject.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\stringobject.obj" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\stringobject.obj" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\soundex.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\soundex.obj" : $(SOURCE) $(DEP_CPP_SOUND) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\soundex.obj" : $(SOURCE) $(DEP_CPP_SOUND) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\signalmodule.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\signalmodule.obj" : $(SOURCE) $(DEP_CPP_SIGNA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	

"$(INTDIR)\signalmodule.obj" : $(SOURCE) $(DEP_CPP_SIGNA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\rotormodule.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\rotormodule.obj" : $(SOURCE) $(DEP_CPP_ROTOR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\rotormodule.obj" : $(SOURCE) $(DEP_CPP_ROTOR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\rgbimgmodule.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

DEP_CPP_RGBIM=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	{$(INCLUDE)}"\unistd.h"\
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
	

"$(INTDIR)\rgbimgmodule.obj" : $(SOURCE) $(DEP_CPP_RGBIM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

DEP_CPP_RGBIM=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	{$(INCLUDE)}"\unistd.h"\
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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

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
	

"$(INTDIR)\regexpr.obj" : $(SOURCE) $(DEP_CPP_REGEX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\regexmodule.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

DEP_CPP_REGEXM=\
	".\./Include\Python.h"\
	".\Modules\regexpr.h"\
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
	

"$(INTDIR)\regexmodule.obj" : $(SOURCE) $(DEP_CPP_REGEXM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

DEP_CPP_REGEXM=\
	".\./Include\Python.h"\
	".\Modules\regexpr.h"\
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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\regexmodule.obj" : $(SOURCE) $(DEP_CPP_REGEXM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\rangeobject.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\rangeobject.obj" : $(SOURCE) $(DEP_CPP_RANGE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\rangeobject.obj" : $(SOURCE) $(DEP_CPP_RANGE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\pythonrun.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\pythonrun.obj" : $(SOURCE) $(DEP_CPP_PYTHO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\Include\bitset.h"\
	

"$(INTDIR)\pythonrun.obj" : $(SOURCE) $(DEP_CPP_PYTHO) "$(INTDIR)"
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
	".\./Include\rename2.h"\
	".\./Include\token.h"\
	".\Include\bitset.h"\
	

"$(INTDIR)\parsetok.obj" : $(SOURCE) $(DEP_CPP_PARSE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\parser.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\parser.obj" : $(SOURCE) $(DEP_CPP_PARSER) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\Include\bitset.h"\
	

"$(INTDIR)\parser.obj" : $(SOURCE) $(DEP_CPP_PARSER) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\object.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\object.obj" : $(SOURCE) $(DEP_CPP_OBJEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

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
	

"$(INTDIR)\node.obj" : $(SOURCE) $(DEP_CPP_NODE_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\newmodule.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\newmodule.obj" : $(SOURCE) $(DEP_CPP_NEWMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

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
	

"$(INTDIR)\mystrtoul.obj" : $(SOURCE) $(DEP_CPP_MYSTR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


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
	

"$(INTDIR)\myreadline.obj" : $(SOURCE) $(DEP_CPP_MYREA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\moduleobject.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\moduleobject.obj" : $(SOURCE) $(DEP_CPP_MODUL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\moduleobject.obj" : $(SOURCE) $(DEP_CPP_MODUL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\modsupport.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\modsupport.obj" : $(SOURCE) $(DEP_CPP_MODSU) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\modsupport.obj" : $(SOURCE) $(DEP_CPP_MODSU) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\methodobject.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\methodobject.obj" : $(SOURCE) $(DEP_CPP_METHO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\methodobject.obj" : $(SOURCE) $(DEP_CPP_METHO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\md5module.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\md5module.obj" : $(SOURCE) $(DEP_CPP_MD5MO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

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
	

"$(INTDIR)\md5c.obj" : $(SOURCE) $(DEP_CPP_MD5C_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\mathmodule.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\mathmodule.obj" : $(SOURCE) $(DEP_CPP_MATHM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\mathmodule.obj" : $(SOURCE) $(DEP_CPP_MATHM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\marshal.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\marshal.obj" : $(SOURCE) $(DEP_CPP_MARSH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\marshal.obj" : $(SOURCE) $(DEP_CPP_MARSH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\mappingobject.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\mappingobject.obj" : $(SOURCE) $(DEP_CPP_MAPPI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\mappingobject.obj" : $(SOURCE) $(DEP_CPP_MAPPI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\main.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getopt.c

"$(INTDIR)\getopt.obj" : $(SOURCE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\sysmodule.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\sysmodule.obj" : $(SOURCE) $(DEP_CPP_SYSMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\sysmodule.obj" : $(SOURCE) $(DEP_CPP_SYSMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\socketmodule.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	

"$(INTDIR)\socketmodule.obj" : $(SOURCE) $(DEP_CPP_SOCKE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\socketmodule.obj" : $(SOURCE) $(DEP_CPP_SOCKE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\selectmodule.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	".\./Include\thread.h"\
	".\./Include\mytime.h"\
	

"$(INTDIR)\selectmodule.obj" : $(SOURCE) $(DEP_CPP_SELEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	".\./Include\mytime.h"\
	

"$(INTDIR)\selectmodule.obj" : $(SOURCE) $(DEP_CPP_SELEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\posixmodule.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

DEP_CPP_POSIX=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	{$(INCLUDE)}"\sys\STAT.H"\
	".\./Include\mytime.h"\
	{$(INCLUDE)}"\unistd.h"\
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
	

"$(INTDIR)\posixmodule.obj" : $(SOURCE) $(DEP_CPP_POSIX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

DEP_CPP_POSIX=\
	".\./Include\allobjects.h"\
	".\./Include\modsupport.h"\
	".\./Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	{$(INCLUDE)}"\sys\STAT.H"\
	".\./Include\mytime.h"\
	{$(INCLUDE)}"\unistd.h"\
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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\posixmodule.obj" : $(SOURCE) $(DEP_CPP_POSIX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\import.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

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
	".\Python\importdl.h"\
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
	".\Python\macglue.h"\
	

"$(INTDIR)\import.obj" : $(SOURCE) $(DEP_CPP_IMPORT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

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
	".\Python\importdl.h"\
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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	
NODEP_CPP_IMPORT=\
	".\Python\macglue.h"\
	

"$(INTDIR)\import.obj" : $(SOURCE) $(DEP_CPP_IMPORT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\operator.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

DEP_CPP_OPERA=\
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
	

"$(INTDIR)\operator.obj" : $(SOURCE) $(DEP_CPP_OPERA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

DEP_CPP_OPERA=\
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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\operator.obj" : $(SOURCE) $(DEP_CPP_OPERA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\errnomodule.c

!IF  "$(CFG)" == "vc40_nt - Win32 Release"

DEP_CPP_ERRNO=\
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
	

"$(INTDIR)\errnomodule.obj" : $(SOURCE) $(DEP_CPP_ERRNO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vc40_nt - Win32 Debug"

DEP_CPP_ERRNO=\
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
	".\./Include\rename2.h"\
	".\./Include\thread.h"\
	

"$(INTDIR)\errnomodule.obj" : $(SOURCE) $(DEP_CPP_ERRNO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
