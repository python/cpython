#####################==================----------------תתתתתתתתתתתתת
#
# Top-Level Makefile for Building Python 2.3 for OS/2 using GCC/EMX
# Originally written by Andrew Zabolotny, <bit@eltech.ru> for Python 1.5.2
# Modified by Andrew MacIntyre, <andymac@pcug.org.au> for Python 2.3
#
# This makefile was developed for use with [P]GCC/EMX compiler any
# version and GNU Make.
#
# The output of the build is a largish Python23.DLL containing the
# essential modules of Python and a small Python.exe program to start
# the interpreter. When embedding Python within another program, only
# Python23.DLL is needed. We also build python_s.a static library (which
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
#####################==================----------------תתתתתתתתתתתתת

# === Compilation mode: debug or release ===
MODE=		optimize
#MODE=		debug
# === Assert() enabled ===
#ASSERTIONS=no
ASSERTIONS=yes

# === Optional modules ===
# Do you have the InfoZip compression library installed?
ZLIB=		no
# Do you have the Ultra Fast Crypt (UFC) library installed?
UFC=		no
# Do you have the Tcl/Tk library installed?
TCLTK=		no
# Do you have the GNU multiprecision library installed?
GMPZ=		no
# Do you have the GNU readline library installed?
# NOTE: I'm using a modified version of Kai Uwe Rommel's port that 
#       - is compiled with multithreading enabled
#       - is linked statically
#       I have had no success trying to use a DLL version, even with
#       the multithreading switch.
GREADLINE=	no
# Do you have the BSD DB library (v1.85) as included in the EMXBSD package?
# NOTE: this library needs to be recompiled with a structure member
#       renamed to avoid problems with the multithreaded errno support
#       (there is a structure member called errno, used for shadowing the
#       real errno, which conflicts with the errno redefinition of -Zmt)
BSDDB=		no
# Do you have the ncurses library installed? EMX's BSD curses aren't enough! 
CURSES=		no
# Do you have the expat XML parsing library installed?
EXPAT=		no
# Do you have the GDBM library installed?
GDBM=		no

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

# adjust C compiler settings based on build options
ifeq ($(MODE),debug)
  CFLAGS+=	-g -O
  LDFLAGS+=	-g
else
  CFLAGS+=	-s -O2
  LDFLAGS+=	-s
endif
ifeq ($(ASSERTIONS),no)
  CFLAGS+=	-DNDEBUG
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
	@echo STACKSIZE 1048576 >>$@

# Output file names
PYTHON_VER=	2.3
PYTHON_LIB=	python23
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
DESCRIPTION.mpz$(MODULE.EXT)=		Python Extension DLL for access to GNU multi-precision library
DESCRIPTION.readline$(MODULE.EXT)=	Python Extension DLL for access to GNU ReadLine library
DESCRIPTION.bsddb$(MODULE.EXT)=		Python Extension DLL for access to BSD DB (v1.85) library
DESCRIPTION._curses$(MODULE.EXT)=	Python Extension DLL for access to ncurses library
DESCRIPTION.pyexpat$(MODULE.EXT)=	Python Extension DLL for access to expat library

# Source files
SRC.PGEN=	$(addprefix ../../Parser/, \
		pgenmain.c \
		pgen.c \
		printgrammar.c \
		grammar.c \
		bitset.c \
		firstsets.c)
OBJ.PGEN=	$(addprefix $(OUT),$(notdir $(SRC.PGEN:.c=$O)))

SRC.OS2EMX=	config.c dlfcn.c getpathp.c
SRC.MAIN=	$(addprefix $(TOP), \
		Modules/getbuildinfo.c \
		Modules/main.c)
SRC.MODULES=	$(addprefix $(TOP), \
		Modules/gcmodule.c \
		Modules/signalmodule.c \
		Modules/posixmodule.c \
		Modules/threadmodule.c \
		Modules/_sre.c)
SRC.PARSER=	$(addprefix $(TOP), \
		Parser/acceler.c \
		Parser/grammar1.c \
		Parser/listnode.c \
		Parser/node.c \
		Parser/parser.c \
		Parser/parsetok.c \
		Parser/tokenizer.c \
		Parser/bitset.c \
		Parser/metagrammar.c \
		Parser/myreadline.c)
