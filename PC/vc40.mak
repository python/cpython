# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103
# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=python15 - Win32 Release
!MESSAGE No configuration specified.  Defaulting to python15 - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "python15 - Win32 Release" && "$(CFG)" !=\
 "python - Win32 Release" && "$(CFG)" != "_tkinter - Win32 Release" && "$(CFG)"\
 != "python15 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "vc40.mak" CFG="python15 - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "python15 - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "python - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "_tkinter - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "python15 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "python15 - Win32 Debug"

!IF  "$(CFG)" == "python15 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "python15\Release"
# PROP BASE Intermediate_Dir "python15\Release"
# PROP BASE Target_Dir "python15"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vc40"
# PROP Intermediate_Dir "vc40\tmp"
# PROP Target_Dir "python15"
OUTDIR=.\vc40
INTDIR=.\vc40\tmp

ALL : "$(OUTDIR)\python15.dll"

CLEAN : 
	-@erase "$(INTDIR)\abstract.obj"
	-@erase "$(INTDIR)\acceler.obj"
	-@erase "$(INTDIR)\arraymodule.obj"
	-@erase "$(INTDIR)\audioop.obj"
	-@erase "$(INTDIR)\binascii.obj"
	-@erase "$(INTDIR)\bltinmodule.obj"
	-@erase "$(INTDIR)\ceval.obj"
	-@erase "$(INTDIR)\classobject.obj"
	-@erase "$(INTDIR)\cmathmodule.obj"
	-@erase "$(INTDIR)\cobject.obj"
	-@erase "$(INTDIR)\compile.obj"
	-@erase "$(INTDIR)\complexobject.obj"
	-@erase "$(INTDIR)\config.obj"
	-@erase "$(INTDIR)\cPickle.obj"
	-@erase "$(INTDIR)\cStringIO.obj"
	-@erase "$(INTDIR)\dictobject.obj"
	-@erase "$(INTDIR)\dl_nt.obj"
	-@erase "$(INTDIR)\errnomodule.obj"
	-@erase "$(INTDIR)\errors.obj"
	-@erase "$(INTDIR)\fileobject.obj"
	-@erase "$(INTDIR)\floatobject.obj"
	-@erase "$(INTDIR)\frameobject.obj"
	-@erase "$(INTDIR)\frozen.obj"
	-@erase "$(INTDIR)\funcobject.obj"
	-@erase "$(INTDIR)\getargs.obj"
	-@erase "$(INTDIR)\getbuildinfo.obj"
	-@erase "$(INTDIR)\getcompiler.obj"
	-@erase "$(INTDIR)\getcopyright.obj"
	-@erase "$(INTDIR)\getmtime.obj"
	-@erase "$(INTDIR)\getopt.obj"
	-@erase "$(INTDIR)\getpath_nt.obj"
	-@erase "$(INTDIR)\getplatform.obj"
	-@erase "$(INTDIR)\getversion.obj"
	-@erase "$(INTDIR)\graminit.obj"
	-@erase "$(INTDIR)\grammar1.obj"
	-@erase "$(INTDIR)\imageop.obj"
	-@erase "$(INTDIR)\import.obj"
	-@erase "$(INTDIR)\import_nt.obj"
	-@erase "$(INTDIR)\importdl.obj"
	-@erase "$(INTDIR)\intobject.obj"
	-@erase "$(INTDIR)\listobject.obj"
	-@erase "$(INTDIR)\longobject.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\marshal.obj"
	-@erase "$(INTDIR)\mathmodule.obj"
	-@erase "$(INTDIR)\md5c.obj"
	-@erase "$(INTDIR)\md5module.obj"
	-@erase "$(INTDIR)\methodobject.obj"
	-@erase "$(INTDIR)\modsupport.obj"
	-@erase "$(INTDIR)\moduleobject.obj"
	-@erase "$(INTDIR)\msvcrtmodule.obj"
	-@erase "$(INTDIR)\myreadline.obj"
	-@erase "$(INTDIR)\mystrtoul.obj"
	-@erase "$(INTDIR)\newmodule.obj"
	-@erase "$(INTDIR)\node.obj"
	-@erase "$(INTDIR)\object.obj"
	-@erase "$(INTDIR)\operator.obj"
	-@erase "$(INTDIR)\parser.obj"
	-@erase "$(INTDIR)\parsetok.obj"
	-@erase "$(INTDIR)\posixmodule.obj"
	-@erase "$(INTDIR)\pystate.obj"
	-@erase "$(INTDIR)\python_nt.res"
	-@erase "$(INTDIR)\pythonrun.obj"
	-@erase "$(INTDIR)\rangeobject.obj"
	-@erase "$(INTDIR)\regexmodule.obj"
	-@erase "$(INTDIR)\regexpr.obj"
	-@erase "$(INTDIR)\rgbimgmodule.obj"
	-@erase "$(INTDIR)\rotormodule.obj"
	-@erase "$(INTDIR)\selectmodule.obj"
	-@erase "$(INTDIR)\signalmodule.obj"
	-@erase "$(INTDIR)\sliceobject.obj"
	-@erase "$(INTDIR)\socketmodule.obj"
	-@erase "$(INTDIR)\soundex.obj"
	-@erase "$(INTDIR)\stringobject.obj"
	-@erase "$(INTDIR)\stropmodule.obj"
	-@erase "$(INTDIR)\structmember.obj"
	-@erase "$(INTDIR)\structmodule.obj"
	-@erase "$(INTDIR)\sysmodule.obj"
	-@erase "$(INTDIR)\thread.obj"
	-@erase "$(INTDIR)\threadmodule.obj"
	-@erase "$(INTDIR)\timemodule.obj"
	-@erase "$(INTDIR)\tokenizer.obj"
	-@erase "$(INTDIR)\traceback.obj"
	-@erase "$(INTDIR)\tupleobject.obj"
	-@erase "$(INTDIR)\typeobject.obj"
	-@erase "$(INTDIR)\yuvconvert.obj"
	-@erase "$(OUTDIR)\python15.dll"
	-@erase "$(OUTDIR)\python15.exp"
	-@erase "$(OUTDIR)\python15.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /O2 /I "PC" /I "Include" /I "Python" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /D "USE_DL_EXPORT" /YX /c
