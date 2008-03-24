#####################==================----------------
#
# Top-Level Makefile for Building Python 2.6 for OS/2 using GCC/EMX
# Originally written by Andrew Zabolotny, <bit@eltech.ru> for Python 1.5.2
# Modified by Andrew MacIntyre, <andymac@pcug.org.au> for Python 2.6
#
# This makefile was developed for use with [P]GCC/EMX compiler any
# version and GNU Make.
#
# The output of the build is a largish Python26.DLL containing the
# essential modules of Python and a small Python.exe program to start
# the interpreter. When embedding Python within another program, only
# Python26.DLL is needed. We also build python_s.a static library (which
# can be converted into OMF (.lib) format using emxomf tool) and both
# python.a and python.lib import libraries.  Then the optional 
# extension modules, which are OS/2 DLLs renamed with a PYD file extension.
#
# Recommended build order:
#   make depend		(if you have makedep)
#   make all
#   make lx		(if you have lxlite)
#   make test		(optional)
#
#####################==================----------------

# === Compilation mode: debug or release ===
MODE=		optimize
#MODE=		debug
# === Assert() enabled ===
ASSERTIONS=no
#ASSERTIONS=yes
# === Hard-wire installation location ===
FIXED_PYHOME=no
#FIXED_PYHOME=yes

# === Optional modules ===
# Do you have the InfoZip compression library installed?
HAVE_ZLIB=	no
# Do you have the Ultra Fast Crypt (UFC) library installed?
HAVE_UFC=	no
# Do you have the Tcl/Tk library installed?
HAVE_TCLTK=	no
# Do you have the GNU readline library installed?
# NOTE: I'm using a modified version of Kai Uwe Rommel's port that 
#       - is compiled with multithreading enabled
#       - is linked statically
#       I have had no success trying to use a DLL version, even when
#       compiled with multithreading enabled.
HAVE_GREADLINE=	no
# Do you have the BSD DB library (v1.85) as included in the EMXBSD package?
# NOTE: this library needs to be recompiled with a structure member
#       renamed to avoid problems with the multithreaded errno support
#       (there is a structure member called errno, used for shadowing the
#       real errno, which conflicts with the errno redefinition of -Zmt)
HAVE_BSDDB=	no
# Do you have the ncurses library installed? EMX's BSD curses aren't enough! 
HAVE_NCURSES=	no
# Do you have the GDBM library installed?
HAVE_GDBM=	no
# Do you have the BZ2 compression library installed?
HAVE_BZ2=	no
# Do you have the OpenSSL libraries installed
HAVE_OPENSSL=	no

# === install locations ===
# default value of PYTHONHOME
LIB_DIR=C:/Python26
# default is to have everything in or under PYTHONHOME
EXE_DIR=$(LIB_DIR)
DLL_DIR=$(EXE_DIR)


# === The Tools ===
CC=		gcc
CFLAGS=		-Zmt -Wall $(INCLUDE)
CFLAGS.LIB=	$(CFLAGS)
LD=		gcc
LDFLAGS=	-Zmt -Zcrtdll -L. -lgcc
LDFLAGS.EXE=	$(LDFLAGS)
LDFLAGS.DLL=	$(LDFLAGS) -Zdll
LDFLAGS.A=	$(LDFLAGS) $(LIBS)
ARFLAGS=	crs
IMPLIB=		emximp
EXPLIB=		emxexp
EXEOPT=		emxbind
PY_DEF=		-DPy_BUILD_CORE


# adjust C compiler settings based on build options
ifeq ($(MODE),debug)
  CFLAGS+=	-g -O
  LDFLAGS+=	-g
else
  CFLAGS+=	-s -O3 -fomit-frame-pointer -mprobe
  LDFLAGS+=	-s
endif
CFLAGS+=	$(PY_DEF)
ifeq ($(ASSERTIONS),no)
  CFLAGS+=	-DNDEBUG
endif
ifeq ($(FIXED_PYHOME),yes)
  CFLAGS+=	-DPREFIX=$(DQUOTE)$(LIB_DIR)$(DQUOTE)
endif

