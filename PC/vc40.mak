# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103
# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=python14 - Win32 Release
!MESSAGE No configuration specified.  Defaulting to python14 - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "python14 - Win32 Release" && "$(CFG)" !=\
 "python - Win32 Release" && "$(CFG)" != "_tkinter - Win32 Release" && "$(CFG)"\
 != "NumPy - Win32 Release" && "$(CFG)" != "multiarray - Win32 Release" &&\
 "$(CFG)" != "fast_umath - Win32 Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "vc40.mak" CFG="python14 - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "python14 - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "python - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "_tkinter - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NumPy - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "multiarray - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "fast_umath - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "_tkinter - Win32 Release"

!IF  "$(CFG)" == "python14 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "python14\Release"
# PROP BASE Intermediate_Dir "python14\Release"
# PROP BASE Target_Dir "python14"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vc40"
# PROP Intermediate_Dir "vc40\tmp"
# PROP Target_Dir "python14"
OUTDIR=.\vc40
INTDIR=.\vc40\tmp

ALL : "$(OUTDIR)\python14.dll"

CLEAN : 
	-@erase ".\vc40\python14.dll"
	-@erase ".\vc40\tmp\thread.obj"
	-@erase ".\vc40\tmp\getargs.obj"
	-@erase ".\vc40\tmp\posixmodule.obj"
	-@erase ".\vc40\tmp\getpath_nt.obj"
	-@erase ".\vc40\tmp\mystrtoul.obj"
	-@erase ".\vc40\tmp\selectmodule.obj"
	-@erase ".\vc40\tmp\parsetok.obj"
	-@erase ".\vc40\tmp\cgensupport.obj"
	-@erase ".\vc40\tmp\funcobject.obj"
	-@erase ".\vc40\tmp\md5c.obj"
	-@erase ".\vc40\tmp\accessobject.obj"
	-@erase ".\vc40\tmp\object.obj"
	-@erase ".\vc40\tmp\socketmodule.obj"
	-@erase ".\vc40\tmp\getplatform.obj"
	-@erase ".\vc40\tmp\mappingobject.obj"
	-@erase ".\vc40\tmp\stringobject.obj"
	-@erase ".\vc40\tmp\dl_nt.obj"
	-@erase ".\vc40\tmp\node.obj"
	-@erase ".\vc40\tmp\modsupport.obj"
	-@erase ".\vc40\tmp\listobject.obj"
	-@erase ".\vc40\tmp\importdl.obj"
	-@erase ".\vc40\tmp\frozen.obj"
	-@erase ".\vc40\tmp\getopt.obj"
	-@erase ".\vc40\tmp\binascii.obj"
	-@erase ".\vc40\tmp\import_nt.obj"
	-@erase ".\vc40\tmp\rotormodule.obj"
	-@erase ".\vc40\tmp\acceler.obj"
	-@erase ".\vc40\tmp\mathmodule.obj"
	-@erase ".\vc40\tmp\tupleobject.obj"
	-@erase ".\vc40\tmp\regexpr.obj"
	-@erase ".\vc40\tmp\threadmodule.obj"
	-@erase ".\vc40\tmp\tokenizer.obj"
	-@erase ".\vc40\tmp\classobject.obj"
	-@erase ".\vc40\tmp\structmodule.obj"
	-@erase ".\vc40\tmp\config.obj"
	-@erase ".\vc40\tmp\intobject.obj"
	-@erase ".\vc40\tmp\grammar1.obj"
	-@erase ".\vc40\tmp\abstract.obj"
	-@erase ".\vc40\tmp\fileobject.obj"
	-@erase ".\vc40\tmp\longobject.obj"
	-@erase ".\vc40\tmp\bltinmodule.obj"
	-@erase ".\vc40\tmp\methodobject.obj"
	-@erase ".\vc40\tmp\getcompiler.obj"
	-@erase ".\vc40\tmp\rgbimgmodule.obj"
	-@erase ".\vc40\tmp\traceback.obj"
	-@erase ".\vc40\tmp\sysmodule.obj"
	-@erase ".\vc40\tmp\errnomodule.obj"
	-@erase ".\vc40\tmp\signalmodule.obj"
	-@erase ".\vc40\tmp\moduleobject.obj"
	-@erase ".\vc40\tmp\pythonrun.obj"
	-@erase ".\vc40\tmp\frameobject.obj"
	-@erase ".\vc40\tmp\marshal.obj"
	-@erase ".\vc40\tmp\regexmodule.obj"
	-@erase ".\vc40\tmp\floatobject.obj"
	-@erase ".\vc40\tmp\myreadline.obj"
	-@erase ".\vc40\tmp\complexobject.obj"
	-@erase ".\vc40\tmp\audioop.obj"
	-@erase ".\vc40\tmp\compile.obj"
	-@erase ".\vc40\tmp\cmathmodule.obj"
	-@erase ".\vc40\tmp\timemodule.obj"
	-@erase ".\vc40\tmp\structmember.obj"
	-@erase ".\vc40\tmp\cobject.obj"
	-@erase ".\vc40\tmp\arraymodule.obj"
	-@erase ".\vc40\tmp\graminit.obj"
	-@erase ".\vc40\tmp\import.obj"
	-@erase ".\vc40\tmp\errors.obj"
	-@erase ".\vc40\tmp\sliceobject.obj"
	-@erase ".\vc40\tmp\typeobject.obj"
	-@erase ".\vc40\tmp\getcopyright.obj"
	-@erase ".\vc40\tmp\ceval.obj"
	-@erase ".\vc40\tmp\parser.obj"
	-@erase ".\vc40\tmp\stropmodule.obj"
	-@erase ".\vc40\tmp\soundex.obj"
	-@erase ".\vc40\tmp\yuvconvert.obj"
	-@erase ".\vc40\tmp\getversion.obj"
	-@erase ".\vc40\tmp\main.obj"
	-@erase ".\vc40\tmp\newmodule.obj"
	-@erase ".\vc40\tmp\imageop.obj"
	-@erase ".\vc40\tmp\md5module.obj"
	-@erase ".\vc40\tmp\rangeobject.obj"
	-@erase ".\vc40\tmp\getmtime.obj"
	-@erase ".\vc40\tmp\operator.obj"
	-@erase ".\vc40\tmp\python_nt.res"
	-@erase ".\vc40\python14.lib"
	-@erase ".\vc40\python14.exp"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /O2 /I "PC" /I "Include" /I "Python" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /D "USE_DL_EXPORT" /YX /c