CPP_PROJ=/nologo /MD /W3 /O2 /I "PC" /I "Include" /I "Python" /D "WIN32" /D\
 "_WINDOWS" /D "HAVE_CONFIG_H" /D "USE_DL_EXPORT" /Fp"$(INTDIR)/python15.pch"\
 /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\vc40\tmp/
CPP_SBRS=.\.

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)/python15.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib largeint.lib /nologo /base:0x1e100000 /subsystem:windows /dll /machine:I386 /nodefaultlib:"libc"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib wsock32.lib largeint.lib /nologo /base:0x1e100000\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)/python15.pdb"\
 /machine:I386 /nodefaultlib:"libc" /def:".\PC\python_nt.def"\
 /out:"$(OUTDIR)/python15.dll" /implib:"$(OUTDIR)/python15.lib" 
DEF_FILE= \
	".\PC\python_nt.def"
LINK32_OBJS= \
	"$(INTDIR)\abstract.obj" \
	"$(INTDIR)\acceler.obj" \
	"$(INTDIR)\arraymodule.obj" \
	"$(INTDIR)\audioop.obj" \
	"$(INTDIR)\binascii.obj" \
	"$(INTDIR)\bltinmodule.obj" \
	"$(INTDIR)\ceval.obj" \
	"$(INTDIR)\classobject.obj" \
	"$(INTDIR)\cmathmodule.obj" \
	"$(INTDIR)\cobject.obj" \
	"$(INTDIR)\compile.obj" \
	"$(INTDIR)\complexobject.obj" \
	"$(INTDIR)\config.obj" \
	"$(INTDIR)\cPickle.obj" \
	"$(INTDIR)\cStringIO.obj" \
	"$(INTDIR)\dictobject.obj" \
	"$(INTDIR)\dl_nt.obj" \
	"$(INTDIR)\errnomodule.obj" \
	"$(INTDIR)\errors.obj" \
	"$(INTDIR)\fileobject.obj" \
	"$(INTDIR)\floatobject.obj" \
	"$(INTDIR)\frameobject.obj" \
	"$(INTDIR)\frozen.obj" \
	"$(INTDIR)\funcobject.obj" \
	"$(INTDIR)\getargs.obj" \
	"$(INTDIR)\getbuildinfo.obj" \
	"$(INTDIR)\getcompiler.obj" \
	"$(INTDIR)\getcopyright.obj" \
	"$(INTDIR)\getmtime.obj" \
	"$(INTDIR)\getopt.obj" \
	"$(INTDIR)\getpath_nt.obj" \
	"$(INTDIR)\getplatform.obj" \
	"$(INTDIR)\getversion.obj" \
	"$(INTDIR)\graminit.obj" \
	"$(INTDIR)\grammar1.obj" \
	"$(INTDIR)\imageop.obj" \
	"$(INTDIR)\import.obj" \
	"$(INTDIR)\import_nt.obj" \
	"$(INTDIR)\importdl.obj" \
	"$(INTDIR)\intobject.obj" \
	"$(INTDIR)\listobject.obj" \
	"$(INTDIR)\longobject.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\marshal.obj" \
	"$(INTDIR)\mathmodule.obj" \
	"$(INTDIR)\md5c.obj" \
	"$(INTDIR)\md5module.obj" \
	"$(INTDIR)\methodobject.obj" \
	"$(INTDIR)\modsupport.obj" \
	"$(INTDIR)\moduleobject.obj" \
	"$(INTDIR)\msvcrtmodule.obj" \
	"$(INTDIR)\myreadline.obj" \
	"$(INTDIR)\mystrtoul.obj" \
	"$(INTDIR)\newmodule.obj" \
	"$(INTDIR)\node.obj" \
	"$(INTDIR)\object.obj" \
	"$(INTDIR)\operator.obj" \
	"$(INTDIR)\parser.obj" \
	"$(INTDIR)\parsetok.obj" \
	"$(INTDIR)\posixmodule.obj" \
	"$(INTDIR)\pystate.obj" \
	"$(INTDIR)\python_nt.res" \
	"$(INTDIR)\pythonrun.obj" \
	"$(INTDIR)\rangeobject.obj" \
	"$(INTDIR)\regexmodule.obj" \
	"$(INTDIR)\regexpr.obj" \
	"$(INTDIR)\rgbimgmodule.obj" \
	"$(INTDIR)\rotormodule.obj" \
	"$(INTDIR)\selectmodule.obj" \
	"$(INTDIR)\signalmodule.obj" \
	"$(INTDIR)\sliceobject.obj" \
	"$(INTDIR)\socketmodule.obj" \
	"$(INTDIR)\soundex.obj" \
	"$(INTDIR)\stringobject.obj" \
	"$(INTDIR)\stropmodule.obj" \
	"$(INTDIR)\structmember.obj" \
	"$(INTDIR)\structmodule.obj" \
	"$(INTDIR)\sysmodule.obj" \
	"$(INTDIR)\thread.obj" \
	"$(INTDIR)\threadmodule.obj" \
	"$(INTDIR)\timemodule.obj" \
	"$(INTDIR)\tokenizer.obj" \
	"$(INTDIR)\traceback.obj" \
	"$(INTDIR)\tupleobject.obj" \
	"$(INTDIR)\typeobject.obj" \
	"$(INTDIR)\yuvconvert.obj"