# We're using the OMF format since EMX's ld has a obscure bug
# because of which it sometimes fails to build relocations
# in .data segment that point to another .data locations
# (except for the final linking if the .EXEs)
OMF=		yes

# if fork() support is required, the main executable must be linked with ld
EXEOMF=		no

# File extensions
MODULE.EXT=	.pyd
MODLIB.EXT=	.dll
ifeq ($(OMF),yes)
  O=		.obj
  A=		.lib
  AR=		emxomfar
  CFLAGS+=	-Zomf
  LDFLAGS+=	-Zomf
  ifeq ($(MODE),debug)
    ARFLAGS=	-p64 crs
  else
    ARFLAGS=	-p32 crs
  endif
else
  O=		.o
  A=		.a
  AR=		ar
endif


# === Build time resource settings ===

# EMX's default number of file handles is 40, which is sometimes insufficient
# (the tempfile regression test tries to create 100 temporary files)
NFILES=250

# The default stack size for child threads is 64k bytes, which is
# insufficient for some applications which do a lot of work in threads
# (such as Zope, especially in conjunction with Plone).
# Note that this setting is distinct from the stack size for the main
# thread, which is set via the %.def rule below.
# EMX documents that the thread stack size should be at least 32768 bytes;
# for Zope/Plone at least 128k bytes is recommended.
# Uncomment & adjust the next line to override the default stack size:
#CFLAGS+=	-DTHREAD_STACK_SIZE=0x20000


# === The environment ===

# Source file paths
SRCPATH=.;../../Python;../../Parser;../../Objects;../../Include;../../Modules
# Python contains the central core, containing the builtins and interpreter.
# Parser contains Python's Internal Parser and
#   Standalone Parser Generator Program (Shares Some of Python's Modules)
# Objects contains Python Object Types
# Modules contains extension Modules (Built-In or as Separate DLLs)

# Unix shells tend to use "$" as delimiter for variable names.
# Test for this behaviour and set $(BUCK) variable correspondigly ...
__TMP__:=$(shell echo $$$$)
ifeq ($(__TMP__),$$$$)
  BUCK=		$$
  BRO=		(
  BRC=		)
else
  BUCK=		\$$
  BRO=		\(
  BRC=		\)
endif
# Compute the "double quote" variable
__TMP__:=$(shell echo "")
ifeq ($(__TMP__),"")
  DQUOTE=	"
else
  DQUOTE=	\"
endif

# Include paths
#INCLUDE=	-I$(subst ;, -I, $(SRCPATH))
INCLUDE=	-I. -I../../Include

# Path to search for .c files
vpath %.c .;..;$(SRCPATH)

# Top of the package tree
TOP=		../../

# Directory for output files
OUTBASE=	out/
OUT=		$(OUTBASE)$(MODE)/

# Additional libraries
LIBS=		-lsocket

# Utility macro: replacement for $^
^^=		$(filter-out %$A,$^)
# Use $(L^) to link with all libraries specified as dependencies
L^=		$(addprefix -l,$(basename $(notdir $(filter %$A,$+))))

# Build rules
$(OUT)%$O: %.c
	$(CC) $(CFLAGS.LIB) -c $< -o $@

%.a:
	$(LD) $(LDFLAGS.A) -o $@ $(^^) $(L^)

%.dll:
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS)

%.pyd: $(OUT)%module$O $(OUT)%_m.def
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(PYTHON.IMPLIB) $(LIBS)

%.exe:
	$(LD) $(LDFLAGS.EXE) -o $@ $(^^) $(L^)

%_m.def:
	@echo Creating .DEF file: $@
	@echo LIBRARY $(notdir $*) INITINSTANCE TERMINSTANCE >$@
        ifeq ($(DESCRIPTION.$(notdir $*)$(MODULE.EXT)),)
	  @echo DESCRIPTION $(DQUOTE)Python standard module $(notdir $*)$(DQUOTE) >>$@
        else
	  @echo DESCRIPTION $(DQUOTE)$(DESCRIPTION.$(notdir $*)$(MODULE.EXT))$(DQUOTE) >>$@
        endif
	@echo DATA MULTIPLE NONSHARED >>$@
	@echo EXPORTS >>$@
	@echo 	init$(notdir $*) >>$@