SRC.PYTHON=	$(addprefix $(TOP), \
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
		Python/pyfpe.c \
		Python/pystate.c \
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
		Objects/bufferobject.c \
		Objects/cellobject.c \
		Objects/classobject.c \
		Objects/cobject.c \
		Objects/complexobject.c \
		Objects/descrobject.c \
		Objects/dictobject.c \
		Objects/fileobject.c \
		Objects/floatobject.c \
		Objects/frameobject.c \
		Objects/funcobject.c \
		Objects/intobject.c \
		Objects/iterobject.c \
		Objects/listobject.c \
		Objects/longobject.c \
		Objects/methodobject.c \
		Objects/moduleobject.c \
		Objects/object.c \
		Objects/obmalloc.c \
		Objects/rangeobject.c \
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

SRC.EXE=	$(TOP)Modules/python.c
SRC.PMEXE=	pythonpm.c

# Python modules to be dynamically loaded that:
#   1) have only single source file and require no extra libs
#   2) use the standard module naming convention
#      (the 'module' in ?????module.c is assumed)
# - these can be built with implicit rules
EASYEXTMODULES=	array \
		cmath \
		_codecs \
		dl \
		errno \
		fcntl \
		fpectl \
		fpetest \
		_locale \
		math \
		new \
		parser \
		pwd \
		rgbimg \
		rotor \
		select \
		sha \
		strop \
		struct \
		time \
		timing

# Python modules to be dynamically loaded that need explicit build rules
#  (either multiple source files and/or non-standard module naming)
#  (NOTE: use shortened names for modules affected by 8 char name limit)
HARDEXTMODULES=	binascii \
		cPickle \
		cStringI \
		_hotshot \
		imageop \
		md5 \
		operator \
		pcre \
		regex \
		_socket \
		termios \
		_testcap \
		unicoded \
		_weakref \
		xreadlin

# Python external ($(MODULE.EXT)) modules - can be EASY or HARD
ifeq ($(ZLIB),yes)
  HARDEXTMODULES+=	zlib
endif
ifeq ($(UFC),yes)
  HARDEXTMODULES+=	crypt
endif
ifeq ($(TCLTK),yes)
  HARDEXTMODULES+=	_tkinter
  CFLAGS+=		-DHAS_DIRENT -I/TclTk80/include
  TK_LIBS+=		-L/TclTk80/lib -ltcl80 -ltk80
endif
ifeq ($(GMPZ),yes)
  HARDEXTMODULES+=	mpz
endif
ifeq ($(GREADLINE),yes)
  HARDEXTMODULES+=	readline
endif
ifeq ($(BSDDB),yes)
  HARDEXTMODULES+=	bsddb
endif
ifeq ($(CURSES),yes)
  HARDEXTMODULES+=	_curses _curses_
endif
ifeq ($(EXPAT),yes)
  HARDEXTMODULES+=	pyexpat
endif
ifeq ($(GDBM),yes)
  HARDEXTMODULES+=	gdbm dbm
endif

EXTERNDLLS=	$(addsuffix $(MODULE.EXT),$(patsubst %module,%,$(EASYEXTMODULES)))
EXTERNDLLS+=	$(addsuffix $(MODULE.EXT),$(patsubst %module,%,$(HARDEXTMODULES)))

# Targets
all:  $(OUT) $(PYTHON.LIB) $(PYTHON.DEF) $(PYTHON.IMPLIB) $(PYTHON.DLL) \
  $(PYTHON.EXE) $(PYTHONPM.EXE) $(PGEN.EXE) $(EXTERNDLLS)

clean:
	rm -f $(OUT)*
	rm -f $(PYTHON.LIB) $(PYTHON.IMPLIB) $(PYTHON.EXEIMP) $(PYTHON.DLL) \
	  $(PYTHON.EXE) $(PYTHONPM.EXE) $(PGEN.EXE) *$(MODULE.EXT)

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

$(PYTHON.DEF): $(PYTHON.LIB)
	@echo Creating .DEF file: $@
	@echo LIBRARY $(PYTHON_LIB) INITINSTANCE TERMINSTANCE >$@
	@echo DESCRIPTION $(DQUOTE)Python $(PYTHON_VER) Core DLL$(DQUOTE) >>$@
	@echo PROTMODE >>$@
	@echo DATA MULTIPLE NONSHARED >>$@
	@echo EXPORTS >>$@
	$(EXPLIB) -u $(PYTHON.LIB) >>$@

$(PYTHON.IMPLIB): $(PYTHON.DEF)
	$(IMPLIB) -o $@ $^

$(PYTHON.EXEIMP): $(PYTHON.DEF)
	$(IMPLIB) -o $@ $^

$(PYTHON.DLL): $(OUT)dllentry$O $(PYTHON.LIB) $(PYTHON.DEF)

# Explicit make targets for the .EXEs to be able to use LD to link
# (so that fork() will work if required)

$(PYTHON.EXE): $(SRC.EXE) $(PYTHON.EXEIMP) $(OUT)python.def
	$(CC) -Zmt $(LDMODE.EXE) -Zcrtdll -Wall $(INCLUDE) -L. -lgcc -o $@ $(SRC.EXE) $(PYTHON.EXEIMP) $(LIBS) $(OUT)python.def

$(PYTHONPM.EXE): $(SRC.PMEXE) $(PYTHON.EXEIMP) $(OUT)pythonpm.def
	$(CC) -Zmt $(LDMODE.EXE) -Zcrtdll -Wall $(INCLUDE) -L. -lgcc -o $@ $(SRC.PMEXE) $(PYTHON.EXEIMP) $(LIBS) $(OUT)pythonpm.def

$(PGEN.EXE): $(OBJ.PGEN) $(PYTHON.LIB) $(OUT)pgen.def

# Explicit building instructions for those external modules that require 
# awkward handling (due e.g. to non-std naming, or multiple source files)
# - standard modules
binascii$(MODULE.EXT): $(OUT)binascii$O $(OUT)binascii_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS)

