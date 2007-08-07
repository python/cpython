# RISC OS Python
LIBSROOT = $.AcornC_C++.Libraries

# You may need to change some of the following
OSLIB = $(LIBSROOT).OSLib
CLIB = $(LIBSROOT).clib
TCPIPLIB = $(LIBSROOT).TCPIPLibs
DLKLIB = $(LIBSROOT).dlk
ZLIB = $(LIBSROOT).zlib
BZLIB = $(LIBSROOT).bzlib
EXPAT = $(LIBSROOT).expat.lib

OBJSCAN = $(DLKLIB).objscan
MAKEDLK = $(DLKLIB).makedlk

# change from time to time (don't forget to change !Boot also)
TARGET=Python23
BUILD=40


#
# You shouldn't need to change anything below this line
#
DLKFLAG= -DDLK
DLKOBJS = $(DLKLIB).o.dlk_load @.o.linktab

HEADERS = @,$(TCPIPLIB),@.^.Include,@.^.Modules,@.^.Objects,@.^.Python,$(CLIB),$(OSLIB),$(DLKLIB)

CFLAGS = -c  $(DLKFLAG) -DRISCOS -DHAVE_CONFIG_H -wadP -throwback -APCS 3/32bit/fpe3

CC = cc $(CFLAGS) -j$(HEADERS)
CCEXPAT = cc $(CFLAGS) -j$(HEADERS),$(EXPAT) -DHAVE_EXPAT_H

LINK = link
LINKFLAGS = -aif #-NOUNUSED #-d
LOADLIBS = $(TCPIPLIB).o.unixlib $(TCPIPLIB).o.inetlib $(TCPIPLIB).o.socklib $(CLIB).o.Stubs $(OSLIB).o.OSLib32 $(DLKOBJS)

LIBFILE = libfile -c 


.c.o  :
       $(CC) -o $@ $*.c

MAIN_PYTHON =\
	@.^.Modules.o.python

LIB_PYTHON =\
	@.^.LibPython

# code for main Python binary
MODULES_STATIC =\
	@.^.Modules.o.main\
	@.Modules.o.config\
	@.^.Modules.o.getbuildinfo\
	@.Modules.o.getpath_riscos\
	@.Modules.o.riscosmodule\
	@.^.Modules.o.gcmodule


# dynamic Modules
MODULES_DYNAMIC =\
	@.^.Lib.array/pyd\
	@.^.Lib.audioop/pyd\
	@.^.Lib.binascii/pyd\
	@.^.Lib.bz2/pyd\
	@.^.Lib.cmath/pyd\
	@.^.Lib.cPickle/pyd\
	@.^.Lib.cStringIO/pyd\
	@.^.Lib.datetime/pyd\
	@.^.Lib.errno/pyd\
	@.^.Lib.imageop/pyd\
	@.^.Lib.itertools/pyd\
	@.^.Lib.math/pyd\
	@.^.Lib.md5/pyd\
	@.^.Lib.operator/pyd\
	@.^.Lib.parser/pyd\
	@.^.Lib.rgbimg/pyd\
	@.^.Lib.sha/pyd\
	@.^.Lib.signal/pyd\
	@.^.Lib.struct/pyd\
	@.^.Lib.time/pyd\
	@.^.Lib._locale/pyd\
	@.^.Lib.zlib/pyd\
	@.^.Lib.select/pyd\
	@.^.Lib._socket/pyd\
	@.^.Lib._codecs/pyd\
	@.^.Lib._weakref/pyd\
	@.^.Lib._testcapi/pyd\
	@.^.Lib.unicodedata/pyd\
	@.^.Lib.pyexpat/pyd\
	@.^.Lib.plat-riscos.drawf/pyd\
	@.^.Lib.plat-riscos.swi/pyd\
	@.^.Lib._sre/pyd\
	@.^.Lib.xxsubtype/pyd\
	@.^.Lib._symtable/pyd\
	@.^.Lib._random/pyd\
	@.^.Lib.zipimport/pyd\
	@.^.Lib._csv/pyd