%.def:
	@echo Creating .DEF file: $@
	@echo NAME $(notdir $*) $(EXETYPE.$(notdir $*).exe) >$@
	@echo DESCRIPTION $(DQUOTE)$(DESCRIPTION.$(notdir $*).exe)$(DQUOTE) >>$@
	@echo STACKSIZE 2097152 >>$@

# Output file names
PYTHON_VER=	2.6
PYTHON_LIB=	python26
PYTHON.LIB=	$(PYTHON_LIB)_s$A
PYTHON.IMPLIB=	$(PYTHON_LIB)$A
ifeq ($(EXEOMF),yes)
  PYTHON.EXEIMP=	$(PYTHON.IMPLIB)
  LDMODE.EXE=		-Zomf
else
  PYTHON.EXEIMP=	$(PYTHON_LIB).a
  LDMODE.EXE = 
endif
PYTHON.DLL=	$(PYTHON_LIB).dll
PYTHON.DEF=	$(PYTHON_LIB).def
PYTHON.EXE=	python.exe
PYTHONPM.EXE=	pythonpm.exe
PGEN.EXE=	pgen.exe
LIBRARY=	$(PYTHON.LIB)
LD_LIBRARY=	$(PYTHON.IMPLIB)

# Additional executable parameters
EXETYPE.$(PYTHON.EXE)=		WINDOWCOMPAT
EXETYPE.$(PYTHONPM.EXE)=	WINDOWAPI
EXETYPE.$(PGEN.EXE)=		WINDOWCOMPAT
DESCRIPTION.$(PYTHON.EXE)=	Python object-oriented programming language interpreter for OS/2
DESCRIPTION.$(PYTHONPM.EXE)=	$(DESCRIPTION.$(PYTHON.EXE))
DESCRIPTION.$(PGEN.EXE)=	Python object-oriented programming language parser generator for OS/2

# Module descriptions
DESCRIPTION.zlib$(MODULE.EXT)=		Python Extension DLL for accessing the InfoZip compression library
DESCRIPTION.crypt$(MODULE.EXT)=		Python Extension DLL implementing the crypt$(BRO)$(BRC) function
DESCRIPTION._tkinter$(MODULE.EXT)=	Python Extension DLL for access to Tcl/Tk Environment
DESCRIPTION.readline$(MODULE.EXT)=	Python Extension DLL for access to GNU ReadLine library
DESCRIPTION._curses$(MODLIB.EXT)=	Python Extension DLL for access to ncurses library
DESCRIPTION.pyexpat$(MODULE.EXT)=	Python Extension DLL for access to expat library
DESCRIPTION.bz2$(MODULE.EXT)=		Python Extension DLL for accessing the bz2 compression library

# Source files
SRC.OS2EMX=	config.c dlfcn.c getpathp.c
SRC.MAIN=	$(addprefix $(TOP), \
		Modules/getbuildinfo.c \
		Modules/main.c)
SRC.MODULES=	$(addprefix $(TOP), \
		Modules/gcmodule.c \
		Modules/signalmodule.c \
		Modules/posixmodule.c \
		Modules/threadmodule.c \
		Modules/arraymodule.c \
		Modules/binascii.c \
		Modules/cmathmodule.c \
		Modules/_codecsmodule.c \
		Modules/collectionsmodule.c \
		Modules/cStringIO.c \
		Modules/_csv.c \
		Modules/datetimemodule.c \
		Modules/errnomodule.c \
		Modules/fcntlmodule.c \
		Modules/_functoolsmodule.c \
		Modules/_heapqmodule.c \
		Modules/imageop.c \
		Modules/itertoolsmodule.c \
		Modules/_localemodule.c \
		Modules/mathmodule.c \
		Modules/operator.c \
		Modules/_randommodule.c \
		Modules/sha256module.c \
		Modules/sha512module.c \
		Modules/_sre.c \
		Modules/_struct.c \
		Modules/symtablemodule.c \
		Modules/termios.c \
		Modules/timemodule.c \
		Modules/_weakref.c \
		Modules/xxsubtype.c \
		Modules/zipimport.c)