cPickle$(MODULE.EXT): $(OUT)cPickle$O $(OUT)cPickle_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS)

# cStringIO needs to be renamed to be useful (8 char DLL name limit)
cStringIO$(MODULE.EXT): $(OUT)cStringIO$O $(OUT)cStringIO_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS)

cStringI$(MODULE.EXT): cStringIO$(MODULE.EXT)
	cp $^ $@

_hotshot$(MODULE.EXT): $(OUT)_hotshot$O $(OUT)_hotshot_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS)

imageop$(MODULE.EXT): $(OUT)imageop$O $(OUT)imageop_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS)

md5$(MODULE.EXT): $(OUT)md5module$O $(OUT)md5c$O $(OUT)md5_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS)

operator$(MODULE.EXT): $(OUT)operator$O $(OUT)operator_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS)

pcre$(MODULE.EXT): $(OUT)pcremodule$O $(OUT)pypcre$O $(OUT)pcre_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS)

regex$(MODULE.EXT): $(OUT)regexmodule$O $(OUT)regexpr$O $(OUT)regex_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS)

_socket$(MODULE.EXT): $(OUT)socketmodule$O $(OUT)_socket_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS)

# _symtable needs to be renamed to be useful
_symtable$(MODULE.EXT): $(OUT)symtablemodule$O $(OUT)_symtable_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS)

_symtabl$(MODULE.EXT): _symtable$(MODULE.EXT)
	cp $^ $@

termios$(MODULE.EXT): $(OUT)termios$O $(OUT)termios_m.def $(PYTHON.IMPLIB)
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

_weakref$(MODULE.EXT): $(OUT)_weakref$O $(OUT)_weakref_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS)

# xreadlines needs to be renamed to be useful
xreadlines$(MODULE.EXT): $(OUT)xreadlinesmodule$O $(OUT)xreadlines_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS)

xreadlin$(MODULE.EXT): xreadlines$(MODULE.EXT)
	cp $^ $@

# - optional modules (requiring other software to be installed)
bsddb$(MODULE.EXT): $(OUT)bsddbmodule$O $(OUT)bsddb_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) -ldb $(LIBS)

crypt$(MODULE.EXT): $(OUT)cryptmodule$O $(OUT)crypt_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) -lufc $(LIBS)

# The _curses_panel module requires a couple of ncurses library entry
# points, which are best exposed as exports from the _curses module DLL
$(OUT)_curses_m.def:
	@echo Creating .DEF file: $@
	@echo LIBRARY $(notdir $*) INITINSTANCE TERMINSTANCE >$@
	@echo DESCRIPTION $(DQUOTE)$(DESCRIPTION.$(notdir $*)$(MODULE.EXT))$(DQUOTE) >>$@
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

_curses$(MODULE.EXT): $(OUT)_cursesmodule$O $(OUT)_curses_m.def $(PYTHON.IMPLIB)
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

mpz$(MODULE.EXT): $(OUT)mpzmodule$O $(OUT)mpz_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS) -lgmp

pyexpat$(MODULE.EXT): $(OUT)pyexpat$O $(OUT)pyexpat_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS) -lexpat

readline$(MODULE.EXT): $(OUT)readline$O $(OUT)readline_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS) -lreadline -lncurses

#_tkinter$(MODULE.EXT): $(OUT)_tkinter$O $(OUT)tclNotify$O $(OUT)tkappinit$O
_tkinter$(MODULE.EXT): $(OUT)_tkinter$O $(OUT)tclNotify$O \
    $(OUT)_tkinter_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS) $(TK_LIBS)

zlib$(MODULE.EXT): $(OUT)zlibmodule$O $(OUT)zlib_m.def $(PYTHON.IMPLIB)
	$(LD) $(LDFLAGS.DLL) -o $@ $(^^) $(L^) $(LIBS) -lz

# the test target
test:
	./python -tt ../../lib/test/regrtest.py -l -u "network"

-include $(OUTBASE)python.dep