CPP_PROJ=/nologo /MD /W3 /O2 /I "PC" /I "Include" /I "Python" /D "WIN32" /D\
 "_WINDOWS" /D "HAVE_CONFIG_H" /D "USE_DL_EXPORT" /Fp"$(INTDIR)/python14.pch"\
 /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\vc40\tmp/
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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/python_nt.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/python14.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /base:0x1e100000 /subsystem:windows /dll /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib wsock32.lib /nologo /base:0x1e100000 /subsystem:windows /dll\
 /incremental:no /pdb:"$(OUTDIR)/python14.pdb" /machine:I386\
 /def:".\PC\python_nt.def" /out:"$(OUTDIR)/python14.dll"\
 /implib:"$(OUTDIR)/python14.lib" 
DEF_FILE= \
	".\PC\python_nt.def"
LINK32_OBJS= \
	"$(INTDIR)/thread.obj" \
	"$(INTDIR)/getargs.obj" \
	"$(INTDIR)/posixmodule.obj" \
	"$(INTDIR)/getpath_nt.obj" \
	"$(INTDIR)/mystrtoul.obj" \
	"$(INTDIR)/selectmodule.obj" \
	"$(INTDIR)/parsetok.obj" \
	"$(INTDIR)/cgensupport.obj" \
	"$(INTDIR)/funcobject.obj" \
	"$(INTDIR)/md5c.obj" \
	"$(INTDIR)/accessobject.obj" \
	"$(INTDIR)/object.obj" \
	"$(INTDIR)/socketmodule.obj" \
	"$(INTDIR)/getplatform.obj" \
	"$(INTDIR)/mappingobject.obj" \
	"$(INTDIR)/stringobject.obj" \
	"$(INTDIR)/dl_nt.obj" \
	"$(INTDIR)/node.obj" \
	"$(INTDIR)/modsupport.obj" \
	"$(INTDIR)/listobject.obj" \
	"$(INTDIR)/importdl.obj" \
	"$(INTDIR)/frozen.obj" \
	"$(INTDIR)/getopt.obj" \
	"$(INTDIR)/binascii.obj" \
	"$(INTDIR)/import_nt.obj" \
	"$(INTDIR)/rotormodule.obj" \
	"$(INTDIR)/acceler.obj" \
	"$(INTDIR)/mathmodule.obj" \
	"$(INTDIR)/tupleobject.obj" \
	"$(INTDIR)/regexpr.obj" \
	"$(INTDIR)/threadmodule.obj" \
	"$(INTDIR)/tokenizer.obj" \
	"$(INTDIR)/classobject.obj" \
	"$(INTDIR)/structmodule.obj" \
	"$(INTDIR)/config.obj" \
	"$(INTDIR)/intobject.obj" \
	"$(INTDIR)/grammar1.obj" \
	"$(INTDIR)/abstract.obj" \
	"$(INTDIR)/fileobject.obj" \
	"$(INTDIR)/longobject.obj" \
	"$(INTDIR)/bltinmodule.obj" \
	"$(INTDIR)/methodobject.obj" \
	"$(INTDIR)/getcompiler.obj" \
	"$(INTDIR)/rgbimgmodule.obj" \
	"$(INTDIR)/traceback.obj" \
	"$(INTDIR)/sysmodule.obj" \
	"$(INTDIR)/errnomodule.obj" \
	"$(INTDIR)/signalmodule.obj" \
	"$(INTDIR)/moduleobject.obj" \
	"$(INTDIR)/pythonrun.obj" \
	"$(INTDIR)/frameobject.obj" \
	"$(INTDIR)/marshal.obj" \
	"$(INTDIR)/regexmodule.obj" \
	"$(INTDIR)/floatobject.obj" \
	"$(INTDIR)/myreadline.obj" \
	"$(INTDIR)/complexobject.obj" \
	"$(INTDIR)/audioop.obj" \
	"$(INTDIR)/compile.obj" \
	"$(INTDIR)/cmathmodule.obj" \
	"$(INTDIR)/timemodule.obj" \
	"$(INTDIR)/structmember.obj" \
	"$(INTDIR)/cobject.obj" \
	"$(INTDIR)/arraymodule.obj" \
	"$(INTDIR)/graminit.obj" \
	"$(INTDIR)/import.obj" \
	"$(INTDIR)/errors.obj" \
	"$(INTDIR)/sliceobject.obj" \
	"$(INTDIR)/typeobject.obj" \
	"$(INTDIR)/getcopyright.obj" \
	"$(INTDIR)/ceval.obj" \
	"$(INTDIR)/parser.obj" \
	"$(INTDIR)/stropmodule.obj" \
	"$(INTDIR)/soundex.obj" \
	"$(INTDIR)/yuvconvert.obj" \
	"$(INTDIR)/getversion.obj" \
	"$(INTDIR)/main.obj" \
	"$(INTDIR)/newmodule.obj" \
	"$(INTDIR)/imageop.obj" \
	"$(INTDIR)/md5module.obj" \
	"$(INTDIR)/rangeobject.obj" \
	"$(INTDIR)/getmtime.obj" \
	"$(INTDIR)/operator.obj" \
	"$(INTDIR)/python_nt.res"