SRC.PARSE1=	$(addprefix $(TOP), \
		Parser/acceler.c \
		Parser/grammar1.c \
		Parser/listnode.c \
		Parser/node.c \
		Parser/parser.c \
		Parser/parsetok.c \
		Parser/bitset.c \
		Parser/metagrammar.c)
SRC.PARSE2=	$(addprefix $(TOP), \
		Parser/tokenizer.c \
		Parser/myreadline.c)
SRC.PARSER=	$(SRC.PARSE1) \
		$(SRC.PARSE2)
SRC.PYTHON=	$(addprefix $(TOP), \
		Python/Python-ast.c \
		Python/asdl.c \
		Python/ast.c \
		Python/bltinmodule.c \
		Python/exceptions.c \
		Python/ceval.c \
		Python/compile.c \
		Python/codecs.c \
		Python/errors.c \
		Python/frozen.c \
		Python/frozenmain.c \
		Python/future.c \
		Python/getargs.c \
		Python/getcompiler.c \
		Python/getcopyright.c \
		Python/getmtime.c \
		Python/getplatform.c \
		Python/getversion.c \
		Python/graminit.c \
		Python/import.c \
		Python/importdl.c \
		Python/marshal.c \
		Python/modsupport.c \
		Python/mysnprintf.c \
		Python/mystrtoul.c \
		Python/pyarena.c \
		Python/pyfpe.c \
		Python/pystate.c \
		Python/pystrtod.c \
		Python/pythonrun.c \
		Python/structmember.c \
		Python/symtable.c \
		Python/sysmodule.c \
		Python/traceback.c \
		Python/getopt.c \
		Python/dynload_shlib.c \
		Python/thread.c)
SRC.OBJECT=	$(addprefix $(TOP), \
		Objects/abstract.c \
		Objects/boolobject.c \
		Objects/cellobject.c \
		Objects/classobject.c \
		Objects/cobject.c \
		Objects/codeobject.c \
		Objects/complexobject.c \
		Objects/descrobject.c \
		Objects/dictobject.c \
		Objects/enumobject.c \
		Objects/fileobject.c \
		Objects/floatobject.c \
		Objects/frameobject.c \
		Objects/funcobject.c \
		Objects/genobject.c \
		Objects/iterobject.c \
		Objects/listobject.c \
		Objects/longobject.c \
		Objects/methodobject.c \
		Objects/moduleobject.c \
		Objects/object.c \
		Objects/obmalloc.c \
		Objects/rangeobject.c \
		Objects/setobject.c \
		Objects/sliceobject.c \
		Objects/stringobject.c \
		Objects/structseq.c \
		Objects/tupleobject.c \
		Objects/typeobject.c \
		Objects/unicodeobject.c \
		Objects/unicodectype.c \
		Objects/weakrefobject.c)

SRC.LIB=	$(SRC.OS2EMX) \
		$(SRC.MAIN) \
		$(SRC.PARSER) \
		$(SRC.OBJECT) \
		$(SRC.PYTHON) \
		$(SRC.MODULES)
OBJ.LIB=	$(addprefix $(OUT),$(notdir $(SRC.LIB:.c=$O)))

SRC.PGEN=	$(SRC.PARSE1) \
		$(addprefix $(TOP), \
		Objects/obmalloc.c) \
		$(addprefix $(TOP), \
		Python/mysnprintf.c) \
		$(addprefix $(TOP), \
		Parser/tokenizer_pgen.c \
		Parser/pgenmain.c \
		Parser/pgen.c \
		Parser/printgrammar.c \
		Parser/grammar.c \
		Parser/firstsets.c) \

OBJ.PGEN=	$(addprefix $(OUT),$(notdir $(SRC.PGEN:.c=$O)))

SRC.EXE=	$(TOP)Modules/python.c
SRC.PMEXE=	pythonpm.c

# Python modules to be dynamically loaded that:
#   1) have only single source file and require no extra libs
#   2) use the standard module naming convention
#      (the 'module' in ?????module.c is assumed)
# - these can be built with implicit rules
EASYEXTMODULES=	fpectl \
		fpetest \
		parser \
		pwd \
		select 

