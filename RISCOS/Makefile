# RISC OS Python
LIBSROOT = $.AcornC_C++.Libraries

# You may need to change some of the following
OSLIB = $(LIBSROOT).OSLib
CLIB = $(LIBSROOT).clib
SOCKLIB = $(LIBSROOT).netlib
DLKLIB = $(LIBSROOT).dlk
ZLIB = $(LIBSROOT).zlib
EXPAT = $(LIBSROOT).expat.lib

OBJSCAN = $(DLKLIB).objscan
MAKEDLK = $(DLKLIB).makedlk

# change from time to time
TARGET=Python21
BUILD=10


#
# You shouldn't need to change anything below this line
#
OSLIBS = OSLib:Computer,OSLib:Core,OSLib:User

DLKFLAG= -DDLK
DLKOBJS = $(DLKLIB).o.dlk_load o.linktab

HEADERS = @,@.^.Include,@.^.Modules,@.^.Objects,@.^.Python,$(CLIB),$(OSLIBS),$(DLKLIB)

CC = cc -c -j$(HEADERS) $(DLKFLAG) -DRISCOS -DHAVE_CONFIG_H -wad -throwback
#-depend !Depend
CCEXPAT = cc -c -j$(HEADERS),$(EXPAT) $(DLKFLAG) -DHAVE_EXPAT_H -DRISCOS -DHAVE_CONFIG_H -wad -throwback

LINK = link
LINKFLAGS = -aif -NOUNUSED #-d
LOADLIBS = $(CLIB).o.Stubs $(OSLIB).o.OSLib $(DLKOBJS)


.c.o  :
       $(CC) -o $@ $*.c


# code for main Python binary
MODULES_STATIC = \
	@.^.Modules.o.python \
	@.^.Modules.o.main \
	Modules.o.config \
	@.^.Modules.o.getbuildinfo \
	Modules.o.getpath_riscos \
	Modules.o.riscosmodule \
	@.^.Modules.o._sre


# dynamic Modules
MODULES_DYNAMIC = \
	@.^.Lib.array/pyd \
	@.^.Lib.audioop/pyd \
	@.^.Lib.binascii/pyd \
	@.^.Lib.cmath/pyd \
	@.^.Lib.cPickle/pyd \
	@.^.Lib.cStringIO/pyd \
	@.^.Lib.errno/pyd \
	@.^.Lib.imageop/pyd \
	@.^.Lib.math/pyd \
	@.^.Lib.md5/pyd \
	@.^.Lib.new/pyd \
	@.^.Lib.operator/pyd \
	@.^.Lib.parser/pyd \
	@.^.Lib.pcre/pyd \
	@.^.Lib.regex/pyd \
	@.^.Lib.rgbimg/pyd \
	@.^.Lib.rotor/pyd \
	@.^.Lib.sha/pyd \
	@.^.Lib.signal/pyd \
	@.^.Lib.struct/pyd \
	@.^.Lib.time/pyd \
	@.^.Lib._locale/pyd \
	@.^.Lib.zlib/pyd \
	@.^.Lib.select/pyd \
	@.^.Lib._socket/pyd \
	@.^.Lib._codecs/pyd \
	@.^.Lib._weakref/pyd \
	@.^.Lib._testcapi/pyd \
	@.^.Lib.unicodedata/pyd \
	@.^.Lib.xreadlines/pyd \
	@.^.Lib.pyexpat/pyd \
	@.^.Lib.plat-riscos.drawf/pyd \
	@.^.Lib.plat-riscos.swi/pyd

	# @.^.Lib.soundex/pyd \
	# leave  strop out? It's no longer in use for string operations
	# @.^.Lib.mmap/pyd would it make sense? I read about a mmap
	# implementation for RISC OS recently in usenet news.

	#@.^.Lib.strop/pyd \
	#@.^.Lib._sre/pyd \


OBJECTS_PYTHON = \
	@.^.Python.o.traceback \
	@.^.Python.o.sysmodule \
	@.^.Python.o.structmember \
	@.^.Python.o.strdup \
	@.^.Python.o.sigcheck \
	@.^.Python.o.pythonrun \
	@.^.Python.o.pystate \
	@.^.Python.o.pyfpe \
	@.^.Python.o.mystrtoul \
	@.^.Python.o.modsupport \
	@.^.Python.o.marshal \
	@.^.Python.o.importdl \
	@.^.Python.o.import \
	@.^.Python.o.graminit \
	@.^.Python.o.getversion \
	@.^.Python.o.getplatform \
	@.^.Python.o.getopt \
	@.^.Python.o.getcopyright \
	@.^.Python.o.getcompiler \
	@.^.Python.o.getargs \
	@.^.Python.o.frozenmain \
	@.^.Python.o.frozen \
	@.^.Python.o.errors \
	@.^.Python.o.compile \
	@.^.Python.o.ceval \
	@.^.Python.o.bltinmodule \
	@.^.Python.o.exceptions \
	@.^.Python.o.hypot \
	@.^.Python.o.codecs \
	@.^.Python.o.symtable