"$(OUTDIR)\python14.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "python - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "python\Release"
# PROP BASE Intermediate_Dir "python\Release"
# PROP BASE Target_Dir "python"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vc40"
# PROP Intermediate_Dir "vc40\tmp"
# PROP Target_Dir "python"
OUTDIR=.\vc40
INTDIR=.\vc40\tmp

ALL : "$(OUTDIR)\python.exe"

CLEAN : 
	-@erase ".\vc40\python.exe"
	-@erase ".\vc40\tmp\main_nt.obj"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MD /W3 /O2 /I "PC" /I "Include" /I "Python" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /YX /c
CPP_PROJ=/nologo /MD /W3 /O2 /I "PC" /I "Include" /I "Python" /D "WIN32" /D\
 "_WINDOWS" /D "HAVE_CONFIG_H" /Fp"$(INTDIR)/python.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\vc40\tmp/
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
BSC32_FLAGS=/nologo /o"$(OUTDIR)/python.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/python.pdb" /machine:I386 /out:"$(OUTDIR)/python.exe" 
LINK32_OBJS= \
	"$(INTDIR)/main_nt.obj" \
	".\vc40\python14.lib"

"$(OUTDIR)\python.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "_tkinter - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "_tkinter\Release"
# PROP BASE Intermediate_Dir "_tkinter\Release"
# PROP BASE Target_Dir "_tkinter"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vc40"
# PROP Intermediate_Dir "vc40\tmp"
# PROP Target_Dir "_tkinter"
OUTDIR=.\vc40
INTDIR=.\vc40\tmp

ALL : "$(OUTDIR)\_tkinter.dll"

CLEAN : 
	-@erase ".\vc40\_tkinter.dll"
	-@erase ".\vc40\tmp\_tkinter.obj"
	-@erase ".\vc40\_tkinter.lib"
	-@erase ".\vc40\_tkinter.exp"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "PC" /I "Include" /I "C:\tcl\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "HAVE_CONFIG_H" /YX /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "PC" /I "Include" /I "C:\tcl\include" /D\
 "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "HAVE_CONFIG_H"\
 /Fp"$(INTDIR)/_tkinter.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\vc40\tmp/
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
BSC32_FLAGS=/nologo /o"$(OUTDIR)/_tkinter.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:0x1e190000 /subsystem:windows /dll /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /base:0x1e190000 /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/_tkinter.pdb" /machine:I386 /def:".\PC\_tkinter.def"\
 /out:"$(OUTDIR)/_tkinter.dll" /implib:"$(OUTDIR)/_tkinter.lib" 
DEF_FILE= \
	".\PC\_tkinter.def"
LINK32_OBJS= \
	"$(INTDIR)/_tkinter.obj" \
	".\PC\tcl75.lib" \
	".\vc40\python14.lib" \
	".\PC\tk41.lib"

"$(OUTDIR)\_tkinter.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "NumPy - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "NumPy\NumPy___"
# PROP BASE Intermediate_Dir "NumPy\NumPy___"
# PROP BASE Target_Dir "NumPy"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vc40"
# PROP Intermediate_Dir "vc40\tmp"
# PROP Target_Dir "NumPy"
OUTDIR=.\vc40
INTDIR=.\vc40\tmp

ALL : "$(OUTDIR)\NumPy.dll"

CLEAN : 
	-@erase ".\vc40\NumPy.dll"
	-@erase ".\vc40\tmp\ofuncobject.obj"
	-@erase ".\vc40\tmp\arrayobject.obj"
	-@erase ".\vc40\NumPy.lib"
	-@erase ".\vc40\NumPy.exp"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "PC" /I "Include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "PC" /I "Include" /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)/NumPy.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\vc40\tmp/
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
BSC32_FLAGS=/nologo /o"$(OUTDIR)/NumPy.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/NumPy.pdb" /machine:I386 /def:".\NumPy\numpy_nt.def"\
 /out:"$(OUTDIR)/NumPy.dll" /implib:"$(OUTDIR)/NumPy.lib" 
DEF_FILE= \
	".\NumPy\numpy_nt.def"
LINK32_OBJS= \
	"$(INTDIR)/ofuncobject.obj" \
	"$(INTDIR)/arrayobject.obj" \
	".\vc40\python14.lib"

"$(OUTDIR)\NumPy.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "multiarray - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "multiarray\Release"
# PROP BASE Intermediate_Dir "multiarray\Release"
# PROP BASE Target_Dir "multiarray"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vc40"
# PROP Intermediate_Dir "vc40\tmp"
# PROP Target_Dir "multiarray"
OUTDIR=.\vc40
INTDIR=.\vc40\tmp

ALL : "$(OUTDIR)\multiarray.dll"