# Python modules to be dynamically loaded that need explicit build rules
#  (either multiple source files and/or non-standard module naming)
#  (NOTE: use shortened names for modules affected by 8 char name limit)
HARDEXTMODULES=	_socket \
		_testcap \
		unicoded

# Python modules that are used as libraries and therefore must use
# a .DLL extension
LIBEXTMODULES=

# Python external ($(MODULE.EXT)) modules - can be EASY or HARD
ifeq ($(HAVE_ZLIB),yes)
  HARDEXTMODULES+=	zlib
endif
ifeq ($(HAVE_UFC),yes)
  HARDEXTMODULES+=	crypt
endif
ifeq ($(HAVE_TCLTK),yes)
  HARDEXTMODULES+=	_tkinter
  CFLAGS+=		-DHAS_DIRENT -I/TclTk80/include
  TK_LIBS+=		-L/TclTk80/lib -ltcl80 -ltk80
endif
ifeq ($(HAVE_GREADLINE),yes)
  HARDEXTMODULES+=	readline
endif
ifeq ($(HAVE_NCURSES),yes)
  LIBEXTMODULES+=	_curses
  HARDEXTMODULES+=	_curses_
endif
ifeq ($(HAVE_GDBM),yes)
  HARDEXTMODULES+=	gdbm dbm
endif
ifeq ($(HAVE_BZ2),yes)
  HARDEXTMODULES+=	bz2
endif
ifeq ($(HAVE_OPENSSL),yes)
  HARDEXTMODULES+=	_ssl
endif

# Expat is now distributed with the Python source
HARDEXTMODULES+=	pyexpat
EXPAT.INC=	-I../../Modules/expat
EXPAT.DEF=	-DHAVE_EXPAT_H -DXML_NS=1 -DXML_DTD=1 -DXML_BYTE_ORDER=12 \
		-DXML_CONTENT_BYTES=1024 -DHAVE_MEMMOVE=1 -DHAVE_BCOPY=1
EXPAT.SRC=	$(addprefix ../../Modules/expat/, \
		xmlparse.c \
		xmlrole.c \
		xmltok.c)

# all the external modules
EXTERNDLLS=	$(addsuffix $(MODULE.EXT),$(patsubst %module,%,$(EASYEXTMODULES)))
EXTERNDLLS+=	$(addsuffix $(MODULE.EXT),$(patsubst %module,%,$(HARDEXTMODULES)))
EXTERNDLLS+=	$(addsuffix $(MODLIB.EXT),$(patsubst %module,%,$(LIBEXTMODULES)))

# Targets
all:  $(OUT) $(PYTHON.LIB) $(PYTHON.DEF) $(PYTHON.IMPLIB) $(PYTHON.DLL) \
	python_noncore

python_noncore:
	make PY_DEF= $(PYTHON.EXE) $(PYTHONPM.EXE) $(PGEN.EXE) $(EXTERNDLLS)

clean:
	rm -f $(OUT)*
	rm -f $(PYTHON.LIB) $(PYTHON.IMPLIB) $(PYTHON.EXEIMP) $(PYTHON.DLL) \
	  $(PYTHON.EXE) $(PYTHONPM.EXE) $(PGEN.EXE) *$(MODULE.EXT) *.dll
	find ../../Lib -name "*.py[co]" -exec rm {} ";"

lx:
	@echo Packing everything with lxLite...
	lxlite $(PYTHON.DLL) $(PYTHON.EXE) $(PYTHONPM.EXE) $(PGEN.EXE)

depend: $(OUTBASE)
	makedep -f $(OUTBASE)python.dep -o $(BUCK)O -p $(BUCK)\(OUT\) \
	  -r -c $(INCLUDE) $(SRC.LIB) $(SRC.PGEN)

$(OUT): $(OUTBASE)

$(OUT) $(OUTBASE):
	mkdir.exe $@

$(PYTHON.LIB): $(OBJ.LIB)
	rm.exe -f $@
	$(AR) $(ARFLAGS) $@ $^

