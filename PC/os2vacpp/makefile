######################################################################
#
#          Top-Level Makefile for Building Python for OS/2
#
# This makefile was developed for use with IBM's VisualAge C/C++
# for OS/2 compiler, version 3.0, with Fixpack 8 applied.  It uses
# version 4.0 of the NMAKE tool that comes with that package.
#
# The output of the build is a largish Python23.DLL containing the
# essential modules of Python and a small Python.exe program to start
# the interpreter.  When embedding Python within another program, only
# Python23.DLL is needed.
#
# These two binaries can be statically linked with the VisualAge C/C++
# runtime library (producing larger binaries), or dynamically linked
# to make smaller ones that require the compiler to be installed on
# any system Python is used on.  Review the /Gd+ compiler option for
# how to do this.
#
# NOTE: IBM's NMAKE 4.0 is rather dumb, requiring this makefile to
#       be much more complicated than necessary.  I use OpusMAKE
#       myself for a much more powerful MAKE tool but not everyone
#       wishes to buy it.  However, as a result I didn't hook in
#       the dependencies on the include files as NMAKE has no easy
#       way to do this without explicitly spelling it all out.
#
# History (Most Recent First)
#
# 26-Sep-98 jrr Retested and adjusted for building w/Python 1.5.2a1
# 20-Nov-97 jrr Cleaned Up for Applying to Distribution
# 29-Oct-97 jrr Modified for Use with Python 1.5 Alpha 4
# 03-Aug-96 jrr Original for Use with Python 1.4 Release
#
######################################################################

###################
# Places and Things
###################
PY_MODULES      = ..\..\Modules
PY_OBJECTS      = ..\..\Objects
PY_PARSER       = ..\..\Parser
PY_PYTHON       = ..\..\Python
PY_INCLUDE      = ..\..\Include
PY_INCLUDES     = .;$(PY_INCLUDE);$(PY_MODULES);$(PY_OBJECTS);$(PY_PARSER);$(PY_PYTHON)

# File to Collect Wordy Compiler Output re Errors
ERRS		= make.out

# Where to Find the IBM TCP/IP Socket Includes and Libraries
OS2TCPIP        = C:\MPTN

# Where to Find the Tcl/Tk Base Directory for Libs/Includes
TCLTK		= D:\TclTk
TCLBASE		= D:\Tcl7.6\OS2
TKBASE		= D:\Tk4.2\OS2

# Where to Put the .OBJ Files, To Keep Them Out of the Way
PATHOBJ		= obj

# Search Path for Include Files
PROJINCLUDE	= .;$(TCLBASE);$(TKBASE);$(OS2TCPIP)\Include;$(PY_INCLUDES)

# Place to Search for Sources re OpusMAKE Dependency Generator (Commercial)
MKMF_SRCS	= $(PY_MODULES)\*.c $(PY_OBJECTS)\*.c $(PY_PARSER)\*.c $(PY_PYTHON)\*.c

#.HDRPATH.c	:= $(PROJINCLUDE,;= ) $(.HDRPATH.c)
#.PATH.c         = .;$(PY_MODULES);$(PY_OBJECTS);$(PY_PARSER);$(PY_PYTHON)
OTHERLIBS	= so32dll.lib tcp32dll.lib # Tcl76.lib Tk42.lib

#################
# Inference Rules
#################
{$(PY_MODULES)\}.c{$(PATHOBJ)\}.obj:	# Compile C Code into a .OBJ File
	@ Echo Compiling $<
	@ $(CC) -c $(CFLAGS) -Fo$@ $< >>$(ERRS)

{$(PY_OBJECTS)\}.c{$(PATHOBJ)\}.obj:	# Compile C Code into a .OBJ File
	@ Echo Compiling $<
	@ $(CC) -c $(CFLAGS) -Fo$@ $< >>$(ERRS)