# @.^.Python.o.atof @.^.Python.o.strerror


OBJECTS_RISCOS =  \
	@.Python.o.dynload_riscos \
	@.Python.o.getcwd_riscos \
	@.Python.o.getmtime_riscos \
	@.o.unixstuff


OBJECTS_OBJECTS = \
	@.^.Objects.o.typeobject \
	@.^.Objects.o.tupleobject \
	@.^.Objects.o.stringobject \
	@.^.Objects.o.sliceobject \
	@.^.Objects.o.rangeobject \
	@.^.Objects.o.object \
	@.^.Objects.o.moduleobject \
	@.^.Objects.o.methodobject \
	@.^.Objects.o.longobject \
	@.^.Objects.o.listobject \
	@.^.Objects.o.intobject \
	@.^.Objects.o.funcobject \
	@.^.Objects.o.frameobject \
	@.^.Objects.o.floatobject \
	@.^.Objects.o.fileobject \
	@.^.Objects.o.dictobject \
	@.^.Objects.o.complexobject \
	@.^.Objects.o.cobject \
	@.^.Objects.o.classobject \
	@.^.Objects.o.cellobject \
	@.^.Objects.o.bufferobject \
	@.^.Objects.o.abstract \
	@.^.Objects.o.unicodectype \
	@.^.Objects.o.unicodeobject


OBJECTS_PARSER = \
	@.^.Parser.o.tokenizer \
	@.^.Parser.o.printgrammar \
	@.^.Parser.o.parsetok \
	@.^.Parser.o.parser \
	@.^.Parser.o.node \
	@.^.Parser.o.myreadline \
	@.^.Parser.o.metagrammar \
	@.^.Parser.o.listnode \
	@.^.Parser.o.intrcheck \
	@.^.Parser.o.grammar1 \
	@.^.Parser.o.grammar \
	@.^.Parser.o.firstsets \
	@.^.Parser.o.bitset \
	@.^.Parser.o.acceler

SUPPORT_FILES = @.^.!Boot @.^.!Run @.^.!Sprites @.^.!Sprites22 @.^.AddToPath

OBJECTS = $(OBJECTS_PYTHON) $(OBJECTS_PARSER) $(MODULES_STATIC) $(OBJECTS_OBJECTS) $(OBJECTS_RISCOS)


all: @.^.$(TARGET) $(MODULES_DYNAMIC) $(SUPPORT_FILES)

@.^.Modules.o.getbuildinfo: @.^.Modules.c.getbuildinfo
       $(CC) -DBUILD=$(BUILD) -o @.^.Modules.o.getbuildinfo @.^.Modules.c.getbuildinfo


@.^.$(TARGET):  $(OBJECTS) o.linktab
	$(LINK) -o @.^.$(TARGET) $(OBJECTS) $(LOADLIBS)


#########################################################################
# Support files
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

@.^.Lib.plat-riscos.drawf/pyd: Modules.o.drawfmodule #s.linktab
	$(LINK) -aof -o Modules.o.drawflink Modules.o.drawfmodule $(OSLIB).o.OSLIB
	$(MAKEDLK) -d @.^.Lib.plat-riscos.drawf/pyd -s s.linktab -o Modules.o.drawflink -e initdrawf

@.^.Lib.errno/pyd: @.^.Modules.o.errnomodule #s.linktab
	$(MAKEDLK) -d @.^.Lib.errno/pyd -s s.linktab -o @.^.Modules.o.errnomodule -e initerrno

@.^.Lib.imageop/pyd: @.^.Modules.o.imageop s.linktab
	$(MAKEDLK) -d @.^.Lib.imageop/pyd -s s.linktab -o @.^.Modules.o.imageop -e initimageop

@.^.Lib.math/pyd: @.^.Modules.o.mathmodule s.linktab
	$(MAKEDLK) -d @.^.Lib.math/pyd -s s.linktab -o @.^.Modules.o.mathmodule -e initmath