# the Python core DLL .def file needs to have a number of non-static
# symbols that aren't part of the Python C API removed (commented out)
# from the DLL export list.
$(PYTHON.DEF): $(PYTHON.LIB)
	@echo Creating .DEF file: $@
	@echo LIBRARY $(PYTHON_LIB) INITINSTANCE TERMINSTANCE >$@
	@echo DESCRIPTION $(DQUOTE)Python $(PYTHON_VER) Core DLL$(DQUOTE) >>$@
	@echo PROTMODE >>$@
	@echo DATA MULTIPLE NONSHARED >>$@
	@echo EXPORTS >>$@
	$(EXPLIB) -u $(PYTHON.LIB) |\
	 sed -e "/^  .init.*/s/^ /; /" \
		-e "/^  .pcre_.*/s/^ /; /" \
		-e "/^  .array_methods/s/^ /; /" \
		-e "/^  .fast_save_leave/s/^ /; /" \
		-e "/^  .dlopen/s/^ /; /" \
		-e "/^  .dlsym/s/^ /; /" \
		-e "/^  .dlclose/s/^ /; /" \
		-e "/^  .dlerror/s/^ /; /" \
		-e "/^  ._Py_re_.*/s/^ /; /" \
		-e "/^  ._Py_MD5.*/s/^ /; /" >>$@

$(PYTHON.IMPLIB): $(PYTHON.DEF)
	$(IMPLIB) -o $@ $^

$(PYTHON.EXEIMP): $(PYTHON.DEF)
	$(IMPLIB) -o $@ $^

$(PYTHON.DLL): $(OUT)dllentry$O $(PYTHON.LIB) $(PYTHON.DEF)

# Explicit make targets for the .EXEs to be able to use LD to link
# (so that fork() will work if required)

$(PYTHON.EXE): $(SRC.EXE) $(PYTHON.EXEIMP) $(OUT)python.def
	$(CC) -Zmt $(LDMODE.EXE) -Zcrtdll -Wall $(INCLUDE) -L. -lgcc -o $@ $(SRC.EXE) $(PYTHON.EXEIMP) $(LIBS) $(OUT)python.def
	$(EXEOPT) -aq $(PYTHON.EXE) -h$(NFILES)

$(PYTHONPM.EXE): $(SRC.PMEXE) $(PYTHON.EXEIMP) $(OUT)pythonpm.def
	$(CC) -Zmt $(LDMODE.EXE) -Zcrtdll -Wall $(INCLUDE) -L. -lgcc -o $@ $(SRC.PMEXE) $(PYTHON.EXEIMP) $(LIBS) $(OUT)pythonpm.def
	$(EXEOPT) -aq $(PYTHONPM.EXE) -h$(NFILES)

$(PGEN.EXE): $(OBJ.PGEN) $(OUT)pgen.def

# Explicit building instructions for those external modules that require 
# awkward handling (due e.g. to non-std naming, or multiple source files)
# - standard modules

_socket$(MODULE.EXT): $(OUT)socketmodule$O $(OUT)_socket_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS)

# _testcapi needs to be renamed to be useful
_testcapi$(MODULE.EXT): $(OUT)_testcapimodule$O $(OUT)_testcapi_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS)

_testcap$(MODULE.EXT): _testcapi$(MODULE.EXT)
	cp $^ $@

# unicodedata needs to be renamed to be useful
unicodedata$(MODULE.EXT): $(OUT)unicodedata$O $(OUT)unicodedata_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS) $(MODULE_LIBS)

unicoded$(MODULE.EXT): unicodedata$(MODULE.EXT)
	cp $^ $@

crypt$(MODULE.EXT): $(OUT)cryptmodule$O $(OUT)crypt_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) -lufc $(LIBS)