{$(PY_PARSER)\}.c{$(PATHOBJ)\}.obj:	# Compile C Code into a .OBJ File
	@ Echo Compiling $<
	@ $(CC) -c $(CFLAGS) -Fo$@ $< >>$(ERRS)

{$(PY_PYTHON)\}.c{$(PATHOBJ)\}.obj:	# Compile C Code into a .OBJ File
	@ Echo Compiling $<
	@ $(CC) -c $(CFLAGS) -Fo$@ $< >>$(ERRS)

.c{$(PATHOBJ)\}.obj:			# Compile C Code into a .OBJ File
	@ Echo Compiling $<
	@ $(CC) -c $(CFLAGS) -Fo$@ $< >>$(ERRS)

###################
# Python Subsystems
###################

# PYTHON is the central core, containing the builtins and interpreter.
PYTHON		=                               \
                  $(PATHOBJ)\BltinModule.obj    \
                  $(PATHOBJ)\CEval.obj          \
                  $(PATHOBJ)\Compile.obj        \
                  $(PATHOBJ)\Errors.obj         \
                  $(PATHOBJ)\Frozen.obj         \
                  $(PATHOBJ)\Getargs.obj        \
                  $(PATHOBJ)\GetCompiler.obj    \
                  $(PATHOBJ)\GetCopyright.obj   \
                  $(PATHOBJ)\GetMTime.obj       \
                  $(PATHOBJ)\GetOpt.obj         \
                  $(PATHOBJ)\GetPlatform.obj    \
                  $(PATHOBJ)\GetVersion.obj     \
                  $(PATHOBJ)\GramInit.obj       \
                  $(PATHOBJ)\Import.obj         \
                  $(PATHOBJ)\ImportDL.obj       \
                  $(PATHOBJ)\Marshal.obj        \
                  $(PATHOBJ)\ModSupport.obj     \
                  $(PATHOBJ)\MyStrtoul.obj      \
                  $(PATHOBJ)\PyState.obj        \
                  $(PATHOBJ)\PythonRun.obj      \
                  $(PATHOBJ)\StructMember.obj   \
                  $(PATHOBJ)\SysModule.obj      \
                  $(PATHOBJ)\Thread.obj         \
                  $(PATHOBJ)\TraceBack.obj      \
                  $(PATHOBJ)\FrozenMain.obj     \
                  $(PATHOBJ)\exceptions.obj     \
                  $(PATHOBJ)\symtable.obj       \
                  $(PATHOBJ)\codecs.obj         \
                  $(PATHOBJ)\future.obj         \
                  $(PATHOBJ)\dynload_os2.obj    \
                  $(PATHOBJ)\mysnprintf.obj     \
                  $(PATHOBJ)\iterobject.obj

# Python's Internal Parser
PARSER		=                              \
                  $(PATHOBJ)\Acceler.obj       \
                  $(PATHOBJ)\Grammar1.obj      \
                  $(PATHOBJ)\ListNode.obj      \
                  $(PATHOBJ)\Node.obj          \
                  $(PATHOBJ)\Parser.obj        \
                  $(PATHOBJ)\ParseTok.obj      \
                  $(PATHOBJ)\BitSet.obj        \
                  $(PATHOBJ)\MetaGrammar.obj   \
                  $(PATHOBJ)\Tokenizer.obj     \
                  $(PATHOBJ)\MyReadline.obj