OBJECTS_PYTHON =\
	@.^.Python.o.traceback\
	@.^.Python.o.sysmodule\
	@.^.Python.o.structmember\
	@.^.Python.o.strdup\
	@.^.Python.o.sigcheck\
	@.^.Python.o.pythonrun\
	@.^.Python.o.pystate\
	@.^.Python.o.pyfpe\
	@.^.Python.o.mystrtoul\
	@.^.Python.o.mysnprintf\
	@.^.Python.o.modsupport\
	@.^.Python.o.marshal\
	@.^.Python.o.importdl\
	@.^.Python.o.import\
	@.^.Python.o.graminit\
	@.^.Python.o.getversion\
	@.^.Python.o.getplatform\
	@.^.Python.o.getopt\
	@.^.Python.o.getcopyright\
	@.^.Python.o.getcompiler\
	@.^.Python.o.getargs\
	@.^.Python.o.frozenmain\
	@.^.Python.o.frozen\
	@.^.Python.o.errors\
	@.^.Python.o.compile\
	@.^.Python.o.ceval\
	@.^.Python.o.bltinmodule\
	@.^.Python.o.exceptions\
	@.^.Python.o.codecs\
	@.^.Python.o.symtable\
	@.^.Python.o.future


OBJECTS_RISCOS = \
	@.Python.o.dynload_riscos\
	@.Python.o.getcwd_riscos\
	@.Python.o.getmtime_riscos\
	@.o.unixstuff


OBJECTS_OBJECTS =\
	@.^.Objects.o.typeobject\
	@.^.Objects.o.tupleobject\
	@.^.Objects.o.stringobject\
	@.^.Objects.o.sliceobject\
	@.^.Objects.o.rangeobject\
	@.^.Objects.o.object\
	@.^.Objects.o.obmalloc\
	@.^.Objects.o.moduleobject\
	@.^.Objects.o.methodobject\
	@.^.Objects.o.longobject\
	@.^.Objects.o.listobject\
	@.^.Objects.o.boolobject\
	@.^.Objects.o.iterobject\
	@.^.Objects.o.funcobject\
	@.^.Objects.o.frameobject\
	@.^.Objects.o.floatobject\
	@.^.Objects.o.fileobject\
	@.^.Objects.o.enumobject\
	@.^.Objects.o.dictobject\
	@.^.Objects.o.complexobject\
	@.^.Objects.o.cobject\
	@.^.Objects.o.classobject\
	@.^.Objects.o.cellobject\
	@.^.Objects.o.bufferobject\
	@.^.Objects.o.abstract\
	@.^.Objects.o.unicodectype\
	@.^.Objects.o.unicodeobject\
	@.^.Objects.o.descrobject\
	@.^.Objects.o.weakrefobject\
	@.^.Objects.o.structseq


OBJECTS_PARSER =\
	@.^.Parser.o.tokenizer\
	@.^.Parser.o.printgrammar\
	@.^.Parser.o.parsetok\
	@.^.Parser.o.parser\
	@.^.Parser.o.node\
	@.^.Parser.o.myreadline\
	@.^.Parser.o.metagrammar\
	@.^.Parser.o.listnode\
	@.^.Parser.o.intrcheck\
	@.^.Parser.o.grammar1\
	@.^.Parser.o.grammar\
	@.^.Parser.o.firstsets\
	@.^.Parser.o.bitset\
	@.^.Parser.o.acceler

SUPPORT_FILES = @.^.!Boot @.^.!Run @.^.!Sprites @.^.!Sprites22 @.^.AddToPath

OBJECTS = $(OBJECTS_PYTHON) $(OBJECTS_PARSER) $(MODULES_STATIC) $(OBJECTS_OBJECTS) $(OBJECTS_RISCOS)


all: @.^.$(TARGET) $(MODULES_DYNAMIC) $(SUPPORT_FILES)