"$(OUTDIR)\python15.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
	-@erase "$(INTDIR)\python.obj"
	-@erase "$(OUTDIR)\python.exe"

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
CPP_SBRS=.\.

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
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/python.pdb" /machine:I386 /out:"$(OUTDIR)/python.exe" 
LINK32_OBJS= \
	"$(INTDIR)\python.obj" \
	"$(OUTDIR)\python15.lib"

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
	-@erase "$(INTDIR)\_tkinter.obj"
	-@erase "$(OUTDIR)\_tkinter.dll"
	-@erase "$(OUTDIR)\_tkinter.exp"
	-@erase "$(OUTDIR)\_tkinter.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "PC" /I "Include" /I "C:\TCL\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "HAVE_CONFIG_H" /YX /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "PC" /I "Include" /I "C:\TCL\include" /D\
 "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "HAVE_CONFIG_H"\
 /Fp"$(INTDIR)/_tkinter.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\vc40\tmp/
CPP_SBRS=.\.

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
BSC32_SBRS= \
	
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
	"$(INTDIR)\_tkinter.obj" \
	"$(OUTDIR)\python15.lib" \
	".\PC\tcl80.lib" \
	".\PC\tk80.lib"

"$(OUTDIR)\_tkinter.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "python15 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "python15\Release"
# PROP BASE Intermediate_Dir "python15\Release"
# PROP BASE Target_Dir "python15"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "python15\Debug"
# PROP Intermediate_Dir "python15\Debug"
# PROP Target_Dir "python15"
OUTDIR=.\python15\Debug
INTDIR=.\python15\Debug

ALL : ".\vc40\python15.dll"

CLEAN : 
	-@erase "$(INTDIR)\abstract.obj"
	-@erase "$(INTDIR)\acceler.obj"
	-@erase "$(INTDIR)\arraymodule.obj"
	-@erase "$(INTDIR)\audioop.obj"
	-@erase "$(INTDIR)\binascii.obj"
	-@erase "$(INTDIR)\bltinmodule.obj"
	-@erase "$(INTDIR)\ceval.obj"
	-@erase "$(INTDIR)\classobject.obj"
	-@erase "$(INTDIR)\cmathmodule.obj"
	-@erase "$(INTDIR)\cobject.obj"
	-@erase "$(INTDIR)\compile.obj"
	-@erase "$(INTDIR)\complexobject.obj"
	-@erase "$(INTDIR)\config.obj"
	-@erase "$(INTDIR)\cPickle.obj"
	-@erase "$(INTDIR)\cStringIO.obj"
	-@erase "$(INTDIR)\dictobject.obj"
	-@erase "$(INTDIR)\dl_nt.obj"
	-@erase "$(INTDIR)\errnomodule.obj"
	-@erase "$(INTDIR)\errors.obj"
	-@erase "$(INTDIR)\fileobject.obj"
	-@erase "$(INTDIR)\floatobject.obj"
	-@erase "$(INTDIR)\frameobject.obj"
	-@erase "$(INTDIR)\frozen.obj"
	-@erase "$(INTDIR)\funcobject.obj"
	-@erase "$(INTDIR)\getargs.obj"
	-@erase "$(INTDIR)\getbuildinfo.obj"
	-@erase "$(INTDIR)\getcompiler.obj"
	-@erase "$(INTDIR)\getcopyright.obj"
	-@erase "$(INTDIR)\getmtime.obj"
	-@erase "$(INTDIR)\getopt.obj"
	-@erase "$(INTDIR)\getpath_nt.obj"
	-@erase "$(INTDIR)\getplatform.obj"
	-@erase "$(INTDIR)\getversion.obj"
	-@erase "$(INTDIR)\graminit.obj"
	-@erase "$(INTDIR)\grammar1.obj"
	-@erase "$(INTDIR)\imageop.obj"
	-@erase "$(INTDIR)\import.obj"
	-@erase "$(INTDIR)\import_nt.obj"
	-@erase "$(INTDIR)\importdl.obj"
	-@erase "$(INTDIR)\intobject.obj"
	-@erase "$(INTDIR)\listobject.obj"
	-@erase "$(INTDIR)\longobject.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\marshal.obj"
	-@erase "$(INTDIR)\mathmodule.obj"
	-@erase "$(INTDIR)\md5c.obj"
	-@erase "$(INTDIR)\md5module.obj"
	-@erase "$(INTDIR)\methodobject.obj"
	-@erase "$(INTDIR)\modsupport.obj"
	-@erase "$(INTDIR)\moduleobject.obj"
	-@erase "$(INTDIR)\msvcrtmodule.obj"
	-@erase "$(INTDIR)\myreadline.obj"
	-@erase "$(INTDIR)\mystrtoul.obj"
	-@erase "$(INTDIR)\newmodule.obj"
	-@erase "$(INTDIR)\node.obj"
	-@erase "$(INTDIR)\object.obj"
	-@erase "$(INTDIR)\operator.obj"
	-@erase "$(INTDIR)\parser.obj"
	-@erase "$(INTDIR)\parsetok.obj"
	-@erase "$(INTDIR)\posixmodule.obj"
	-@erase "$(INTDIR)\pystate.obj"
	-@erase "$(INTDIR)\python_nt.res"
	-@erase "$(INTDIR)\pythonrun.obj"
	-@erase "$(INTDIR)\rangeobject.obj"
	-@erase "$(INTDIR)\regexmodule.obj"
	-@erase "$(INTDIR)\regexpr.obj"
	-@erase "$(INTDIR)\rgbimgmodule.obj"
	-@erase "$(INTDIR)\rotormodule.obj"
	-@erase "$(INTDIR)\selectmodule.obj"
	-@erase "$(INTDIR)\signalmodule.obj"
	-@erase "$(INTDIR)\sliceobject.obj"
	-@erase "$(INTDIR)\socketmodule.obj"
	-@erase "$(INTDIR)\soundex.obj"
	-@erase "$(INTDIR)\stringobject.obj"
	-@erase "$(INTDIR)\stropmodule.obj"
	-@erase "$(INTDIR)\structmember.obj"
	-@erase "$(INTDIR)\structmodule.obj"
	-@erase "$(INTDIR)\sysmodule.obj"
	-@erase "$(INTDIR)\thread.obj"
	-@erase "$(INTDIR)\threadmodule.obj"
	-@erase "$(INTDIR)\timemodule.obj"
	-@erase "$(INTDIR)\tokenizer.obj"
	-@erase "$(INTDIR)\traceback.obj"
	-@erase "$(INTDIR)\tupleobject.obj"
	-@erase "$(INTDIR)\typeobject.obj"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\yuvconvert.obj"
	-@erase "$(OUTDIR)\python15.exp"
	-@erase "$(OUTDIR)\python15.lib"
	-@erase "$(OUTDIR)\python15.pdb"
	-@erase ".\vc40\python15.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /O2 /I "PC" /I "Include" /I "Python" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /D "USE_DL_EXPORT" /YX /c