# Python Object Types
OBJECTS		=                              \
                  $(PATHOBJ)\Abstract.obj      \
                  $(PATHOBJ)\BoolObject.obj    \
                  $(PATHOBJ)\BufferObject.obj  \
                  $(PATHOBJ)\CellObject.obj    \
                  $(PATHOBJ)\ClassObject.obj   \
                  $(PATHOBJ)\CObject.obj       \
                  $(PATHOBJ)\ComplexObject.obj \
                  $(PATHOBJ)\DescrObject.obj   \
                  $(PATHOBJ)\DictObject.obj    \
                  $(PATHOBJ)\EnumObject.obj    \
                  $(PATHOBJ)\FileObject.obj    \
                  $(PATHOBJ)\FloatObject.obj   \
                  $(PATHOBJ)\FrameObject.obj   \
                  $(PATHOBJ)\FuncObject.obj    \
                  $(PATHOBJ)\IntObject.obj     \
                  $(PATHOBJ)\IterObject.obj    \
                  $(PATHOBJ)\ListObject.obj    \
                  $(PATHOBJ)\LongObject.obj    \
                  $(PATHOBJ)\MethodObject.obj  \
                  $(PATHOBJ)\ModuleObject.obj  \
                  $(PATHOBJ)\Object.obj        \
                  $(PATHOBJ)\ObMalloc.obj      \
                  $(PATHOBJ)\RangeObject.obj   \
                  $(PATHOBJ)\SliceObject.obj   \
                  $(PATHOBJ)\StringObject.obj  \
                  $(PATHOBJ)\StructSeq.obj     \
                  $(PATHOBJ)\TupleObject.obj   \
                  $(PATHOBJ)\TypeObject.obj    \
                  $(PATHOBJ)\UnicodeObject.obj \
                  $(PATHOBJ)\UnicodeCType.obj  \
                  $(PATHOBJ)\WeakrefObject.obj

# Extension Modules (Built-In or as Separate DLLs)
MODULES		=                              \
                  $(PATHOBJ)\ArrayModule.obj   \
                  $(PATHOBJ)\BinAscii.obj      \
                  $(PATHOBJ)\CMathModule.obj   \
                  $(PATHOBJ)\cPickle.obj       \
                  $(PATHOBJ)\cStringIO.obj     \
                  $(PATHOBJ)\ErrnoModule.obj   \
                  $(PATHOBJ)\GCModule.obj      \
                  $(PATHOBJ)\GetBuildInfo.obj  \
                  $(PATHOBJ)\GetPathP.obj      \
                  $(PATHOBJ)\Main.obj          \
                  $(PATHOBJ)\MathModule.obj    \
                  $(PATHOBJ)\MD5c.obj          \
                  $(PATHOBJ)\MD5Module.obj     \
                  $(PATHOBJ)\Operator.obj      \
                  $(PATHOBJ)\PCREModule.obj    \
                  $(PATHOBJ)\PyPCRE.obj        \
                  $(PATHOBJ)\PosixModule.obj   \
                  $(PATHOBJ)\RegexModule.obj   \
                  $(PATHOBJ)\RegExpr.obj       \
                  $(PATHOBJ)\SelectModule.obj  \
                  $(PATHOBJ)\SignalModule.obj  \
                  $(PATHOBJ)\SocketModule.obj  \
                  $(PATHOBJ)\StropModule.obj   \
                  $(PATHOBJ)\StructModule.obj  \
                  $(PATHOBJ)\TimeModule.obj    \
                  $(PATHOBJ)\ThreadModule.obj  \
                  $(PATHOBJ)\YUVConvert.obj

# Standalone Parser Generator Program (Shares Some of Python's Modules)
PGEN            =                              \
                  $(PATHOBJ)\PGen.obj          \
                  $(PATHOBJ)\PGenMain.obj      \
                  $(PATHOBJ)\MySNPrintf.obj    \
                  $(PATHOBJ)\Tokenizer_Pgen.obj \
                  $(PATHOBJ)\PrintGrammar.obj  \
                  $(PATHOBJ)\Grammar.obj       \
                  $(PATHOBJ)\FirstSets.obj

##################
# Macros and Flags
##################
_BASE		= /Q /W2 /I$(PROJINCLUDE)
		# /Q   = Omit IBM Copyright
		# /W2  = Show Warnings/Errors Only

_GEN		= /G4 /Gm /Gd-
		# /G4  = Generate Code for 486 (Use 386 for Debugger)
		# /Gm  = Use Multithread Runtime
		# /Gd  = Dynamically Load Runtime
		# /Ms  = Use _System Calling Convention (vs _Optlink)
		#        (to allow non-VAC++ code to call into Python23.dll)