@.^.Lib.mmap/pyd: @.^.Modules.o.mmapmodule s.linktab
	$(MAKEDLK) -d @.^.Lib.mmap/pyd -s s.linktab -o @.^.Modules.o.mmapmodule -e initmmap

@.^.Lib.md5/pyd: @.^.Modules.o.md5c @.^.Modules.o.md5module s.linktab
	$(LINK) -aof -o @.^.Modules.o.md5link @.^.Modules.o.md5c @.^.Modules.o.md5module
	$(MAKEDLK) -d @.^.Lib.md5/pyd -s s.linktab -o @.^.Modules.o.md5link -e initmd5

@.^.Lib.new/pyd: @.^.Modules.o.newmodule s.linktab
	$(MAKEDLK) -d @.^.Lib.new/pyd -s s.linktab -o @.^.Modules.o.newmodule -e initnew

@.^.Lib.operator/pyd: @.^.Modules.o.operator s.linktab
	$(MAKEDLK) -d @.^.Lib.operator/pyd -s s.linktab -o @.^.Modules.o.operator -e initoperator

@.^.Lib.parser/pyd: @.^.Modules.o.parsermodule s.linktab
	$(MAKEDLK) -d @.^.Lib.parser/pyd -s s.linktab -o @.^.Modules.o.parsermodule -e initparser

@.^.Lib.pcre/pyd: @.^.Modules.o.pcremodule @.^.Modules.o.pypcre s.linktab
	$(LINK) -aof -o @.^.Modules.o.pcrelink @.^.Modules.o.pcremodule @.^.Modules.o.pypcre
	$(MAKEDLK) -d @.^.Lib.pcre/pyd -s s.linktab -o @.^.Modules.o.pcrelink -e initpcre

@.^.Lib.regex/pyd: @.^.Modules.o.regexmodule @.^.Modules.o.regexpr s.linktab
	$(LINK) -aof -o @.^.Modules.o.regexlink @.^.Modules.o.regexmodule @.^.Modules.o.regexpr
	$(MAKEDLK) -d @.^.Lib.regex/pyd -s s.linktab -o @.^.Modules.o.regexlink -e initregex

@.^.Lib.rgbimg/pyd: @.^.Modules.o.rgbimgmodule s.linktab
	$(MAKEDLK) -d @.^.Lib.rgbimg/pyd -s s.linktab -o @.^.Modules.o.rgbimgmodule -e initrgbimg

@.^.Lib.rotor/pyd: @.^.Modules.o.rotormodule s.linktab
	$(MAKEDLK) -d @.^.Lib.rotor/pyd -s s.linktab -o @.^.Modules.o.rotormodule -e initrotor

@.^.Lib.sha/pyd: @.^.Modules.o.shamodule s.linktab
	$(MAKEDLK) -d @.^.Lib.sha/pyd -s s.linktab -o @.^.Modules.o.shamodule -e initsha

@.^.Lib.signal/pyd: @.^.Modules.o.signalmodule s.linktab
	$(MAKEDLK) -d @.^.Lib.signal/pyd -s s.linktab -o @.^.Modules.o.signalmodule -e initsignal

#@.^.Lib.soundex/pyd: @.^.Modules.o.soundex s.linktab
#	$(MAKEDLK) -d @.^.Lib.soundex/pyd -s s.linktab -o @.^.Modules.o.soundex -e initsoundex

@.^.Lib.strop/pyd: @.^.Modules.o.stropmodule s.linktab
	$(MAKEDLK) -d @.^.Lib.strop/pyd -s s.linktab -o @.^.Modules.o.stropmodule -e initstrop

@.^.Lib.struct/pyd: @.^.Modules.o.structmodule s.linktab
	$(MAKEDLK) -d @.^.Lib.struct/pyd -s s.linktab -o @.^.Modules.o.structmodule -e initstruct

@.^.Lib.plat-riscos.swi/pyd: Modules.o.swimodule s.linktab
	$(LINK) -aof -o Modules.o.swilink Modules.o.swimodule $(OSLIB).o.OSLIB
	$(MAKEDLK) -d @.^.Lib.plat-riscos.swi/pyd -s s.linktab -o Modules.o.swilink -e initswi

@.^.Lib.time/pyd: @.^.Modules.o.timemodule s.linktab
	$(MAKEDLK) -d @.^.Lib.time/pyd -s s.linktab -o @.^.Modules.o.timemodule -e inittime

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

@.^.Lib.xreadlines/pyd: @.^.Modules.o.xreadlinesmodule s.linktab
	$(MAKEDLK) -d @.^.Lib.xreadlines/pyd -s s.linktab -o @.^.Modules.o.xreadlinesmodule -e initxreadlines


