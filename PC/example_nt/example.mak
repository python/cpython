# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=example - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to example - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "example - Win32 Release" && "$(CFG)" !=\
 "example - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "example.mak" CFG="example - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "example - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "example - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "example - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "example - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\example.dll"

CLEAN : 
	-@erase ".\Release\example.dll"
	-@erase ".\Release\example.obj"
	-@erase ".\Release\example.lib"
	-@erase ".\Release\example.exp"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../Include" /I "../PC" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "../Include" /I "../PC" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)/example.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/example.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/example.pdb" /machine:I386 /def:".\example.def"\
 /out:"$(OUTDIR)/example.dll" /implib:"$(OUTDIR)/example.lib" 
DEF_FILE= \
	".\example.def"
LINK32_OBJS= \
	"$(INTDIR)/example.obj" \
	"..\vc40\python14.lib"

"$(OUTDIR)\example.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "example - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\example.dll"

CLEAN : 
	-@erase ".\Debug\example.dll"
	-@erase ".\Debug\example.obj"
	-@erase ".\Debug\example.ilk"
	-@erase ".\Debug\example.lib"
	-@erase ".\Debug\example.exp"
	-@erase ".\Debug\example.pdb"
	-@erase ".\Debug\vc40.pdb"
	-@erase ".\Debug\vc40.idb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "../Include" /I "../PC" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "../Include" /I "../PC" /D "WIN32"\
 /D "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)/example.pch" /YX /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/example.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/example.pdb" /debug /machine:I386 /def:".\example.def"\
 /out:"$(OUTDIR)/example.dll" /implib:"$(OUTDIR)/example.lib" 
DEF_FILE= \
	".\example.def"
LINK32_OBJS= \
	"$(INTDIR)/example.obj" \
	"..\vc40\python14.lib"

"$(OUTDIR)\example.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

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

################################################################################
# Begin Target

# Name "example - Win32 Release"
# Name "example - Win32 Debug"

!IF  "$(CFG)" == "example - Win32 Release"

!ELSEIF  "$(CFG)" == "example - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\example.c
DEP_CPP_EXAMP=\
	".\../Include\Python.h"\
	"..\Include\allobjects.h"\
	".\../PC\config.h"\
	"..\Include\myproto.h"\
	"..\Include\object.h"\
	"..\Include\objimpl.h"\
	"..\Include\pydebug.h"\
	"..\Include\accessobject.h"\
	"..\Include\intobject.h"\
	"..\Include\longobject.h"\
	"..\Include\floatobject.h"\
	"..\Include\complexobject.h"\
	"..\Include\rangeobject.h"\
	"..\Include\stringobject.h"\
	"..\Include\tupleobject.h"\
	"..\Include\listobject.h"\
	"..\Include\mappingobject.h"\
	"..\Include\methodobject.h"\
	"..\Include\moduleobject.h"\
	"..\Include\funcobject.h"\
	"..\Include\classobject.h"\
	"..\Include\fileobject.h"\
	"..\Include\cobject.h"\
	"..\Include\traceback.h"\
	"..\Include\sliceobject.h"\
	"..\Include\pyerrors.h"\
	"..\Include\mymalloc.h"\
	"..\Include\modsupport.h"\
	"..\Include\ceval.h"\
	"..\Include\pythonrun.h"\
	"..\Include\sysmodule.h"\
	"..\Include\intrcheck.h"\
	"..\Include\import.h"\
	"..\Include\bltinmodule.h"\
	"..\Include\abstract.h"\
	"..\Include\rename2.h"\
	"..\Include\thread.h"\
	

"$(INTDIR)\example.obj" : $(SOURCE) $(DEP_CPP_EXAMP) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\example.def

!IF  "$(CFG)" == "example - Win32 Release"

!ELSEIF  "$(CFG)" == "example - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=..\vc40\python14.lib

!IF  "$(CFG)" == "example - Win32 Release"

!ELSEIF  "$(CFG)" == "example - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\readme.txt

!IF  "$(CFG)" == "example - Win32 Release"

!ELSEIF  "$(CFG)" == "example - Win32 Debug"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