_OPT		= /O /Gl
		# /O   = Enable Speed-Optimizations
		# /Ol  = Pass Code Thru Intermediate Linker
		# /Gu  = Advise Linker All Ext Data is ID'd
		# /Gl  = Have Linker Remove Unused Fns

_DBG		= /Wpro- /Ti- /DHAVE_CONFIG_H /DUSE_SOCKET
                # /Wpro= Generate Compiler Warnings re Missing Prototypes
		# /Ti  = Embed Debugger/Analyzer Recs
		# /Tm  = Enable Debug Memory Fns
		# /Tx  = Request Full Dump Upon Exception
		# /DHAVE_CONFIG_H = Causes Use of CONFIG.H Settings
                # /DUSE_SOCKET = Enables Building In of Socket API

_OUT		= 
		# /Fb  = Embed Browser Recs
		# /Gh  = Generate Code for Profiler Hooks
		# /Fl  = Output C/C++ Listing Files
                # /Lf  = Provide Full (Detailed) Listing Files
		# /Fm. = Output Linker Map File
		# /Ft. = Output C++ Template Resolution Files

_MAP		= /FmNoise\$(@R).map

_DLL		= /Ge-
_EXE		= /Ge
		# /Ge = Create an EXE, not DLL

CFLAGS		= $(_BASE) $(_GEN) $(_OPT) $(_DBG) $(_OUT) $(_EXE) /Ss

###################
# Primary Target(s)
###################
All:  obj noise PyCore.lib Python23.lib PGen.exe \
      Python.exe PythonPM.exe Python23.dll # _tkinter.dll

Modules: $(MODULES)
Objects: $(OBJECTS)
Parser:  $(PARSER)
Python:  $(PYTHON)

# Directory to Keep .OBJ Files Out of the Way
obj:
	@-mkdir obj >>NUL

# Directory to Keep .MAP and Such Text Files Out of the Way
noise:
	@-mkdir noise >>NUL

##############
#
##############

# Python Extension DLL: Tcl/Tk Interface
#_tkinter.dll: $(PATHOBJ)\_tkinter.obj Python23.lib _tkinter.def
#	@ Echo Linking $@ As DLL
#	@ $(CC) $(CFLAGS) /B"/NOE" $(_DLL) /Fe$@ $(_MAP) $** $(OTHERLIBS) >>$(ERRS)

#$(PATHOBJ)\_tkinter.obj: $(PY_MODULES)\_tkinter.c
#	@ Echo Compiling $**
#	@ $(CC) -c $(CFLAGS) $(_DLL) -Fo$@ $** >>$(ERRS)

# Object Library of All Essential Python Routines
PyCore.lib: $(MODULES) $(OBJECTS) $(PARSER) $(PYTHON) $(PATHOBJ)\Config.obj
	@ Echo Adding Updated Object Files to Link Library $@
	@ ! ILIB $@ /NOLOGO /NOBACKUP -+$? ; >>$(ERRS)

Python23.dll: $(PATHOBJ)\Compile.obj PyCore.lib Python.def
	@ Echo Linking $@ As DLL
	@ $(CC) $(CFLAGS) /B"/NOE" $(_DLL) /Fe$@ $(_MAP) $** $(OTHERLIBS) >>$(ERRS)
#	@ Echo Compressing $@ with LxLite
#	@ lxlite $@

# IBM Linker Requires One Explicit .OBJ To Build a .DLL from a .LIB
$(PATHOBJ)\Compile.obj: $(PY_PYTHON)\Compile.c
	@ Echo Compiling $**
	@ $(CC) -c $(CFLAGS) $(_DLL) -Fo$@ $** >>$(ERRS)

# Import Library for Using the Python23.dll
Python23.lib: Python.def
	@ Echo Making $@
	@ IMPLIB /NOLOGO /NOIGNORE $@ $** >>$(ERRS)
	@ ILIB /NOLOGO /CONVFORMAT /NOEXTDICTIONARY /NOBROWSE /NOBACKUP $@; >>$(ERRS)