# ADD CPP /nologo /MDd /W3 /Zi /Od /I "PC" /I "Include" /I "Python" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /D "USE_DL_EXPORT" /YX /c
CPP_PROJ=/nologo /MDd /W3 /Zi /Od /I "PC" /I "Include" /I "Python" /D "WIN32"\
 /D "_WINDOWS" /D "HAVE_CONFIG_H" /D "USE_DL_EXPORT" /Fp"$(INTDIR)/python15.pch"\
 /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\python15\Debug/
CPP_SBRS=.\.

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
# ADD RSC /l 0x409
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/python_nt.res" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/python15.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib largeint.lib /nologo /base:0x1e100000 /subsystem:windows /dll /machine:I386 /nodefaultlib:"libc" /out:"vc40/python15.dll"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib largeint.lib /nologo /base:0x1e100000 /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"libc" /out:"vc40/python15.dll"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib wsock32.lib largeint.lib /nologo /base:0x1e100000\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)/python15.pdb" /debug\
 /machine:I386 /nodefaultlib:"libc" /def:".\PC\python_nt.def"\
 /out:"vc40/python15.dll" /implib:"$(OUTDIR)/python15.lib" 
DEF_FILE= \
	".\PC\python_nt.def"
LINK32_OBJS= \
	"$(INTDIR)\abstract.obj" \
	"$(INTDIR)\acceler.obj" \
	"$(INTDIR)\arraymodule.obj" \
	"$(INTDIR)\audioop.obj" \
	"$(INTDIR)\binascii.obj" \
	"$(INTDIR)\bltinmodule.obj" \
	"$(INTDIR)\ceval.obj" \
	"$(INTDIR)\classobject.obj" \
	"$(INTDIR)\cmathmodule.obj" \
	"$(INTDIR)\cobject.obj" \
	"$(INTDIR)\compile.obj" \
	"$(INTDIR)\complexobject.obj" \
	"$(INTDIR)\config.obj" \
	"$(INTDIR)\cPickle.obj" \
	"$(INTDIR)\cStringIO.obj" \
	"$(INTDIR)\dictobject.obj" \
	"$(INTDIR)\dl_nt.obj" \
	"$(INTDIR)\errnomodule.obj" \
	"$(INTDIR)\errors.obj" \
	"$(INTDIR)\fileobject.obj" \
	"$(INTDIR)\floatobject.obj" \
	"$(INTDIR)\frameobject.obj" \
	"$(INTDIR)\frozen.obj" \
	"$(INTDIR)\funcobject.obj" \
	"$(INTDIR)\getargs.obj" \
	"$(INTDIR)\getbuildinfo.obj" \
	"$(INTDIR)\getcompiler.obj" \
	"$(INTDIR)\getcopyright.obj" \
	"$(INTDIR)\getmtime.obj" \
	"$(INTDIR)\getopt.obj" \
	"$(INTDIR)\getpath_nt.obj" \
	"$(INTDIR)\getplatform.obj" \
	"$(INTDIR)\getversion.obj" \
	"$(INTDIR)\graminit.obj" \
	"$(INTDIR)\grammar1.obj" \
	"$(INTDIR)\imageop.obj" \
	"$(INTDIR)\import.obj" \
	"$(INTDIR)\import_nt.obj" \
	"$(INTDIR)\importdl.obj" \
	"$(INTDIR)\intobject.obj" \
	"$(INTDIR)\listobject.obj" \
	"$(INTDIR)\longobject.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\marshal.obj" \
	"$(INTDIR)\mathmodule.obj" \
	"$(INTDIR)\md5c.obj" \
	"$(INTDIR)\md5module.obj" \
	"$(INTDIR)\methodobject.obj" \
	"$(INTDIR)\modsupport.obj" \
	"$(INTDIR)\moduleobject.obj" \
	"$(INTDIR)\msvcrtmodule.obj" \
	"$(INTDIR)\myreadline.obj" \
	"$(INTDIR)\mystrtoul.obj" \
	"$(INTDIR)\newmodule.obj" \
	"$(INTDIR)\node.obj" \
	"$(INTDIR)\object.obj" \
	"$(INTDIR)\operator.obj" \
	"$(INTDIR)\parser.obj" \
	"$(INTDIR)\parsetok.obj" \
	"$(INTDIR)\posixmodule.obj" \
	"$(INTDIR)\pystate.obj" \
	"$(INTDIR)\python_nt.res" \
	"$(INTDIR)\pythonrun.obj" \
	"$(INTDIR)\rangeobject.obj" \
	"$(INTDIR)\regexmodule.obj" \
	"$(INTDIR)\regexpr.obj" \
	"$(INTDIR)\rgbimgmodule.obj" \
	"$(INTDIR)\rotormodule.obj" \
	"$(INTDIR)\selectmodule.obj" \
	"$(INTDIR)\signalmodule.obj" \
	"$(INTDIR)\sliceobject.obj" \
	"$(INTDIR)\socketmodule.obj" \
	"$(INTDIR)\soundex.obj" \
	"$(INTDIR)\stringobject.obj" \
	"$(INTDIR)\stropmodule.obj" \
	"$(INTDIR)\structmember.obj" \
	"$(INTDIR)\structmodule.obj" \
	"$(INTDIR)\sysmodule.obj" \
	"$(INTDIR)\thread.obj" \
	"$(INTDIR)\threadmodule.obj" \
	"$(INTDIR)\timemodule.obj" \
	"$(INTDIR)\tokenizer.obj" \
	"$(INTDIR)\traceback.obj" \
	"$(INTDIR)\tupleobject.obj" \
	"$(INTDIR)\typeobject.obj" \
	"$(INTDIR)\yuvconvert.obj"