# The _curses_panel module requires a couple of ncurses library entry
# points, which are best exposed as exports from the _curses module DLL
$(OUT)_curses_m.def:
	@echo Creating .DEF file: $@
	@echo LIBRARY $(notdir $*) INITINSTANCE TERMINSTANCE >$@
	@echo DESCRIPTION $(DQUOTE)$(DESCRIPTION.$(notdir $*)$(MODLIB.EXT))$(DQUOTE) >>$@
	@echo DATA MULTIPLE NONSHARED >>$@
	@echo EXPORTS >>$@
	@echo 	init_curses >>$@
	@echo 	wnoutrefresh >>$@
	@echo 	_nc_panelhook >>$@
	@echo 	is_linetouched >>$@
	@echo 	mvwin >>$@
	@echo 	stdscr >>$@
	@echo 	wtouchln >>$@

$(OUT)_curses_panel_m.def:
	@echo Creating .DEF file: $@
	@echo LIBRARY $(notdir $*) INITINSTANCE TERMINSTANCE >$@
	@echo DESCRIPTION $(DQUOTE)Python standard module $(notdir $*)$(DQUOTE) >>$@
	@echo DATA MULTIPLE NONSHARED >>$@
	@echo IMPORTS >>$@
	@echo 	_curses.wnoutrefresh >>$@
	@echo 	_curses._nc_panelhook >>$@
	@echo 	_curses.is_linetouched >>$@
	@echo 	_curses.mvwin >>$@
	@echo 	_curses.stdscr >>$@
	@echo 	_curses.wtouchln >>$@
	@echo EXPORTS >>$@
	@echo 	init_curses_panel >>$@

_curses$(MODLIB.EXT): $(OUT)_cursesmodule$O $(OUT)_curses_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS) -lncurses

# curses_panel needs to be renamed to be useful
_curses_panel$(MODULE.EXT): $(OUT)_curses_panel$O $(OUT)_curses_panel_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS) -lpanel

_curses_$(MODULE.EXT): _curses_panel$(MODULE.EXT)
	cp $^ $@

dbm$(MODULE.EXT): $(OUT)dbmmodule$O $(OUT)dbm_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS) -lgdbm

gdbm$(MODULE.EXT): $(OUT)gdbmmodule$O $(OUT)gdbm_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS) -lgdbm


# Expat is now distributed with Python, so use the included version
$(OUT)pyexpat$O:	../../Modules/pyexpat.c
	$(CC) $(CFLAGS) $(EXPAT.INC) -c -o $@ $^
$(OUT)xmlparse$O:	../../Modules/expat/xmlparse.c
	$(CC) $(CFLAGS) $(EXPAT.INC) $(EXPAT.DEF) -c -o $@ $^
$(OUT)xmlrole$O:	../../Modules/expat/xmlrole.c
	$(CC) $(CFLAGS) $(EXPAT.INC) $(EXPAT.DEF) -c -o $@ $^
$(OUT)xmltok$O:	../../Modules/expat/xmltok.c
	$(CC) $(CFLAGS) $(EXPAT.INC) $(EXPAT.DEF) -c -o $@ $^
pyexpat$(MODULE.EXT): $(OUT)pyexpat$O $(OUT)xmlparse$O $(OUT)xmlrole$O \
		$(OUT)xmltok$O $(OUT)pyexpat_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS)

readline$(MODULE.EXT): $(OUT)readline$O $(OUT)readline_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS) -lreadline -lncurses

#_tkinter$(MODULE.EXT): $(OUT)_tkinter$O $(OUT)tclNotify$O $(OUT)tkappinit$O
_tkinter$(MODULE.EXT): $(OUT)_tkinter$O $(OUT)tclNotify$O \
		$(OUT)_tkinter_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS) $(TK_LIBS)

zlib$(MODULE.EXT): $(OUT)zlibmodule$O $(OUT)zlib_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS) -lz

bz2$(MODULE.EXT): $(OUT)bz2module$O $(OUT)bz2_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS) -lbz2

_ssl$(MODULE.EXT): $(OUT)_ssl$O $(OUT)_ssl_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS) -lssl -lcrypto

# the test target
test:
	-find ../../Lib -name "*.py[co]" -exec rm {} ";"
	-./python -E -tt ../../lib/test/regrtest.py -l -u "network"
	./python -E -tt ../../lib/test/regrtest.py -l -u "network"

-include $(OUTBASE)python.dep