# Small Command-Line Program to Start Interpreter in Python23.dll
Python.exe: $(PATHOBJ)\Python.obj Python23.lib
	@ Echo Linking $@ As EXE
	@ $(CC) $(CFLAGS) $(_EXE) /B"/PM:VIO /STACK:360000" /Fe$@ $(_MAP) $** $(OTHERLIBS) >>$(ERRS)

# Small PM-GUI Program to Start Interpreter in Python23.dll
PythonPM.exe: $(PATHOBJ)\Python.obj Python23.lib
	@ Echo Linking $@ As EXE
	@ $(CC) $(CFLAGS) $(_EXE) /B"/PM:PM /STACK:360000" /Fe$@ $(_MAP) $** $(OTHERLIBS) >>$(ERRS)

PGen.exe: $(PGEN) PyCore.lib
	@ Echo Linking $@ As EXE
	@ $(CC) $(CFLAGS) $(_EXE) /B"/STACK:360000" /Fe$@ $(_MAP) $** $(OTHERLIBS) >>$(ERRS)

#######################
# Miscellaneous Targets
#######################

# Remove Intermediate Targets but Leave Executable Binaries
clean:
	-- Del /Q $(PATHOBJ)\*.obj		>NUL 2>&1
	-- Del /Q /Y Noise			>NUL 2>&1
	-- Del /Q $(ERRS)			>NUL 2>&1

# Remove All Targets, Including Final Binaries
distclean: clean
        -- Del /Q PyCore.lib Python23.lib       >NUL 2>&1
        -- Del /Q Python23.dll Python.exe PGen.exe >NUL 2>&1

release: Python.exe Python23.dll Python23.lib
	-- @Echo Y | copy /U Python.exe   D:\EXEs
	-- @Echo Y | copy /U Python23.dll D:\DLLs
	-- @Echo Y | copy /U Python23.lib E:\Tau\Lib
	-- @Echo Y | copy /U _tkinter.dll D:\Python

test:
        python ..\..\lib\test\regrtest.py

# Update Dependencies on Targets (Uses OpusMAKE Commercial Product)
depend:
	D:\OpusMake\os2mkmf -c -s

### OPUS MKMF:  Do not remove this line!  Generated dependencies follow.

_tkinter.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

almodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

arraymodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

audioop.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\mymath.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

binascii.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

bsddbmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

cdmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

cgensupport.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_MODULES)\cgensupport.h $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h \
	 $(PY_INCLUDE)\complexobject.h pyconfig.h $(PY_INCLUDE)\dictobject.h \
	 $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h \
	 $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h \
	 $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

clmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

cmathmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\mymath.h $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h \
	 $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h \
	 $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h \
	 $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

cpickle.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\cstringio.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\mymath.h $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h \
	 $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h \
	 $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h \
	 $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

cryptmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

cstringio.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\cstringio.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

cursesmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

dbmmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

dlmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

errno.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

errnomodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

fcntlmodule.obj: $(PY_INCLUDE)\abstract.h $(OS2TCPIP)\Include\sys\ioctl.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

flmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\structmember.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

fmmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

fpectlmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

fpetestmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

gdbmmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

getbuildinfo.obj: pyconfig.h

getpath.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\osdefs.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

glmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_MODULES)\cgensupport.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

grpmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(OS2TCPIP)\Include\grp.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

imageop.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

imgfile.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

main.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

mathmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\mymath.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

md5c.obj: pyconfig.h $(PY_MODULES)\md5.h

md5module.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_MODULES)\md5.h $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

mpzmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longintrepr.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

nismodule.obj: $(PY_INCLUDE)\abstract.h $(OS2TCPIP)\Include\sys\time.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

operator.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

parsermodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\compile.h \
	 $(PY_INCLUDE)\complexobject.h pyconfig.h $(PY_INCLUDE)\dictobject.h \
	 $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h \
	 $(PY_INCLUDE)\graminit.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\node.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\token.h \
	 $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

pcremodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_MODULES)\pcre-internal.h \
	 $(PY_MODULES)\pcre.h $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h \
	 $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h \
	 $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

posix.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\mytime.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

posixmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\mytime.h $(PY_INCLUDE)\object.h \
	 $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h \
	 $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h \
	 $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

puremodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

pwdmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(OS2TCPIP)\Include\pwd.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

pypcre.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\graminit.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_MODULES)\pcre-internal.h $(PY_MODULES)\pcre.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

readline.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

regexmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_MODULES)\regexpr.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

regexpr.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_MODULES)\regexpr.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

resource.obj: $(PY_INCLUDE)\abstract.h $(OS2TCPIP)\Include\sys\time.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

rgbimgmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

selectmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\myselect.h $(PY_INCLUDE)\mytime.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

sgimodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

signalmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

socketmodule.obj: $(PY_INCLUDE)\abstract.h $(OS2TCPIP)\Include\netinet\in.h \
	 $(OS2TCPIP)\Include\sys\socket.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\mytime.h $(OS2TCPIP)\Include\netdb.h $(PY_INCLUDE)\object.h \
	 $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h \
	 $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h \
	 $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

soundex.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

stdwinmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

stropmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

structmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\mymath.h $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h \
	 $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h \
	 $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h \
	 $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

sunaudiodev.obj: $(PY_INCLUDE)\abstract.h $(OS2TCPIP)\Include\sys\ioctl.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\structmember.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

svmodule.obj: $(PY_INCLUDE)\abstract.h $(OS2TCPIP)\Include\sys\time.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\compile.h \
	 $(PY_INCLUDE)\complexobject.h pyconfig.h $(PY_INCLUDE)\dictobject.h \
	 $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h \
	 $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h \
	 $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h \
	 $(PY_MODULES)\yuv.h

syslogmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(OS2TCPIP)\Include\syslog.h $(PY_INCLUDE)\sysmodule.h \
	 $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

termios.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

threadmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\thread.h \
	 $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

timemodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\mytime.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

timingmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_MODULES)\timing.h \
	 $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

xxmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

yuvconvert.obj: $(PY_MODULES)\yuv.h

zlibmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

abstract.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

classobject.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\structmember.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

cobject.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

complexobject.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\mymath.h $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h \
	 $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h \
	 $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h \
	 $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

dictobject.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

fileobject.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\structmember.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

floatobject.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\mymath.h $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h \
	 $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h \
	 $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h \
	 $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

frameobject.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\compile.h \
	 $(PY_INCLUDE)\complexobject.h pyconfig.h $(PY_INCLUDE)\dictobject.h \
	 $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\frameobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\opcode.h $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h \
	 $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h \
	 $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\structmember.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

funcobject.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\compile.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\structmember.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

intobject.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

listobject.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

longobject.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longintrepr.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\mymath.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

methodobject.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\token.h \
	 $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

moduleobject.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

object.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

rangeobject.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

sliceobject.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

stringobject.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\mymath.h $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h \
	 $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h \
	 $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h \
	 $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

tupleobject.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

typeobject.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

xxobject.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

acceler.obj: $(PY_INCLUDE)\bitset.h pyconfig.h $(PY_INCLUDE)\grammar.h \
	 $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\node.h \
	 $(PY_PARSER)\parser.h $(PY_INCLUDE)\pgenheaders.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\token.h

bitset.obj: $(PY_INCLUDE)\bitset.h pyconfig.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\pgenheaders.h $(PY_INCLUDE)\pydebug.h

firstsets.obj: $(PY_INCLUDE)\bitset.h pyconfig.h $(PY_INCLUDE)\grammar.h \
	 $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\pgenheaders.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\token.h

grammar.obj: $(PY_INCLUDE)\bitset.h pyconfig.h \
	 $(PY_INCLUDE)\grammar.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\pgenheaders.h $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\token.h