@.^.Modules.o.getbuildinfo: @.^.Modules.c.getbuildinfo
       $(CC) -DBUILD=$(BUILD) -o @.^.Modules.o.getbuildinfo @.^.Modules.c.getbuildinfo


$(LIB_PYTHON):	$(OBJECTS)
	$(LIBFILE) $(LIB_PYTHON) $(OBJECTS)

@.^.$(TARGET):  o.linktab $(MAIN_PYTHON) $(LIB_PYTHON)
	$(LINK) -o @.^.$(TARGET) $(MAIN_PYTHON) @.^.LibPython $(LOADLIBS)


#########################################################################
# RISC OS support files
#
@.^.!Boot: support.!Boot
	copy support.!Boot @.^.!Boot ~C~VF
	settype @.^.!Boot feb
@.^.!Run:  support.!Run
	copy support.!Run @.^.!Run ~C~VF
	settype @.^.!Run feb
@.^.!Sprites: support.!Sprites
	copy support.!Sprites @.^.!Sprites ~C~VF
	settype @.^.!Sprites ff9
@.^.!Sprites22: support.!Sprites22
	copy support.!Sprites22 @.^.!Sprites22 ~C~VF
	settype @.^.!Sprites22 ff9
@.^.AddToPath: support.AddToPath
	copy support.AddToPath @.^.AddToPath ~C~VF
	settype @.^.AddToPath ffc


#########################################################################
# Dynamic Modules
#
@.^.Lib.array/pyd: @.^.Modules.o.arraymodule s.linktab
	$(MAKEDLK) -d @.^.Lib.array/pyd -s s.linktab -o @.^.Modules.o.arraymodule -e initarray

@.^.Lib.audioop/pyd: @.^.Modules.o.audioop # s.linktab
	$(MAKEDLK) -d @.^.Lib.audioop/pyd -s s.linktab -o @.^.Modules.o.audioop -e initaudioop

@.^.Lib.binascii/pyd: @.^.Modules.o.binascii s.linktab
	$(MAKEDLK) -d @.^.Lib.binascii/pyd -s s.linktab -o @.^.Modules.o.binascii -e initbinascii

@.^.Lib.cmath/pyd: @.^.Modules.o.cmathmodule s.linktab
	$(MAKEDLK) -d @.^.Lib.cmath/pyd -s s.linktab -o @.^.Modules.o.cmathmodule -e initcmath

@.^.Lib.cPickle/pyd: @.^.Modules.o.cPickle s.linktab
	$(MAKEDLK) -d @.^.Lib.cPickle/pyd -s s.linktab -o @.^.Modules.o.cPickle -e initcPickle

@.^.Lib.cStringIO/pyd: @.^.Modules.o.cStringIO s.linktab
	$(MAKEDLK) -d @.^.Lib.cStringIO/pyd -s s.linktab -o @.^.Modules.o.cStringIO -e initcStringIO

@.^.Lib._csv/pyd: @.^.Modules.o._csv s.linktab
	$(MAKEDLK) -d @.^.Lib._csv/pyd -s s.linktab -o @.^.Modules.o._csv -e init_csv

@.^.Lib.plat-riscos.drawf/pyd: Modules.o.drawfmodule #s.linktab
	$(LINK) -aof -o Modules.o.drawflink Modules.o.drawfmodule $(OSLIB).o.OSLIB32
	$(MAKEDLK) -d @.^.Lib.plat-riscos.drawf/pyd -s s.linktab -o Modules.o.drawflink -e initdrawf

@.^.Lib.datetime/pyd: @.^.Modules.o.datetimemodule s.linktab
	$(MAKEDLK) -d @.^.Lib.datetime/pyd -s s.linktab -o @.^.Modules.o.datetimemodule -e initdatetime

@.^.Lib.errno/pyd: @.^.Modules.o.errnomodule #s.linktab
	$(MAKEDLK) -d @.^.Lib.errno/pyd -s s.linktab -o @.^.Modules.o.errnomodule -e initerrno