".\vc40\python15.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

################################################################################
# Begin Target

# Name "python15 - Win32 Release"
# Name "python15 - Win32 Debug"

!IF  "$(CFG)" == "python15 - Win32 Release"

!ELSEIF  "$(CFG)" == "python15 - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\Objects\longobject.c
DEP_CPP_LONGO=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longintrepr.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\mymath.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\longobject.obj" : $(SOURCE) $(DEP_CPP_LONGO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\listobject.c
DEP_CPP_LISTO=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\listobject.obj" : $(SOURCE) $(DEP_CPP_LISTO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\intobject.c
DEP_CPP_INTOB=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\intobject.obj" : $(SOURCE) $(DEP_CPP_INTOB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\importdl.c
DEP_CPP_IMPOR=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\osdefs.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	".\Python\importdl.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
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
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\imageop.obj" : $(SOURCE) $(DEP_CPP_IMAGE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\grammar1.c
DEP_CPP_GRAMM=\
	".\Include\bitset.h"\
	".\Include\grammar.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\pgenheaders.h"\
	".\Include\pydebug.h"\
	".\Include\token.h"\
	".\PC\config.h"\
	

"$(INTDIR)\grammar1.obj" : $(SOURCE) $(DEP_CPP_GRAMM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\graminit.c
DEP_CPP_GRAMI=\
	".\Include\bitset.h"\
	".\Include\grammar.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\pgenheaders.h"\
	".\Include\pydebug.h"\
	".\PC\config.h"\
	

"$(INTDIR)\graminit.obj" : $(SOURCE) $(DEP_CPP_GRAMI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getversion.c
DEP_CPP_GETVE=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\patchlevel.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\getversion.obj" : $(SOURCE) $(DEP_CPP_GETVE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getplatform.c
DEP_CPP_GETPL=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\getplatform.obj" : $(SOURCE) $(DEP_CPP_GETPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getmtime.c
DEP_CPP_GETMT=\
	".\PC\config.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\getmtime.obj" : $(SOURCE) $(DEP_CPP_GETMT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getcopyright.c
DEP_CPP_GETCO=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\getcopyright.obj" : $(SOURCE) $(DEP_CPP_GETCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getcompiler.c
DEP_CPP_GETCOM=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\getcompiler.obj" : $(SOURCE) $(DEP_CPP_GETCOM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\getargs.c
DEP_CPP_GETAR=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\getargs.obj" : $(SOURCE) $(DEP_CPP_GETAR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\funcobject.c

!IF  "$(CFG)" == "python15 - Win32 Release"

DEP_CPP_FUNCO=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\compile.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\structmember.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\funcobject.obj" : $(SOURCE) $(DEP_CPP_FUNCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "python15 - Win32 Debug"

DEP_CPP_FUNCO=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\compile.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\structmember.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\funcobject.obj" : $(SOURCE) $(DEP_CPP_FUNCO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\frozen.c
DEP_CPP_FROZE=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\frozen.obj" : $(SOURCE) $(DEP_CPP_FROZE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\frameobject.c
DEP_CPP_FRAME=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\compile.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\frameobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\opcode.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\structmember.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\frameobject.obj" : $(SOURCE) $(DEP_CPP_FRAME) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\floatobject.c
DEP_CPP_FLOAT=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\mymath.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\floatobject.obj" : $(SOURCE) $(DEP_CPP_FLOAT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\fileobject.c

!IF  "$(CFG)" == "python15 - Win32 Release"

DEP_CPP_FILEO=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\structmember.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\fileobject.obj" : $(SOURCE) $(DEP_CPP_FILEO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "python15 - Win32 Debug"

DEP_CPP_FILEO=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\structmember.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\fileobject.obj" : $(SOURCE) $(DEP_CPP_FILEO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\errors.c
DEP_CPP_ERROR=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\errors.obj" : $(SOURCE) $(DEP_CPP_ERROR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\config.c
DEP_CPP_CONFI=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\config.obj" : $(SOURCE) $(DEP_CPP_CONFI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\complexobject.c
DEP_CPP_COMPL=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\mymath.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\complexobject.obj" : $(SOURCE) $(DEP_CPP_COMPL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\compile.c
DEP_CPP_COMPI=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\compile.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\graminit.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\node.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\opcode.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\structmember.h"\
	".\Include\sysmodule.h"\
	".\Include\token.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\compile.obj" : $(SOURCE) $(DEP_CPP_COMPI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\cobject.c
DEP_CPP_COBJE=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\cobject.obj" : $(SOURCE) $(DEP_CPP_COBJE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\cmathmodule.c
DEP_CPP_CMATH=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\mymath.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\cmathmodule.obj" : $(SOURCE) $(DEP_CPP_CMATH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\classobject.c

!IF  "$(CFG)" == "python15 - Win32 Release"

DEP_CPP_CLASS=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\structmember.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\classobject.obj" : $(SOURCE) $(DEP_CPP_CLASS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "python15 - Win32 Debug"

DEP_CPP_CLASS=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\structmember.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\classobject.obj" : $(SOURCE) $(DEP_CPP_CLASS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\ceval.c
DEP_CPP_CEVAL=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\compile.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\eval.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\frameobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\opcode.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\thread.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\ceval.obj" : $(SOURCE) $(DEP_CPP_CEVAL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\bltinmodule.c
DEP_CPP_BLTIN=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\compile.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\eval.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\mymath.h"\
	".\Include\myproto.h"\
	".\Include\node.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\bltinmodule.obj" : $(SOURCE) $(DEP_CPP_BLTIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\binascii.c
DEP_CPP_BINAS=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\binascii.obj" : $(SOURCE) $(DEP_CPP_BINAS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\audioop.c
DEP_CPP_AUDIO=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\mymath.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\audioop.obj" : $(SOURCE) $(DEP_CPP_AUDIO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\arraymodule.c
DEP_CPP_ARRAY=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\arraymodule.obj" : $(SOURCE) $(DEP_CPP_ARRAY) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\acceler.c
DEP_CPP_ACCEL=\
	".\Include\bitset.h"\
	".\Include\grammar.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\node.h"\
	".\Include\pgenheaders.h"\
	".\Include\pydebug.h"\
	".\Include\token.h"\
	".\Parser\parser.h"\
	".\PC\config.h"\
	

"$(INTDIR)\acceler.obj" : $(SOURCE) $(DEP_CPP_ACCEL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\abstract.c
DEP_CPP_ABSTR=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

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
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\typeobject.obj" : $(SOURCE) $(DEP_CPP_TYPEO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\tupleobject.c
DEP_CPP_TUPLE=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\tupleobject.obj" : $(SOURCE) $(DEP_CPP_TUPLE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\traceback.c
DEP_CPP_TRACE=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\compile.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\frameobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\osdefs.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\structmember.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\traceback.obj" : $(SOURCE) $(DEP_CPP_TRACE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\tokenizer.c
DEP_CPP_TOKEN=\
	".\Include\errcode.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\pgenheaders.h"\
	".\Include\pydebug.h"\
	".\Include\token.h"\
	".\Parser\tokenizer.h"\
	".\PC\config.h"\
	

"$(INTDIR)\tokenizer.obj" : $(SOURCE) $(DEP_CPP_TOKEN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\timemodule.c
DEP_CPP_TIMEM=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\mymath.h"\
	".\Include\myproto.h"\
	".\Include\myselect.h"\
	".\Include\mytime.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	{$(INCLUDE)}"\sys\TIMEB.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\timemodule.obj" : $(SOURCE) $(DEP_CPP_TIMEM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\thread.c
DEP_CPP_THREA=\
	".\Include\thread.h"\
	".\PC\config.h"\
	".\Python\thread_cthread.h"\
	".\Python\thread_foobar.h"\
	".\Python\thread_lwp.h"\
	".\Python\thread_nt.h"\
	".\Python\thread_pthread.h"\
	".\Python\thread_sgi.h"\
	".\Python\thread_solaris.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_THREA=\
	"\usr\include\thread.h"\
	

"$(INTDIR)\thread.obj" : $(SOURCE) $(DEP_CPP_THREA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\structmodule.c
DEP_CPP_STRUC=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\mymath.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\structmodule.obj" : $(SOURCE) $(DEP_CPP_STRUC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\structmember.c
DEP_CPP_STRUCT=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\structmember.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\structmember.obj" : $(SOURCE) $(DEP_CPP_STRUCT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\stropmodule.c
DEP_CPP_STROP=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\stropmodule.obj" : $(SOURCE) $(DEP_CPP_STROP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\stringobject.c
DEP_CPP_STRIN=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\mymath.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\stringobject.obj" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\soundex.c
DEP_CPP_SOUND=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\soundex.obj" : $(SOURCE) $(DEP_CPP_SOUND) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\signalmodule.c
DEP_CPP_SIGNA=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\thread.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\signalmodule.obj" : $(SOURCE) $(DEP_CPP_SIGNA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\rotormodule.c
DEP_CPP_ROTOR=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\mymath.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\rotormodule.obj" : $(SOURCE) $(DEP_CPP_ROTOR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\rgbimgmodule.c
DEP_CPP_RGBIM=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\rgbimgmodule.obj" : $(SOURCE) $(DEP_CPP_RGBIM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\regexpr.c
DEP_CPP_REGEX=\
	".\Include\myproto.h"\
	".\Modules\regexpr.h"\
	".\PC\config.h"\
	

"$(INTDIR)\regexpr.obj" : $(SOURCE) $(DEP_CPP_REGEX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\regexmodule.c
DEP_CPP_REGEXM=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\Modules\regexpr.h"\
	".\PC\config.h"\
	

"$(INTDIR)\regexmodule.obj" : $(SOURCE) $(DEP_CPP_REGEXM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\rangeobject.c
DEP_CPP_RANGE=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\rangeobject.obj" : $(SOURCE) $(DEP_CPP_RANGE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\pythonrun.c
DEP_CPP_PYTHO=\
	".\Include\abstract.h"\
	".\Include\bitset.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\compile.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\errcode.h"\
	".\Include\eval.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\grammar.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\marshal.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\node.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\parsetok.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\thread.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\pythonrun.obj" : $(SOURCE) $(DEP_CPP_PYTHO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\parsetok.c
DEP_CPP_PARSE=\
	".\Include\bitset.h"\
	".\Include\errcode.h"\
	".\Include\grammar.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\node.h"\
	".\Include\parsetok.h"\
	".\Include\pgenheaders.h"\
	".\Include\pydebug.h"\
	".\Include\token.h"\
	".\Parser\parser.h"\
	".\Parser\tokenizer.h"\
	".\PC\config.h"\
	

"$(INTDIR)\parsetok.obj" : $(SOURCE) $(DEP_CPP_PARSE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\parser.c
DEP_CPP_PARSER=\
	".\Include\bitset.h"\
	".\Include\errcode.h"\
	".\Include\grammar.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\node.h"\
	".\Include\pgenheaders.h"\
	".\Include\pydebug.h"\
	".\Include\token.h"\
	".\Parser\parser.h"\
	".\PC\config.h"\
	

"$(INTDIR)\parser.obj" : $(SOURCE) $(DEP_CPP_PARSER) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\object.c
DEP_CPP_OBJEC=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\object.obj" : $(SOURCE) $(DEP_CPP_OBJEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\node.c
DEP_CPP_NODE_=\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\node.h"\
	".\Include\pgenheaders.h"\
	".\Include\pydebug.h"\
	".\PC\config.h"\
	

"$(INTDIR)\node.obj" : $(SOURCE) $(DEP_CPP_NODE_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\newmodule.c
DEP_CPP_NEWMO=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\compile.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\newmodule.obj" : $(SOURCE) $(DEP_CPP_NEWMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\marshal.c
DEP_CPP_MARSH=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\compile.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longintrepr.h"\
	".\Include\longobject.h"\
	".\Include\marshal.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\marshal.obj" : $(SOURCE) $(DEP_CPP_MARSH) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\mystrtoul.c
DEP_CPP_MYSTR=\
	".\PC\config.h"\
	

"$(INTDIR)\mystrtoul.obj" : $(SOURCE) $(DEP_CPP_MYSTR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Parser\myreadline.c
DEP_CPP_MYREA=\
	".\Include\intrcheck.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\PC\config.h"\
	

"$(INTDIR)\myreadline.obj" : $(SOURCE) $(DEP_CPP_MYREA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\moduleobject.c
DEP_CPP_MODUL=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\moduleobject.obj" : $(SOURCE) $(DEP_CPP_MODUL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\modsupport.c
DEP_CPP_MODSU=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\modsupport.obj" : $(SOURCE) $(DEP_CPP_MODSU) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\methodobject.c
DEP_CPP_METHO=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\token.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\methodobject.obj" : $(SOURCE) $(DEP_CPP_METHO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\md5module.c
DEP_CPP_MD5MO=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\Modules\md5.h"\
	".\PC\config.h"\
	

"$(INTDIR)\md5module.obj" : $(SOURCE) $(DEP_CPP_MD5MO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\md5c.c
DEP_CPP_MD5C_=\
	".\Modules\md5.h"\
	".\PC\config.h"\
	

"$(INTDIR)\md5c.obj" : $(SOURCE) $(DEP_CPP_MD5C_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\mathmodule.c
DEP_CPP_MATHM=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\mymath.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\mathmodule.obj" : $(SOURCE) $(DEP_CPP_MATHM) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\socketmodule.c
DEP_CPP_SOCKE=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\mytime.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\socketmodule.obj" : $(SOURCE) $(DEP_CPP_SOCKE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\selectmodule.c
DEP_CPP_SELEC=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\myselect.h"\
	".\Include\mytime.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	

"$(INTDIR)\selectmodule.obj" : $(SOURCE) $(DEP_CPP_SELEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\sysmodule.c
DEP_CPP_SYSMO=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\osdefs.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\sysmodule.obj" : $(SOURCE) $(DEP_CPP_SYSMO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\import.c
DEP_CPP_IMPORT=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\compile.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\errcode.h"\
	".\Include\eval.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\marshal.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\node.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\osdefs.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\token.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	".\Python\importdl.h"\
	
NODEP_CPP_IMPORT=\
	".\Python\macglue.h"\
	

"$(INTDIR)\import.obj" : $(SOURCE) $(DEP_CPP_IMPORT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\posixmodule.c
DEP_CPP_POSIX=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\mytime.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	{$(INCLUDE)}"\sys\UTIME.H"\
	

"$(INTDIR)\posixmodule.obj" : $(SOURCE) $(DEP_CPP_POSIX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\operator.c
DEP_CPP_OPERA=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\operator.obj" : $(SOURCE) $(DEP_CPP_OPERA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\errnomodule.c
DEP_CPP_ERRNO=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\errnomodule.obj" : $(SOURCE) $(DEP_CPP_ERRNO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\sliceobject.c
DEP_CPP_SLICE=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\sliceobject.obj" : $(SOURCE) $(DEP_CPP_SLICE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\main.c
DEP_CPP_MAIN_=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

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
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\osdefs.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	".\Python\importdl.h"\
	

"$(INTDIR)\import_nt.obj" : $(SOURCE) $(DEP_CPP_IMPORT_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\getpath_nt.c
DEP_CPP_GETPA=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\osdefs.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\getpath_nt.obj" : $(SOURCE) $(DEP_CPP_GETPA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\dl_nt.c
DEP_CPP_DL_NT=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\dl_nt.obj" : $(SOURCE) $(DEP_CPP_DL_NT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\python_nt.def

!IF  "$(CFG)" == "python15 - Win32 Release"

!ELSEIF  "$(CFG)" == "python15 - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\threadmodule.c
DEP_CPP_THREAD=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\thread.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\threadmodule.obj" : $(SOURCE) $(DEP_CPP_THREAD) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\python_nt.rc

!IF  "$(CFG)" == "python15 - Win32 Release"

# ADD BASE RSC /l 0x409 /i "PC"
# ADD RSC /l 0x409 /i "PC" /i "Include"

"$(INTDIR)\python_nt.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/python_nt.res" /i "PC" /i "Include" /d\
 "NDEBUG" $(SOURCE)


!ELSEIF  "$(CFG)" == "python15 - Win32 Debug"

# ADD BASE RSC /l 0x409 /i "PC" /i "Include"
# ADD RSC /l 0x409 /i "PC" /i "Include"

"$(INTDIR)\python_nt.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/python_nt.res" /i "PC" /i "Include" $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\getbuildinfo.c
DEP_CPP_GETBU=\
	".\PC\config.h"\
	

"$(INTDIR)\getbuildinfo.obj" : $(SOURCE) $(DEP_CPP_GETBU) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Python\pystate.c
DEP_CPP_PYSTA=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\pystate.obj" : $(SOURCE) $(DEP_CPP_PYSTA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\cStringIO.c
DEP_CPP_CSTRI=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\cStringIO.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\cStringIO.obj" : $(SOURCE) $(DEP_CPP_CSTRI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\cPickle.c
DEP_CPP_CPICK=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\cStringIO.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\mymath.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\cPickle.obj" : $(SOURCE) $(DEP_CPP_CPICK) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Objects\dictobject.c
DEP_CPP_DICTO=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\dictobject.obj" : $(SOURCE) $(DEP_CPP_DICTO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\msvcrtmodule.c
DEP_CPP_MSVCR=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	

"$(INTDIR)\msvcrtmodule.obj" : $(SOURCE) $(DEP_CPP_MSVCR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
# End Target
################################################################################
# Begin Target

# Name "python - Win32 Release"
################################################################################
# Begin Source File

SOURCE=.\vc40\python15.lib
# End Source File
################################################################################
# Begin Source File

SOURCE=.\Modules\python.c

"$(INTDIR)\python.obj" : $(SOURCE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
# End Target
################################################################################
# Begin Target

# Name "_tkinter - Win32 Release"
################################################################################
# Begin Source File

SOURCE=.\Modules\_tkinter.c
DEP_CPP__TKIN=\
	".\Include\abstract.h"\
	".\Include\ceval.h"\
	".\Include\classobject.h"\
	".\Include\cobject.h"\
	".\Include\complexobject.h"\
	".\Include\dictobject.h"\
	".\Include\fileobject.h"\
	".\Include\floatobject.h"\
	".\Include\funcobject.h"\
	".\Include\import.h"\
	".\Include\intobject.h"\
	".\Include\intrcheck.h"\
	".\Include\listobject.h"\
	".\Include\longobject.h"\
	".\Include\methodobject.h"\
	".\Include\modsupport.h"\
	".\Include\moduleobject.h"\
	".\Include\mymalloc.h"\
	".\Include\myproto.h"\
	".\Include\myselect.h"\
	".\Include\mytime.h"\
	".\Include\object.h"\
	".\Include\objimpl.h"\
	".\Include\pydebug.h"\
	".\Include\pyerrors.h"\
	".\Include\pyfpe.h"\
	".\Include\pystate.h"\
	".\Include\Python.h"\
	".\Include\pythonrun.h"\
	".\Include\rangeobject.h"\
	".\Include\sliceobject.h"\
	".\Include\stringobject.h"\
	".\Include\sysmodule.h"\
	".\Include\traceback.h"\
	".\Include\tupleobject.h"\
	".\PC\config.h"\
	"C:\TCL\include\tcl.h"\
	"C:\TCL\include\tk.h"\
	"C:\TCL\include\X11\X.h"\
	"C:\TCL\include\X11\Xfuncproto.h"\
	"C:\TCL\include\X11\Xlib.h"\
	

"$(INTDIR)\_tkinter.obj" : $(SOURCE) $(DEP_CPP__TKIN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\_tkinter.def
# End Source File
################################################################################
# Begin Source File

SOURCE=.\vc40\python15.lib
# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\tk80.lib
# End Source File
################################################################################
# Begin Source File

SOURCE=.\PC\tcl80.lib
# End Source File
# End Target
# End Project
################################################################################