grammar1.obj: $(PY_INCLUDE)\bitset.h pyconfig.h \
	 $(PY_INCLUDE)\grammar.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\pgenheaders.h $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\token.h

intrcheck.obj: pyconfig.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h

listnode.obj: pyconfig.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\node.h $(PY_INCLUDE)\pgenheaders.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\token.h

metagrammar.obj: $(PY_INCLUDE)\bitset.h pyconfig.h $(PY_INCLUDE)\grammar.h \
	 $(PY_INCLUDE)\metagrammar.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_PARSER)\pgen.h $(PY_INCLUDE)\pgenheaders.h $(PY_INCLUDE)\pydebug.h

myreadline.obj: pyconfig.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h

node.obj: pyconfig.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\node.h \
	 $(PY_INCLUDE)\pgenheaders.h $(PY_INCLUDE)\pydebug.h

parser.obj: $(PY_INCLUDE)\bitset.h pyconfig.h $(PY_INCLUDE)\errcode.h \
	 $(PY_INCLUDE)\grammar.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\node.h $(PY_PARSER)\parser.h $(PY_INCLUDE)\pgenheaders.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\token.h

parsetok.obj: $(PY_INCLUDE)\bitset.h pyconfig.h $(PY_INCLUDE)\errcode.h \
	 $(PY_INCLUDE)\grammar.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\node.h $(PY_PARSER)\parser.h $(PY_INCLUDE)\parsetok.h \
	 $(PY_INCLUDE)\pgenheaders.h $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\token.h \
	 $(PY_PARSER)\tokenizer.h

pgen.obj: $(PY_INCLUDE)\bitset.h pyconfig.h $(PY_INCLUDE)\grammar.h \
	 $(PY_INCLUDE)\metagrammar.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\node.h $(PY_PARSER)\pgen.h $(PY_INCLUDE)\pgenheaders.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\token.h

pgenmain.obj: $(PY_INCLUDE)\bitset.h pyconfig.h $(PY_INCLUDE)\grammar.h \
	 $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\node.h \
	 $(PY_INCLUDE)\parsetok.h $(PY_PARSER)\pgen.h $(PY_INCLUDE)\pgenheaders.h \
	 $(PY_INCLUDE)\pydebug.h

printgrammar.obj: $(PY_INCLUDE)\bitset.h pyconfig.h $(PY_INCLUDE)\grammar.h \
	 $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\pgenheaders.h \
	 $(PY_INCLUDE)\pydebug.h

tokenizer.obj: pyconfig.h $(PY_INCLUDE)\errcode.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\pgenheaders.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\token.h $(PY_PARSER)\tokenizer.h

atof.obj: pyconfig.h

bltinmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\compile.h \
	 $(PY_INCLUDE)\complexobject.h pyconfig.h $(PY_INCLUDE)\dictobject.h \
	 $(PY_INCLUDE)\eval.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\mymath.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\node.h $(PY_INCLUDE)\object.h \
	 $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h \
	 $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h \
	 $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

ceval.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\compile.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\eval.h \
	 $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\frameobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\opcode.h $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h \
	 $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h \
	 $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

compile.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\compile.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\graminit.h \
	 $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h \
	 $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\node.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\opcode.h $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h \
	 $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h \
	 $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\structmember.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\token.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

errors.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

fmod.obj: pyconfig.h $(PY_INCLUDE)\mymath.h

frozen.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

frozenmain.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

getargs.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

getcompiler.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

getcopyright.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

getmtime.obj: pyconfig.h

getplatform.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

getversion.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\patchlevel.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

graminit.obj: $(PY_INCLUDE)\bitset.h pyconfig.h $(PY_INCLUDE)\grammar.h \
	 $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\pgenheaders.h \
	 $(PY_INCLUDE)\pydebug.h

hypot.obj: pyconfig.h $(PY_INCLUDE)\mymath.h $(PY_INCLUDE)\myproto.h