@.^.Lib.imageop/pyd: @.^.Modules.o.imageop s.linktab
	$(MAKEDLK) -d @.^.Lib.imageop/pyd -s s.linktab -o @.^.Modules.o.imageop -e initimageop

@.^.Lib.itertools/pyd: @.^.Modules.o.itertoolsmodule s.linktab
	$(MAKEDLK) -d @.^.Lib.itertools/pyd -s s.linktab -o @.^.Modules.o.itertoolsmodule -e inititertools

@.^.Lib.math/pyd: @.^.Modules.o.mathmodule s.linktab
	$(MAKEDLK) -d @.^.Lib.math/pyd -s s.linktab -o @.^.Modules.o.mathmodule -e initmath

@.^.Lib.mmap/pyd: @.^.Modules.o.mmapmodule s.linktab
	$(MAKEDLK) -d @.^.Lib.mmap/pyd -s s.linktab -o @.^.Modules.o.mmapmodule -e initmmap

@.^.Lib.md5/pyd: @.^.Modules.o.md5c @.^.Modules.o.md5module s.linktab
	$(LINK) -aof -o @.^.Modules.o.md5link @.^.Modules.o.md5c @.^.Modules.o.md5module
	$(MAKEDLK) -d @.^.Lib.md5/pyd -s s.linktab -o @.^.Modules.o.md5link -e initmd5

@.^.Lib.operator/pyd: @.^.Modules.o.operator s.linktab
	$(MAKEDLK) -d @.^.Lib.operator/pyd -s s.linktab -o @.^.Modules.o.operator -e initoperator

@.^.Lib.parser/pyd: @.^.Modules.o.parsermodule s.linktab
	$(MAKEDLK) -d @.^.Lib.parser/pyd -s s.linktab -o @.^.Modules.o.parsermodule -e initparser

@.^.Lib.rgbimg/pyd: @.^.Modules.o.rgbimgmodule s.linktab
	$(MAKEDLK) -d @.^.Lib.rgbimg/pyd -s s.linktab -o @.^.Modules.o.rgbimgmodule -e initrgbimg

@.^.Lib.sha/pyd: @.^.Modules.o.shamodule s.linktab
	$(MAKEDLK) -d @.^.Lib.sha/pyd -s s.linktab -o @.^.Modules.o.shamodule -e initsha

@.^.Lib.signal/pyd: @.^.Modules.o.signalmodule s.linktab
	$(MAKEDLK) -d @.^.Lib.signal/pyd -s s.linktab -o @.^.Modules.o.signalmodule -e initsignal

@.^.Lib.struct/pyd: @.^.Modules.o.structmodule s.linktab
	$(MAKEDLK) -d @.^.Lib.struct/pyd -s s.linktab -o @.^.Modules.o.structmodule -e initstruct

@.^.Lib.plat-riscos.swi/pyd: Modules.o.swimodule s.linktab
	$(LINK) -aof -o Modules.o.swilink Modules.o.swimodule $(OSLIB).o.OSLIB32
	$(MAKEDLK) -d @.^.Lib.plat-riscos.swi/pyd -s s.linktab -o Modules.o.swilink -e initswi

@.^.Lib._locale/pyd: @.^.Modules.o._localemodule s.linktab
	$(MAKEDLK) -d @.^.Lib._locale/pyd -s s.linktab -o @.^.Modules.o._localemodule -e init_locale

@.^.Lib._sre/pyd: @.^.Modules.o._sre s.linktab
	$(MAKEDLK) -d @.^.Lib._sre/pyd -s s.linktab -o @.^.Modules.o._sre -e init_sre

@.^.Lib._codecs/pyd: @.^.Modules.o._codecsmodule s.linktab
	$(MAKEDLK) -d @.^.Lib._codecs/pyd -s s.linktab -o @.^.Modules.o._codecsmodule -e init_codecs

@.^.Lib._weakref/pyd: @.^.Modules.o._weakref s.linktab
	$(MAKEDLK) -d @.^.Lib._weakref/pyd -s s.linktab -o @.^.Modules.o._weakref -e init_weakref