CLEAN : 
	-@erase ".\vc40\multiarray.dll"
	-@erase ".\vc40\tmp\multiarraymodule.obj"
	-@erase ".\vc40\multiarray.lib"
	-@erase ".\vc40\multiarray.exp"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "PC" /I "Include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "PC" /I "Include" /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)/multiarray.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\vc40\tmp/
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
BSC32_FLAGS=/nologo /o"$(OUTDIR)/multiarray.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/multiarray.pdb" /machine:I386 /def:".\NumPy\multiarray.def"\
 /out:"$(OUTDIR)/multiarray.dll" /implib:"$(OUTDIR)/multiarray.lib" 
DEF_FILE= \
	".\NumPy\multiarray.def"
LINK32_OBJS= \
	"$(INTDIR)/multiarraymodule.obj" \
	".\vc40\python14.lib" \
	".\vc40\NumPy.lib"

"$(OUTDIR)\multiarray.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "fast_umath - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "fast_umath\Release"
# PROP BASE Intermediate_Dir "fast_umath\Release"
# PROP BASE Target_Dir "fast_umath"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vc40"
# PROP Intermediate_Dir "vc40\tmp"
# PROP Target_Dir "fast_umath"
OUTDIR=.\vc40
INTDIR=.\vc40\tmp

ALL : "$(OUTDIR)\fast_umath.dll"

CLEAN : 
	-@erase ".\vc40\fast_umath.dll"
	-@erase ".\vc40\tmp\fast_umathmodule.obj"
	-@erase ".\vc40\fast_umath.lib"
	-@erase ".\vc40\fast_umath.exp"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "PC" /I "Include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "PC" /I "Include" /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)/fast_umath.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\vc40\tmp/
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
BSC32_FLAGS=/nologo /o"$(OUTDIR)/fast_umath.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/fast_umath.pdb" /machine:I386 /def:".\NumPy\fast_umath.def"\
 /out:"$(OUTDIR)/fast_umath.dll" /implib:"$(OUTDIR)/fast_umath.lib" 
DEF_FILE= \
	".\NumPy\fast_umath.def"
LINK32_OBJS= \
	"$(INTDIR)/fast_umathmodule.obj" \
	".\vc40\python14.lib" \
	".\vc40\NumPy.lib"

"$(OUTDIR)\fast_umath.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

################################################################################
# Begin Target

# Name "python14 - Win32 Release"
################################################################################
# Begin Source File