import.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\compile.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\errcode.h $(PY_INCLUDE)\eval.h \
	 $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h \
	 $(PY_INCLUDE)\import.h $(PY_PYTHON)\importdl.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\marshal.h $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\node.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\osdefs.h $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h \
	 $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h \
	 $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\token.h \
	 $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

importdl.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_PYTHON)\importdl.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\osdefs.h $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h \
	 $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h \
	 $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

marshal.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\compile.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longintrepr.h $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\marshal.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

modsupport.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

mystrtoul.obj: pyconfig.h

pyfpe.obj: pyconfig.h $(PY_INCLUDE)\pyfpe.h

pystate.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

pythonrun.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\bitset.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\compile.h \
	 $(PY_INCLUDE)\complexobject.h pyconfig.h $(PY_INCLUDE)\dictobject.h \
	 $(PY_INCLUDE)\errcode.h $(PY_INCLUDE)\eval.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\grammar.h \
	 $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h \
	 $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\marshal.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\node.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\parsetok.h $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h \
	 $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h \
	 $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

sigcheck.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\pydebug.h \
	 $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h $(PY_INCLUDE)\pystate.h \
	 $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h $(PY_INCLUDE)\rangeobject.h \
	 $(PY_INCLUDE)\sliceobject.h $(PY_INCLUDE)\stringobject.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

strdup.obj: pyconfig.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h

strtod.obj: pyconfig.h

structmember.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h \
	 $(PY_INCLUDE)\classobject.h $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h \
	 $(PY_INCLUDE)\intobject.h $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h \
	 $(PY_INCLUDE)\longobject.h $(PY_INCLUDE)\methodobject.h \
	 $(PY_INCLUDE)\modsupport.h $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h \
	 $(PY_INCLUDE)\myproto.h $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\structmember.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

sysmodule.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\complexobject.h pyconfig.h \
	 $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h $(PY_INCLUDE)\floatobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\osdefs.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h \
	 $(PY_INCLUDE)\tupleobject.h

thread.obj: pyconfig.h $(PY_INCLUDE)\thread.h

traceback.obj: $(PY_INCLUDE)\abstract.h $(PY_INCLUDE)\ceval.h $(PY_INCLUDE)\classobject.h \
	 $(PY_INCLUDE)\cobject.h $(PY_INCLUDE)\compile.h $(PY_INCLUDE)\complexobject.h \
	 pyconfig.h $(PY_INCLUDE)\dictobject.h $(PY_INCLUDE)\fileobject.h \
	 $(PY_INCLUDE)\floatobject.h $(PY_INCLUDE)\frameobject.h \
	 $(PY_INCLUDE)\funcobject.h $(PY_INCLUDE)\import.h $(PY_INCLUDE)\intobject.h \
	 $(PY_INCLUDE)\intrcheck.h $(PY_INCLUDE)\listobject.h $(PY_INCLUDE)\longobject.h \
	 $(PY_INCLUDE)\methodobject.h $(PY_INCLUDE)\modsupport.h \
	 $(PY_INCLUDE)\moduleobject.h $(PY_INCLUDE)\mymalloc.h $(PY_INCLUDE)\myproto.h \
	 $(PY_INCLUDE)\object.h $(PY_INCLUDE)\objimpl.h $(PY_INCLUDE)\osdefs.h \
	 $(PY_INCLUDE)\pydebug.h $(PY_INCLUDE)\pyerrors.h $(PY_INCLUDE)\pyfpe.h \
	 $(PY_INCLUDE)\pystate.h $(PY_INCLUDE)\python.h $(PY_INCLUDE)\pythonrun.h \
	 $(PY_INCLUDE)\rangeobject.h $(PY_INCLUDE)\sliceobject.h \
	 $(PY_INCLUDE)\stringobject.h $(PY_INCLUDE)\structmember.h \
	 $(PY_INCLUDE)\sysmodule.h $(PY_INCLUDE)\traceback.h $(PY_INCLUDE)\tupleobject.h