@.^.Lib._testcapi/pyd: @.^.Modules.o._testcapimodule s.linktab
	$(MAKEDLK) -d @.^.Lib._testcapi/pyd -s s.linktab -o @.^.Modules.o._testcapimodule -e init_testcapi

@.^.Lib.unicodedata/pyd: @.^.Modules.o.unicodedata s.linktab
	$(MAKEDLK) -d @.^.Lib.unicodedata/pyd -s s.linktab -o @.^.Modules.o.unicodedata -e initunicodedata

@.^.Lib.xxsubtype/pyd: @.^.Modules.o.xxsubtype s.linktab
	$(MAKEDLK) -d @.^.Lib.xxsubtype/pyd -s s.linktab -o @.^.Modules.o.xxsubtype -e initxxsubtype

@.^.Lib._random/pyd: @.^.Modules.o._randommodule s.linktab
	$(MAKEDLK) -d @.^.Lib._random/pyd -s s.linktab -o @.^.Modules.o._randommodule -e init_random

@.^.Lib._symtable/pyd: @.^.Modules.o.symtablemodule s.linktab
	$(MAKEDLK) -d @.^.Lib._symtable/pyd -s s.linktab -o @.^.Modules.o.symtablemodule -e init_symtable


############################################################################
# Dynamic Modules with other dependencies
#
@.^.Lib.select/pyd:  @.^.Modules.o.selectmodule s.linktab
	$(LINK) -aof -o @.^.Modules.o.selectlink @.^.Modules.o.selectmodule $(TCPIPLIB).o.socklib5
	$(MAKEDLK) -d @.^.Lib.select/pyd -s s.linktab -o @.^.Modules.o.selectlink -e initselect

@.^.Modules.o.selectmodule: @.^.Modules.c.selectmodule
	$(CC) -I$(TCPIPLIB).include -o $@ @.^.Modules.c.selectmodule

@.^.Lib._socket/pyd: @.^.Modules.o.socketmodule s.linktab 
	$(LINK) -aof -o @.^.Modules.o._socketlink @.^.Modules.o.socketmodule $(TCPIPLIB).o.inetlib $(TCPIPLIB).o.unixlib $(TCPIPLIB).o.socklib5
	$(MAKEDLK) -d @.^.Lib._socket/pyd -s s.linktab -o @.^.Modules.o._socketlink -e init_socket

@.^.Modules.o.socketmodule: @.^.Modules.c.socketmodule
	$(CC) -I$(TCPIPLIB).include -o $@ @.^.Modules.c.socketmodule


@.^.Lib.zlib/pyd: @.^.Modules.o.zlibmodule s.linktab
	$(LINK) -aof -o @.^.Modules.o.zliblink @.^.Modules.o.zlibmodule $(ZLIB).zlib
	$(MAKEDLK) -d @.^.Lib.zlib/pyd -s s.linktab -o @.^.Modules.o.zliblink -e initzlib

@.^.Modules.o.zlibmodule: @.^.Modules.c.zlibmodule
	$(CC) -I$(ZLIB) -o $@ @.^.Modules.c.zlibmodule

@.^.Lib.bz2/pyd: @.^.Modules.o.bz2module s.linktab
	$(LINK) -aof -o @.^.Modules.o.bz2link @.^.Modules.o.bz2module $(BZLIB).bzlib
	$(MAKEDLK) -d @.^.Lib.bz2/pyd -s s.linktab -o @.^.Modules.o.bz2link -e initbz2

@.^.Modules.o.bz2module: @.^.Modules.c.bz2module
	$(CC) -I$(BZLIB) -o $@ @.^.Modules.c.bz2module

@.^.Lib.time/pyd: @.^.Modules.o.timemodule s.linktab @.o.sleep
	$(LINK) -aof -o @.^.Modules.o.timelink @.^.Modules.o.timemodule @.o.sleep $(OSLIB).o.OSLib32
	$(MAKEDLK) -d @.^.Lib.time/pyd -s s.linktab -o @.^.Modules.o.timelink -e inittime