##@.^.Lib.mmap/pyd: @.^.Modules.o.mmapmodule s.linktab
##	$(MAKEDLK) -d @.^.Lib.mmap/pyd -s s.linktab -o @.^.Modules.o.mmapmodule -e initmmap



############################################################################
# Dynamic Modules with other dependencies
#
@.^.Lib.select/pyd:  @.^.Modules.o.selectmodule s.linktab
	$(LINK) -aof -o @.^.Modules.o.selectlink @.^.Modules.o.selectmodule $(SOCKLIB).o.socklib
	$(MAKEDLK) -d @.^.Lib.select/pyd -s s.linktab -o @.^.Modules.o.selectlink -e initselect

@.^.Modules.o.selectmodule: @.^.Modules.c.selectmodule
	$(CC) -I$(SOCKLIB).include -o $@ @.^.Modules.c.selectmodule

@.^.Lib._socket/pyd: @.^.Modules.o.socketmodule s.linktab 
	$(LINK) -aof -o @.^.Modules.o._socketlink @.^.Modules.o.socketmodule $(SOCKLIB).o.inetlib $(SOCKLIB).o.unixlib $(SOCKLIB).o.socklib
	$(MAKEDLK) -d @.^.Lib._socket/pyd -s s.linktab -o @.^.Modules.o._socketlink -e init_socket

@.^.Modules.o.socketmodule: @.^.Modules.c.socketmodule
	$(CC) -I$(SOCKLIB).include -o $@ @.^.Modules.c.socketmodule


@.^.Lib.zlib/pyd: @.^.Modules.o.zlibmodule s.linktab
	$(LINK) -aof -o @.^.Modules.o.zliblink @.^.Modules.o.zlibmodule $(ZLIB).zlib_lib
	$(MAKEDLK) -d @.^.Lib.zlib/pyd -s s.linktab -o @.^.Modules.o.zliblink -e initzlib

@.^.Modules.o.zlibmodule: @.^.Modules.c.zlibmodule
	$(CC) -I$(ZLIB) -o $@ @.^.Modules.c.zlibmodule


@.^.Lib.pyexpat/pyd: @.^.Modules.o.pyexpat s.linktab
	$(LINK) -aof -o @.^.Modules.o.pyexpatlink @.^.Modules.o.pyexpat $(EXPAT).expat_lib
	$(MAKEDLK) -d @.^.Lib.pyexpat/pyd -s s.linktab -o @.^.Modules.o.pyexpatlink -e initpyexpat

@.^.Modules.o.pyexpat: @.^.Modules.c.pyexpat
	$(CCEXPAT) -o $@ @.^.Modules.c.pyexpat


##########################################################################

o.linktab: s.linktab
	ObjAsm s.linktab o.linktab

s.linktab: $(OBJECTS)
	$(OBJSCAN) -s s.linktab -o $(OBJECTS) $(clib).o.stubs


clean:
	create @.^.Objects.o.dummy
	create @.^.Parser.o.dummy
	create @.^.Modules.o.dummy
	create o.dummy
	create @.^.Python.o.dummy
	create @.^.Lib.dummy/pyc
	create @.^.Lib.dummy/pyo
	create @.^.Lib.plat-riscos.dummy/pyc
	create @.^.Lib.plat-riscos.dummy/pyo
	create @.^.Lib.test.dummy/pyc
	wipe @.^.Modules.o.* ~C ~V
	wipe @.^.Objects.o.* ~C ~V
	wipe @.^.Parser.o.* ~C ~V
	wipe @.^.Python.o.* ~C ~V
	wipe o.* ~C ~V
	wipe @.^.Lib.*/pyc ~C~V
	wipe @.^.Lib.*/pyo ~C~V
	wipe @.^.Lib.plat-riscos.*/pyc ~C~V
	wipe @.^.Lib.plat-riscos.*/pyo ~C~V
	wipe @.^.Lib.test.*/pyc ~C~V

rebuild: clean
	create @.^.Lib.dummy/pyd
	create @.^.$(TARGET)
	create @.^.Lib.plat-riscos.dummy/pyd
	create s.linktab
	create o.linktab
	wipe @.^.$(TARGET) ~C~V
	wipe @.^.Lib.*/pyd ~C ~V
	wipe @.^.Lib.plat-riscos.*/pyd ~C~V
	wipe s.linktab ~C~V
	wipe o.linktab ~C~V

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