SOURCE=.\Objects\longobject.c
DEP_CPP_LONGO=\
	".\Include\allobjects.h"\
	".\Include\longintrepr.h"\
	".\Include\mymath.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\longobject.obj" : $(SOURCE) $(DEP_CPP_LONGO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\listobject.c
DEP_CPP_LISTO=\
	".\Include\allobjects.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\listobject.obj" : $(SOURCE) $(DEP_CPP_LISTO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\intobject.c
DEP_CPP_INTOB=\
	".\Include\allobjects.h"\
	".\Include\modsupport.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\intobject.obj" : $(SOURCE) $(DEP_CPP_INTOB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\importdl.c
DEP_CPP_IMPOR=\
	".\Include\allobjects.h"\
	".\Include\osdefs.h"\
	".\Python\importdl.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	{$(INCLUDE)}"\sys\STAT.H"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	
NODEP_CPP_IMPOR=\
	".\Python\dl.h"\
	".\Python\macdefs.h"\
	".\Python\macglue.h"\
	

"$(INTDIR)\importdl.obj" : $(SOURCE) $(DEP_CPP_IMPOR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\imageop.c
DEP_CPP_IMAGE=\
	".\Include\allobjects.h"\
	".\Include\modsupport.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\imageop.obj" : $(SOURCE) $(DEP_CPP_IMAGE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\grammar1.c
DEP_CPP_GRAMM=\
	".\Include\pgenheaders.h"\
	".\Include\grammar.h"\
	".\Include\token.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\Include\rename2.h"\
	".\Include\bitset.h"\
	

"$(INTDIR)\grammar1.obj" : $(SOURCE) $(DEP_CPP_GRAMM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\graminit.c
DEP_CPP_GRAMI=\
	".\Include\pgenheaders.h"\
	".\Include\grammar.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\Include\rename2.h"\
	".\Include\bitset.h"\
	

"$(INTDIR)\graminit.obj" : $(SOURCE) $(DEP_CPP_GRAMI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getversion.c
DEP_CPP_GETVE=\
	".\Include\Python.h"\
	".\Include\patchlevel.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\getversion.obj" : $(SOURCE) $(DEP_CPP_GETVE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getplatform.c
DEP_CPP_GETPL=\
	".\Include\Python.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\getplatform.obj" : $(SOURCE) $(DEP_CPP_GETPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getmtime.c
DEP_CPP_GETMT=\
	".\PC\config.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	{$(INCLUDE)}"\sys\STAT.H"\
	".\Include\rename2.h"\
	

"$(INTDIR)\getmtime.obj" : $(SOURCE) $(DEP_CPP_GETMT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getcopyright.c
DEP_CPP_GETCO=\
	".\Include\Python.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\getcopyright.obj" : $(SOURCE) $(DEP_CPP_GETCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getcompiler.c
DEP_CPP_GETCOM=\
	".\Include\Python.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\getcompiler.obj" : $(SOURCE) $(DEP_CPP_GETCOM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getargs.c
DEP_CPP_GETAR=\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\getargs.obj" : $(SOURCE) $(DEP_CPP_GETAR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\funcobject.c
DEP_CPP_FUNCO=\
	".\Include\allobjects.h"\
	".\Include\compile.h"\
	".\Include\structmember.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\funcobject.obj" : $(SOURCE) $(DEP_CPP_FUNCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\frozen.c
DEP_CPP_FROZE=\
	".\Include\Python.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\frozen.obj" : $(SOURCE) $(DEP_CPP_FROZE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\frameobject.c
DEP_CPP_FRAME=\
	".\Include\allobjects.h"\
	".\Include\compile.h"\
	".\Include\frameobject.h"\
	".\Include\opcode.h"\
	".\Include\structmember.h"\
	".\Include\bltinmodule.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\frameobject.obj" : $(SOURCE) $(DEP_CPP_FRAME) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\floatobject.c
DEP_CPP_FLOAT=\
	".\Include\allobjects.h"\
	".\Include\modsupport.h"\
	".\Include\mymath.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\floatobject.obj" : $(SOURCE) $(DEP_CPP_FLOAT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\fileobject.c
DEP_CPP_FILEO=\
	".\Include\allobjects.h"\
	".\Include\modsupport.h"\
	".\Include\structmember.h"\
	".\Include\ceval.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\fileobject.obj" : $(SOURCE) $(DEP_CPP_FILEO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\errors.c
DEP_CPP_ERROR=\
	".\Include\allobjects.h"\
	".\Include\traceback.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\errors.obj" : $(SOURCE) $(DEP_CPP_ERROR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\config.c
DEP_CPP_CONFI=\
	".\Include\Python.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\config.obj" : $(SOURCE) $(DEP_CPP_CONFI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\complexobject.c
DEP_CPP_COMPL=\
	".\Include\allobjects.h"\
	".\Include\modsupport.h"\
	".\Include\mymath.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\complexobject.obj" : $(SOURCE) $(DEP_CPP_COMPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\compile.c
DEP_CPP_COMPI=\
	".\Include\allobjects.h"\
	".\Include\node.h"\
	".\Include\token.h"\
	".\Include\graminit.h"\
	".\Include\compile.h"\
	".\Include\opcode.h"\
	".\Include\structmember.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\compile.obj" : $(SOURCE) $(DEP_CPP_COMPI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\cobject.c
DEP_CPP_COBJE=\
	".\Include\Python.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\cobject.obj" : $(SOURCE) $(DEP_CPP_COBJE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\cmathmodule.c
DEP_CPP_CMATH=\
	".\Include\allobjects.h"\
	".\Include\complexobject.h"\
	".\Include\mymath.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
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
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\cmathmodule.obj" : $(SOURCE) $(DEP_CPP_CMATH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\classobject.c
DEP_CPP_CLASS=\
	".\Include\allobjects.h"\
	".\Include\structmember.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\classobject.obj" : $(SOURCE) $(DEP_CPP_CLASS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\cgensupport.c
DEP_CPP_CGENS=\
	".\Include\allobjects.h"\
	".\Include\cgensupport.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\cgensupport.obj" : $(SOURCE) $(DEP_CPP_CGENS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\ceval.c
DEP_CPP_CEVAL=\
	".\Include\allobjects.h"\
	".\Include\compile.h"\
	".\Include\frameobject.h"\
	".\Include\eval.h"\
	".\Include\opcode.h"\
	".\Include\graminit.h"\
	".\Include\thread.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	

"$(INTDIR)\ceval.obj" : $(SOURCE) $(DEP_CPP_CEVAL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\bltinmodule.c
DEP_CPP_BLTIN=\
	".\Include\allobjects.h"\
	".\Include\node.h"\
	".\Include\graminit.h"\
	".\Include\bltinmodule.h"\
	".\Include\import.h"\
	".\Include\compile.h"\
	".\Include\eval.h"\
	".\Include\mymath.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\bltinmodule.obj" : $(SOURCE) $(DEP_CPP_BLTIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\binascii.c
DEP_CPP_BINAS=\
	".\Include\Python.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\binascii.obj" : $(SOURCE) $(DEP_CPP_BINAS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\audioop.c
DEP_CPP_AUDIO=\
	".\Include\allobjects.h"\
	".\Include\modsupport.h"\
	".\Include\mymath.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\audioop.obj" : $(SOURCE) $(DEP_CPP_AUDIO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\arraymodule.c
DEP_CPP_ARRAY=\
	".\Include\allobjects.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\arraymodule.obj" : $(SOURCE) $(DEP_CPP_ARRAY) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\accessobject.c
DEP_CPP_ACCES=\
	".\Include\allobjects.h"\
	".\Include\ceval.h"\
	".\Include\structmember.h"\
	".\Include\modsupport.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\accessobject.obj" : $(SOURCE) $(DEP_CPP_ACCES) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\acceler.c
DEP_CPP_ACCEL=\
	".\Include\pgenheaders.h"\
	".\Include\grammar.h"\
	".\Include\node.h"\
	".\Include\token.h"\
	".\Parser\parser.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\Include\rename2.h"\
	".\Include\bitset.h"\
	

"$(INTDIR)\acceler.obj" : $(SOURCE) $(DEP_CPP_ACCEL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\abstract.c
DEP_CPP_ABSTR=\
	".\Include\Python.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\abstract.obj" : $(SOURCE) $(DEP_CPP_ABSTR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


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
DEP_CPP_TYPEO=\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\typeobject.obj" : $(SOURCE) $(DEP_CPP_TYPEO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\tupleobject.c
DEP_CPP_TUPLE=\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\tupleobject.obj" : $(SOURCE) $(DEP_CPP_TUPLE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\traceback.c
DEP_CPP_TRACE=\
	".\Include\allobjects.h"\
	".\Include\sysmodule.h"\
	".\Include\compile.h"\
	".\Include\frameobject.h"\
	".\Include\traceback.h"\
	".\Include\structmember.h"\
	".\Include\osdefs.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\traceback.obj" : $(SOURCE) $(DEP_CPP_TRACE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\tokenizer.c
DEP_CPP_TOKEN=\
	".\Include\pgenheaders.h"\
	".\Parser\tokenizer.h"\
	".\Include\errcode.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\Include\rename2.h"\
	".\Include\token.h"\
	

"$(INTDIR)\tokenizer.obj" : $(SOURCE) $(DEP_CPP_TOKEN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\timemodule.c
DEP_CPP_TIMEM=\
	".\Include\allobjects.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\mymath.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	".\Include\myselect.h"\
	".\Include\mytime.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\timemodule.obj" : $(SOURCE) $(DEP_CPP_TIMEM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\thread.c
DEP_CPP_THREA=\
	".\PC\config.h"\
	".\Include\thread.h"\
	".\Python\thread_sgi.h"\
	".\Python\thread_solaris.h"\
	".\Python\thread_lwp.h"\
	".\Python\thread_pthread.h"\
	".\Python\thread_cthread.h"\
	".\Python\thread_nt.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\thread.obj" : $(SOURCE) $(DEP_CPP_THREA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\structmodule.c
DEP_CPP_STRUC=\
	".\Include\allobjects.h"\
	".\Include\modsupport.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\structmodule.obj" : $(SOURCE) $(DEP_CPP_STRUC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\structmember.c
DEP_CPP_STRUCT=\
	".\Include\allobjects.h"\
	".\Include\structmember.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\structmember.obj" : $(SOURCE) $(DEP_CPP_STRUCT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\stropmodule.c
DEP_CPP_STROP=\
	".\Include\allobjects.h"\
	".\Include\modsupport.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\stropmodule.obj" : $(SOURCE) $(DEP_CPP_STROP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\stringobject.c
DEP_CPP_STRIN=\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\stringobject.obj" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\soundex.c
DEP_CPP_SOUND=\
	".\Include\Python.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\soundex.obj" : $(SOURCE) $(DEP_CPP_SOUND) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\signalmodule.c
DEP_CPP_SIGNA=\
	".\Include\Python.h"\
	".\Include\intrcheck.h"\
	".\Include\thread.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	

"$(INTDIR)\signalmodule.obj" : $(SOURCE) $(DEP_CPP_SIGNA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\rotormodule.c
DEP_CPP_ROTOR=\
	".\Include\Python.h"\
	".\Include\mymath.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\rotormodule.obj" : $(SOURCE) $(DEP_CPP_ROTOR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\rgbimgmodule.c
DEP_CPP_RGBIM=\
	".\Include\allobjects.h"\
	".\Include\modsupport.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\rgbimgmodule.obj" : $(SOURCE) $(DEP_CPP_RGBIM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\regexpr.c
DEP_CPP_REGEX=\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Modules\regexpr.h"\
	".\Include\rename2.h"\
	
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
DEP_CPP_REGEXM=\
	".\Include\Python.h"\
	".\Modules\regexpr.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\regexmodule.obj" : $(SOURCE) $(DEP_CPP_REGEXM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\rangeobject.c
DEP_CPP_RANGE=\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\rangeobject.obj" : $(SOURCE) $(DEP_CPP_RANGE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\pythonrun.c
DEP_CPP_PYTHO=\
	".\Include\allobjects.h"\
	".\Include\grammar.h"\
	".\Include\node.h"\
	".\Include\parsetok.h"\
	".\Include\graminit.h"\
	".\Include\errcode.h"\
	".\Include\sysmodule.h"\
	".\Include\bltinmodule.h"\
	".\Include\compile.h"\
	".\Include\eval.h"\
	".\Include\ceval.h"\
	".\Include\import.h"\
	".\Include\marshal.h"\
	".\Include\thread.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\pythonrun.h"\
	".\Include\intrcheck.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\bitset.h"\
	

"$(INTDIR)\pythonrun.obj" : $(SOURCE) $(DEP_CPP_PYTHO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\parsetok.c
DEP_CPP_PARSE=\
	".\Include\pgenheaders.h"\
	".\Parser\tokenizer.h"\
	".\Include\node.h"\
	".\Include\grammar.h"\
	".\Parser\parser.h"\
	".\Include\parsetok.h"\
	".\Include\errcode.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\Include\rename2.h"\
	".\Include\token.h"\
	".\Include\bitset.h"\
	

"$(INTDIR)\parsetok.obj" : $(SOURCE) $(DEP_CPP_PARSE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\parser.c
DEP_CPP_PARSER=\
	".\Include\pgenheaders.h"\
	".\Include\token.h"\
	".\Include\grammar.h"\
	".\Include\node.h"\
	".\Parser\parser.h"\
	".\Include\errcode.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\Include\rename2.h"\
	".\Include\bitset.h"\
	

"$(INTDIR)\parser.obj" : $(SOURCE) $(DEP_CPP_PARSER) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\object.c
DEP_CPP_OBJEC=\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\object.obj" : $(SOURCE) $(DEP_CPP_OBJEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\node.c
DEP_CPP_NODE_=\
	".\Include\pgenheaders.h"\
	".\Include\node.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\mymalloc.h"\
	".\Include\pydebug.h"\
	".\Include\rename2.h"\
	

"$(INTDIR)\node.obj" : $(SOURCE) $(DEP_CPP_NODE_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\newmodule.c
DEP_CPP_NEWMO=\
	".\Include\allobjects.h"\
	".\Include\compile.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\newmodule.obj" : $(SOURCE) $(DEP_CPP_NEWMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\marshal.c
DEP_CPP_MARSH=\
	".\Include\allobjects.h"\
	".\Include\modsupport.h"\
	".\Include\longintrepr.h"\
	".\Include\compile.h"\
	".\Include\marshal.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\marshal.obj" : $(SOURCE) $(DEP_CPP_MARSH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\mystrtoul.c
DEP_CPP_MYSTR=\
	".\PC\config.h"\
	".\Include\rename2.h"\
	

"$(INTDIR)\mystrtoul.obj" : $(SOURCE) $(DEP_CPP_MYSTR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\myreadline.c
DEP_CPP_MYREA=\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\mymalloc.h"\
	".\Include\intrcheck.h"\
	".\Include\rename2.h"\
	

"$(INTDIR)\myreadline.obj" : $(SOURCE) $(DEP_CPP_MYREA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\moduleobject.c
DEP_CPP_MODUL=\
	".\Include\allobjects.h"\
	".\Include\ceval.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\moduleobject.obj" : $(SOURCE) $(DEP_CPP_MODUL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\modsupport.c
DEP_CPP_MODSU=\
	".\Include\allobjects.h"\
	".\Include\import.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\modsupport.obj" : $(SOURCE) $(DEP_CPP_MODSU) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\methodobject.c
DEP_CPP_METHO=\
	".\Include\allobjects.h"\
	".\Include\token.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\methodobject.obj" : $(SOURCE) $(DEP_CPP_METHO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\md5module.c
DEP_CPP_MD5MO=\
	".\Include\allobjects.h"\
	".\Include\modsupport.h"\
	".\Modules\md5.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\md5module.obj" : $(SOURCE) $(DEP_CPP_MD5MO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\md5c.c
DEP_CPP_MD5C_=\
	".\PC\config.h"\
	".\Modules\md5.h"\
	

"$(INTDIR)\md5c.obj" : $(SOURCE) $(DEP_CPP_MD5C_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\mathmodule.c
DEP_CPP_MATHM=\
	".\Include\allobjects.h"\
	".\Include\mymath.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\mathmodule.obj" : $(SOURCE) $(DEP_CPP_MATHM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\mappingobject.c
DEP_CPP_MAPPI=\
	".\Include\allobjects.h"\
	".\Include\modsupport.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\mappingobject.obj" : $(SOURCE) $(DEP_CPP_MAPPI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\socketmodule.c
DEP_CPP_SOCKE=\
	".\Include\Python.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	".\Include\mytime.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\socketmodule.obj" : $(SOURCE) $(DEP_CPP_SOCKE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\selectmodule.c
DEP_CPP_SELEC=\
	".\Include\allobjects.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	".\Include\myselect.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	".\Include\mytime.h"\
	

"$(INTDIR)\selectmodule.obj" : $(SOURCE) $(DEP_CPP_SELEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\sysmodule.c
DEP_CPP_SYSMO=\
	".\Include\allobjects.h"\
	".\Include\sysmodule.h"\
	".\Include\import.h"\
	".\Include\modsupport.h"\
	".\Include\osdefs.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\intrcheck.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\sysmodule.obj" : $(SOURCE) $(DEP_CPP_SYSMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\import.c
DEP_CPP_IMPORT=\
	".\Include\allobjects.h"\
	".\Include\node.h"\
	".\Include\token.h"\
	".\Include\graminit.h"\
	".\Include\import.h"\
	".\Include\errcode.h"\
	".\Include\sysmodule.h"\
	".\Include\bltinmodule.h"\
	".\Include\pythonrun.h"\
	".\Include\marshal.h"\
	".\Include\compile.h"\
	".\Include\eval.h"\
	".\Include\osdefs.h"\
	".\Python\importdl.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\intrcheck.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	
NODEP_CPP_IMPORT=\
	".\Python\macglue.h"\
	

"$(INTDIR)\import.obj" : $(SOURCE) $(DEP_CPP_IMPORT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\posixmodule.c
DEP_CPP_POSIX=\
	".\Include\allobjects.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	{$(INCLUDE)}"\sys\STAT.H"\
	".\Include\mytime.h"\
	{$(INCLUDE)}"\sys\UTIME.H"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\posixmodule.obj" : $(SOURCE) $(DEP_CPP_POSIX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\operator.c
DEP_CPP_OPERA=\
	".\Include\Python.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\operator.obj" : $(SOURCE) $(DEP_CPP_OPERA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\errnomodule.c
DEP_CPP_ERRNO=\
	".\Include\Python.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\errnomodule.obj" : $(SOURCE) $(DEP_CPP_ERRNO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\sliceobject.c
DEP_CPP_SLICE=\
	".\Include\Python.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\sliceobject.obj" : $(SOURCE) $(DEP_CPP_SLICE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\main.c
DEP_CPP_MAIN_=\
	".\Include\Python.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getopt.c

"$(INTDIR)\getopt.obj" : $(SOURCE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\import_nt.c
DEP_CPP_IMPORT_=\
	".\Include\allobjects.h"\
	".\Include\osdefs.h"\
	".\Python\importdl.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\import_nt.obj" : $(SOURCE) $(DEP_CPP_IMPORT_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\getpath_nt.c
DEP_CPP_GETPA=\
	".\Include\Python.h"\
	".\Include\osdefs.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\getpath_nt.obj" : $(SOURCE) $(DEP_CPP_GETPA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\dl_nt.c
DEP_CPP_DL_NT=\
	".\PC\config.h"\
	".\Include\allobjects.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\dl_nt.obj" : $(SOURCE) $(DEP_CPP_DL_NT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\python_nt.def
# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\threadmodule.c
DEP_CPP_THREAD=\
	".\Include\allobjects.h"\
	".\Include\thread.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	

"$(INTDIR)\threadmodule.obj" : $(SOURCE) $(DEP_CPP_THREAD) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\python_nt.rc

!IF  "$(CFG)" == "python14 - Win32 Release"

# ADD BASE RSC /l 0x409 /i "PC"
# ADD RSC /l 0x409 /i "PC" /i "Include"

"$(INTDIR)\python_nt.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/python_nt.res" /i "PC" /i "Include" /d\
 "NDEBUG" $(SOURCE)


!ENDIF 

# End Source File
# End Target
################################################################################
# Begin Target

# Name "python - Win32 Release"
################################################################################
# Begin Source File

SOURCE=.\PC\main_nt.c

"$(INTDIR)\main_nt.obj" : $(SOURCE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\vc40\python14.lib
# End Source File
# End Target
################################################################################
# Begin Target

# Name "_tkinter - Win32 Release"
################################################################################
# Begin Source File

SOURCE=.\Modules\_tkinter.c
DEP_CPP__TKIN=\
	".\Include\Python.h"\
	"\tcl\include\tcl.h"\
	"\tcl\include\tk.h"\
	".\Include\allobjects.h"\
	"\tcl\include\X11/Xlib.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	"\tcl\include\X11/X.h"\
	"\tcl\include\X11/Xfuncproto.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\_tkinter.obj" : $(SOURCE) $(DEP_CPP__TKIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\tk41.lib
# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\tcl75.lib
# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\_tkinter.def
# End Source File
################################################################################
# Begin Source File

SOURCE=.\vc40\python14.lib
# End Source File
# End Target
################################################################################
# Begin Target

# Name "NumPy - Win32 Release"
################################################################################
# Begin Source File

SOURCE=.\NumPy\arrayobject.c
DEP_CPP_ARRAYO=\
	".\Include\Python.h"\
	".\NumPy\arrayobject.h"\
	".\NumPy\ofuncobject.h"\
	".\NumPy\arraytypes.c"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\arrayobject.obj" : $(SOURCE) $(DEP_CPP_ARRAYO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\NumPy\ofuncobject.c
DEP_CPP_OFUNC=\
	".\Include\Python.h"\
	".\NumPy\arrayobject.h"\
	".\NumPy\ofuncobject.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\ofuncobject.obj" : $(SOURCE) $(DEP_CPP_OFUNC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\NumPy\numpy_nt.def
# End Source File
################################################################################
# Begin Source File

SOURCE=.\vc40\python14.lib
# End Source File
# End Target
################################################################################
# Begin Target

# Name "multiarray - Win32 Release"
################################################################################
# Begin Source File

SOURCE=.\NumPy\multiarraymodule.c
DEP_CPP_MULTI=\
	".\Include\Python.h"\
	".\NumPy\arrayobject.h"\
	".\NumPy\ofuncobject.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\abstract.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\multiarraymodule.obj" : $(SOURCE) $(DEP_CPP_MULTI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\vc40\python14.lib
# End Source File
################################################################################
# Begin Source File

SOURCE=.\vc40\NumPy.lib
# End Source File
################################################################################
# Begin Source File

SOURCE=.\NumPy\multiarray.def
# End Source File
# End Target
################################################################################
# Begin Target

# Name "fast_umath - Win32 Release"
################################################################################
# Begin Source File

SOURCE=.\NumPy\fast_umathmodule.c
DEP_CPP_FAST_=\
	".\Include\Python.h"\
	".\NumPy\arrayobject.h"\
	".\NumPy\ofuncobject.h"\
	".\Include\abstract.h"\
	".\Include\allobjects.h"\
	".\PC\config.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\accessobject.h"\
	".\Include\intobject.h"\
	".\Include\longobject.h"\
	".\Include\floatobject.h"\
	".\Include\complexobject.h"\
	".\Include\rangeobject.h"\
	".\Include\stringobject.h"\
	".\Include\tupleobject.h"\
	".\Include\listobject.h"\
	".\Include\mappingobject.h"\
	".\Include\methodobject.h"\
	".\Include\moduleobject.h"\
	".\Include\funcobject.h"\
	".\Include\classobject.h"\
	".\Include\fileobject.h"\
	".\Include\cobject.h"\
	".\Include\traceback.h"\
	".\Include\sliceobject.h"\
	".\Include\pyerrors.h"\
	".\Include\mymalloc.h"\
	".\Include\modsupport.h"\
	".\Include\ceval.h"\
	".\Include\pythonrun.h"\
	".\Include\sysmodule.h"\
	".\Include\intrcheck.h"\
	".\Include\import.h"\
	".\Include\bltinmodule.h"\
	".\Include\rename2.h"\
	".\Include\thread.h"\
	

"$(INTDIR)\fast_umathmodule.obj" : $(SOURCE) $(DEP_CPP_FAST_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\vc40\python14.lib
# End Source File
################################################################################
# Begin Source File

SOURCE=.\vc40\NumPy.lib
# End Source File
################################################################################
# Begin Source File

SOURCE=.\NumPy\fast_umath.def
# End Source File
# End Target
# End Project
################################################################################