#@.^.Lib.time/pyd: @.Modules.o.timemodule s.linktab
#	$(LINK) -aof -o @.Modules.o.timelink @.Modules.o.timemodule $(OSLIB).o.OSLib32
#	$(MAKEDLK) -d @.^.Lib.time/pyd -s s.linktab -o @.Modules.o.timelink -e inittime


@.^.Lib.pyexpat/pyd: @.^.Modules.o.pyexpat s.linktab
	$(LINK) -aof -o @.^.Modules.o.pyexpatlink @.^.Modules.o.pyexpat $(EXPAT).expat_lib
	$(MAKEDLK) -d @.^.Lib.pyexpat/pyd -s s.linktab -o @.^.Modules.o.pyexpatlink -e initpyexpat

@.^.Modules.o.pyexpat: @.^.Modules.c.pyexpat
	$(CCEXPAT) -o $@ @.^.Modules.c.pyexpat

@.^.Lib.zipimport/pyd: @.^.Modules.o.zipimport s.linktab
	$(LINK) -aof -o @.^.Modules.o.zipimportlink @.^.Modules.o.zipimport o.unixstuff $(OSLIB).o.OSLib32
	$(MAKEDLK) -d @.^.Lib.zipimport/pyd -s s.linktab -o @.^.Modules.o.zipimportlink -e initzipimport


##########################################################################
# dynamic linking symbol table
#
o.linktab: s.linktab
	ObjAsm -APCS 3/32bit s.linktab o.linktab

s.linktab: $(OBJECTS)
	$(OBJSCAN) -s s.linktab -o $(OBJECTS) $(clib).o.stubs

##########################################################################
# special targets
#
libclean:
	-wipe @.^.Lib.*/pyc ~C~V
	-wipe @.^.Lib.*/pyo ~C~V
	-wipe @.^.Lib.plat-riscos.*/pyc ~C~V
	-wipe @.^.Lib.plat-riscos.*/pyo ~C~V
	-wipe @.^.Lib.test.*/pyc ~C~V
	-wipe @.^.Lib.test.*/pyo ~C~V
	-wipe @.^.Lib.encodings.*/pyc ~C~V
	-wipe @.^.Lib.encodings.*/pyo ~C~V
	-wipe @.^.Lib.curses.*/pyc ~C~V
	-wipe @.^.Lib.curses.*/pyo ~C~V
	-wipe @.^.Lib.xml.*/pyc ~C~V
	-wipe @.^.Lib.xml.*/pyo ~C~V
	-wipe @.^.Lib.xml.sax.*/pyc ~C~V
	-wipe @.^.Lib.xml.sax.*/pyo ~C~V
	-wipe @.^.Lib.xml.dom.*/pyc ~C~V
	-wipe @.^.Lib.xml.dom.*/pyo ~C~V
	-wipe @.^.Lib.xml.parsers.*/pyc ~C~V
	-wipe @.^.Lib.xml.parsers.*/pyo ~C~V

clean: libclean
	-wipe @.^.Modules.o.* ~C~V
	-wipe @.^.Objects.o.* ~C~V
	-wipe @.^.Parser.o.* ~C~V
	-wipe @.^.Python.o.* ~C~V
	-wipe @.Modules.o.* ~C~V
	-wipe @.Python.o.* ~C~V
	-wipe @.o.* ~C ~V

rebuild: clean clean-support
	remove @.^.$(TARGET)
	-wipe @.^.Lib.*/pyd ~C ~V
	-wipe @.^.Lib.plat-riscos.*/pyd ~C~V
	remove s.linktab
	remove @.^.LibPython

clean-support:
	-wipe @.^.!* ~C~V
	remove @.^.AddToPath

cdirs:
	cdir @.Modules.o
	cdir @.Python.o
	cdir @.s
	cdir @.o
	cdir @.^.Python.o
	cdir @.^.Parser.o
	cdir @.^.Objects.o
	cdir @.^.Modules.o


# Dynamic dependencies:
