# Microsoft Visual C++ generated build script - Do not modify

PROJ = PYTHON
DEBUG = 0
PROGTYPE = 4
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = -d_DEBUG
R_RCDEFINES = -dNDEBUG
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = O:\PYTH14B2\PC\VC15_LIB\
USEMFC = 0
CC = cl
CPP = cl
CXX = cl
CCREATEPCHFLAG = 
CPPCREATEPCHFLAG = 
CUSEPCHFLAG = 
CPPUSEPCHFLAG = 
FIRSTC = ABSTRACT.C  
FIRSTCPP =             
RC = rc
CFLAGS_D_LIB = /nologo /G3 /W3 /Z7 /AL /Gt9 /Od /D "_DEBUG" /D "HAVE_CONFIG_H" /FR 
CFLAGS_R_LIB = /nologo /G3 /W3 /AL /Gt9 /O2 /D "NDEBUG" /D "HAVE_CONFIG_H" /FR 
RCFLAGS = /nologo
RESFLAGS = /nologo
RUNFLAGS = 
OBJS_EXT = 
LIBS_EXT = 
!if "$(DEBUG)" == "1"
CFLAGS = $(CFLAGS_D_LIB)
LFLAGS = 
LIBS = 
MAPFILE = nul
RCDEFINES = $(D_RCDEFINES)
!else
CFLAGS = $(CFLAGS_R_LIB)
LFLAGS = 
LIBS = 
MAPFILE = nul
RCDEFINES = $(R_RCDEFINES)
!endif
!if [if exist MSVC.BND del MSVC.BND]
!endif
SBRS = ABSTRACT.SBR \
		ACCELER.SBR \
		ACCESSOB.SBR \
		ARRAYMOD.SBR \
		AUDIOOP.SBR \
		BINASCII.SBR \
		BLTINMOD.SBR \
		CEVAL.SBR \
		CGENSUPP.SBR \
		CLASSOBJ.SBR \
		COMPILE.SBR \
		CONFIG.SBR \
		ENVIRONM.SBR \
		ERRORS.SBR \
		FILEOBJE.SBR \
		FLOATOBJ.SBR \
		FRAMEOBJ.SBR \
		FROZEN.SBR \
		FUNCOBJE.SBR \
		GETARGS.SBR \
		GETCOMPI.SBR \
		GETCOPYR.SBR \
		GETMTIME.SBR \
		GETPATH.SBR \
		GETPLATF.SBR \
		GETVERSI.SBR \
		GRAMINIT.SBR \
		GRAMMAR1.SBR \
		IMAGEOP.SBR \
		IMPORT.SBR \
		IMPORTDL.SBR \
		INTOBJEC.SBR \
		LISTOBJE.SBR \
		LONGOBJE.SBR \
		MAPPINGO.SBR \
		MARSHAL.SBR \
		MATHMODU.SBR \
		MD5C.SBR \
		MD5MODUL.SBR \
		METHODOB.SBR \
		MODSUPPO.SBR \
		MODULEOB.SBR \
		MYREADLI.SBR \
		MYSTRTOU.SBR \
		NEWMODUL.SBR \
		NODE.SBR \
		OBJECT.SBR \
		PARSER.SBR \
		PARSETOK.SBR \
		POSIXMOD.SBR \
		PYTHONRU.SBR \
		RANGEOBJ.SBR \
		REGEXMOD.SBR \
		REGEXPR.SBR \
		RGBIMGMO.SBR \
		ROTORMOD.SBR \
		SIGNALMO.SBR \
		SOUNDEX.SBR \
		STRINGOB.SBR \
		STROPMOD.SBR \
		STRUCTME.SBR \
		STRUCTMO.SBR \
		SYSMODUL.SBR \
		TIMEMODU.SBR \
		TOKENIZE.SBR \
		TRACEBAC.SBR \
		TUPLEOBJ.SBR \
		TYPEOBJE.SBR \
		YUVCONVE.SBR \
		COBJECT.SBR \
		COMPLEXO.SBR \
		CMATHMOD.SBR \
		ERRNOMOD.SBR \
		OPERATOR.SBR \
		SLICEOBJ.SBR


ABSTRACT_DEP = o:\pyth14b2\pc\src\python.h \
	o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


ACCELER_DEP = o:\pyth14b2\pc\src\pgenhead.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\grammar.h \
	o:\pyth14b2\pc\src\bitset.h \
	o:\pyth14b2\pc\src\node.h \
	o:\pyth14b2\pc\src\token.h \
	o:\pyth14b2\pc\src\parser.h


ACCESSOB_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\structme.h


ARRAYMOD_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


AUDIOOP_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\mymath.h


BINASCII_DEP = o:\pyth14b2\pc\src\python.h \
	o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


BLTINMOD_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\node.h \
	o:\pyth14b2\pc\src\graminit.h \
	o:\pyth14b2\pc\src\compile.h \
	o:\pyth14b2\pc\src\eval.h


CEVAL_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\compile.h \
	o:\pyth14b2\pc\src\frameobj.h \
	o:\pyth14b2\pc\src\eval.h \
	o:\pyth14b2\pc\src\opcode.h \
	o:\pyth14b2\pc\src\graminit.h


CGENSUPP_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\cgensupp.h


CLASSOBJ_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\structme.h


COMPILE_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\node.h \
	o:\pyth14b2\pc\src\token.h \
	o:\pyth14b2\pc\src\graminit.h \
	o:\pyth14b2\pc\src\compile.h \
	o:\pyth14b2\pc\src\opcode.h \
	o:\pyth14b2\pc\src\structme.h


CONFIG_DEP = o:\pyth14b2\pc\src\python.h \
	o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


ENVIRONM_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


ERRORS_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


FILEOBJE_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\structme.h


FLOATOBJ_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\mymath.h


FRAMEOBJ_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\compile.h \
	o:\pyth14b2\pc\src\frameobj.h \
	o:\pyth14b2\pc\src\opcode.h \
	o:\pyth14b2\pc\src\structme.h


FROZEN_DEP = o:\pyth14b2\pc\src\python.h \
	o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


FUNCOBJE_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\compile.h \
	o:\pyth14b2\pc\src\structme.h


GETARGS_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


GETCOMPI_DEP = o:\pyth14b2\pc\src\python.h \
	o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


GETCOPYR_DEP = o:\pyth14b2\pc\src\python.h \
	o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


GETMTIME_DEP = o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\rename2.h


GETPATH_DEP = o:\pyth14b2\pc\src\python.h \
	o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\osdefs.h


GETPLATF_DEP = o:\pyth14b2\pc\src\python.h \
	o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


GETVERSI_DEP = o:\pyth14b2\pc\src\python.h \
	o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\patchlev.h


GRAMINIT_DEP = o:\pyth14b2\pc\src\pgenhead.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\grammar.h \
	o:\pyth14b2\pc\src\bitset.h


GRAMMAR1_DEP = o:\pyth14b2\pc\src\pgenhead.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\grammar.h \
	o:\pyth14b2\pc\src\bitset.h \
	o:\pyth14b2\pc\src\token.h


IMAGEOP_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


IMPORT_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\node.h \
	o:\pyth14b2\pc\src\token.h \
	o:\pyth14b2\pc\src\graminit.h \
	o:\pyth14b2\pc\src\errcode.h \
	o:\pyth14b2\pc\src\marshal.h \
	o:\pyth14b2\pc\src\compile.h \
	o:\pyth14b2\pc\src\eval.h \
	o:\pyth14b2\pc\src\osdefs.h \
	o:\pyth14b2\pc\src\importdl.h


IMPORTDL_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\osdefs.h \
	o:\pyth14b2\pc\src\importdl.h


INTOBJEC_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


LISTOBJE_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


LONGOBJE_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\longintr.h \
	o:\pyth14b2\pc\src\mymath.h


MAPPINGO_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


MARSHAL_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\longintr.h \
	o:\pyth14b2\pc\src\compile.h \
	o:\pyth14b2\pc\src\marshal.h


MATHMODU_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\mymath.h


MD5C_DEP = o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\md5.h


MD5MODUL_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\md5.h


METHODOB_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\token.h


MODSUPPO_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


MODULEOB_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


MYREADLI_DEP = o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\intrchec.h


MYSTRTOU_DEP = o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\rename2.h


NEWMODUL_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\compile.h


NODE_DEP = o:\pyth14b2\pc\src\pgenhead.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\node.h


OBJECT_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


PARSER_DEP = o:\pyth14b2\pc\src\pgenhead.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\token.h \
	o:\pyth14b2\pc\src\grammar.h \
	o:\pyth14b2\pc\src\bitset.h \
	o:\pyth14b2\pc\src\node.h \
	o:\pyth14b2\pc\src\parser.h \
	o:\pyth14b2\pc\src\errcode.h


PARSETOK_DEP = o:\pyth14b2\pc\src\pgenhead.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\tokenize.h \
	o:\pyth14b2\pc\src\token.h \
	o:\pyth14b2\pc\src\node.h \
	o:\pyth14b2\pc\src\grammar.h \
	o:\pyth14b2\pc\src\bitset.h \
	o:\pyth14b2\pc\src\parser.h \
	o:\pyth14b2\pc\src\parsetok.h \
	o:\pyth14b2\pc\src\errcode.h


POSIXMOD_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\mytime.h


PYTHONRU_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\grammar.h \
	o:\pyth14b2\pc\src\bitset.h \
	o:\pyth14b2\pc\src\node.h \
	o:\pyth14b2\pc\src\parsetok.h \
	o:\pyth14b2\pc\src\graminit.h \
	o:\pyth14b2\pc\src\errcode.h \
	o:\pyth14b2\pc\src\compile.h \
	o:\pyth14b2\pc\src\eval.h \
	o:\pyth14b2\pc\src\marshal.h


RANGEOBJ_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


REGEXMOD_DEP = o:\pyth14b2\pc\src\python.h \
	o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\regexpr.h


REGEXPR_DEP = o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\regexpr.h \
	o:\pyth14b2\pc\src\config.h


RGBIMGMO_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


ROTORMOD_DEP = o:\pyth14b2\pc\src\python.h \
	o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\mymath.h


SIGNALMO_DEP = o:\pyth14b2\pc\src\python.h \
	o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


SOUNDEX_DEP = o:\pyth14b2\pc\src\python.h \
	o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


STRINGOB_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


STROPMOD_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


STRUCTME_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\structme.h


STRUCTMO_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


SYSMODUL_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\osdefs.h


TIMEMODU_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\myselect.h \
	o:\pyth14b2\pc\src\mytime.h


TOKENIZE_DEP = o:\pyth14b2\pc\src\pgenhead.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\tokenize.h \
	o:\pyth14b2\pc\src\token.h \
	o:\pyth14b2\pc\src\errcode.h


TRACEBAC_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\compile.h \
	o:\pyth14b2\pc\src\frameobj.h \
	o:\pyth14b2\pc\src\structme.h \
	o:\pyth14b2\pc\src\osdefs.h


TUPLEOBJ_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


TYPEOBJE_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


YUVCONVE_DEP = o:\pyth14b2\pc\src\yuv.h


COBJECT_DEP = o:\pyth14b2\pc\src\python.h \
	o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


COMPLEXO_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\mymath.h


CMATHMOD_DEP = o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h \
	o:\pyth14b2\pc\src\mymath.h


ERRNOMOD_DEP = o:\pyth14b2\pc\src\python.h \
	o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


OPERATOR_DEP = o:\pyth14b2\pc\src\python.h \
	o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


SLICEOBJ_DEP = o:\pyth14b2\pc\src\python.h \
	o:\pyth14b2\pc\src\allobjec.h \
	o:\pyth14b2\pc\src\config.h \
	o:\pyth14b2\pc\src\myproto.h \
	o:\pyth14b2\pc\src\rename2.h \
	o:\pyth14b2\pc\src\object.h \
	o:\pyth14b2\pc\src\objimpl.h \
	o:\pyth14b2\pc\src\pydebug.h \
	o:\pyth14b2\pc\src\accessob.h \
	o:\pyth14b2\pc\src\intobjec.h \
	o:\pyth14b2\pc\src\longobje.h \
	o:\pyth14b2\pc\src\floatobj.h \
	o:\pyth14b2\pc\src\complexo.h \
	o:\pyth14b2\pc\src\rangeobj.h \
	o:\pyth14b2\pc\src\stringob.h \
	o:\pyth14b2\pc\src\tupleobj.h \
	o:\pyth14b2\pc\src\listobje.h \
	o:\pyth14b2\pc\src\mappingo.h \
	o:\pyth14b2\pc\src\methodob.h \
	o:\pyth14b2\pc\src\moduleob.h \
	o:\pyth14b2\pc\src\funcobje.h \
	o:\pyth14b2\pc\src\classobj.h \
	o:\pyth14b2\pc\src\thread.h \
	o:\pyth14b2\pc\src\fileobje.h \
	o:\pyth14b2\pc\src\cobject.h \
	o:\pyth14b2\pc\src\tracebac.h \
	o:\pyth14b2\pc\src\sliceobj.h \
	o:\pyth14b2\pc\src\errors.h \
	o:\pyth14b2\pc\src\mymalloc.h \
	o:\pyth14b2\pc\src\modsuppo.h \
	o:\pyth14b2\pc\src\ceval.h \
	o:\pyth14b2\pc\src\pythonru.h \
	o:\pyth14b2\pc\src\sysmodul.h \
	o:\pyth14b2\pc\src\intrchec.h \
	o:\pyth14b2\pc\src\import.h \
	o:\pyth14b2\pc\src\bltinmod.h \
	o:\pyth14b2\pc\src\abstract.h


all:	$(PROJ).LIB $(PROJ).BSC

ABSTRACT.OBJ:	..\SRC\ABSTRACT.C $(ABSTRACT_DEP)
	$(CC) $(CFLAGS) $(CCREATEPCHFLAG) /c ..\SRC\ABSTRACT.C

ACCELER.OBJ:	..\SRC\ACCELER.C $(ACCELER_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\ACCELER.C

ACCESSOB.OBJ:	..\SRC\ACCESSOB.C $(ACCESSOB_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\ACCESSOB.C

ARRAYMOD.OBJ:	..\SRC\ARRAYMOD.C $(ARRAYMOD_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\ARRAYMOD.C

AUDIOOP.OBJ:	..\SRC\AUDIOOP.C $(AUDIOOP_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\AUDIOOP.C

BINASCII.OBJ:	..\SRC\BINASCII.C $(BINASCII_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\BINASCII.C

BLTINMOD.OBJ:	..\SRC\BLTINMOD.C $(BLTINMOD_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\BLTINMOD.C

CEVAL.OBJ:	..\SRC\CEVAL.C $(CEVAL_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\CEVAL.C

CGENSUPP.OBJ:	..\SRC\CGENSUPP.C $(CGENSUPP_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\CGENSUPP.C

CLASSOBJ.OBJ:	..\SRC\CLASSOBJ.C $(CLASSOBJ_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\CLASSOBJ.C

COMPILE.OBJ:	..\SRC\COMPILE.C $(COMPILE_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\COMPILE.C

CONFIG.OBJ:	..\SRC\CONFIG.C $(CONFIG_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\CONFIG.C

ENVIRONM.OBJ:	..\SRC\ENVIRONM.C $(ENVIRONM_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\ENVIRONM.C

ERRORS.OBJ:	..\SRC\ERRORS.C $(ERRORS_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\ERRORS.C

FILEOBJE.OBJ:	..\SRC\FILEOBJE.C $(FILEOBJE_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\FILEOBJE.C

FLOATOBJ.OBJ:	..\SRC\FLOATOBJ.C $(FLOATOBJ_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\FLOATOBJ.C

FRAMEOBJ.OBJ:	..\SRC\FRAMEOBJ.C $(FRAMEOBJ_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\FRAMEOBJ.C

FROZEN.OBJ:	..\SRC\FROZEN.C $(FROZEN_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\FROZEN.C

FUNCOBJE.OBJ:	..\SRC\FUNCOBJE.C $(FUNCOBJE_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\FUNCOBJE.C

GETARGS.OBJ:	..\SRC\GETARGS.C $(GETARGS_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\GETARGS.C

GETCOMPI.OBJ:	..\SRC\GETCOMPI.C $(GETCOMPI_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\GETCOMPI.C

GETCOPYR.OBJ:	..\SRC\GETCOPYR.C $(GETCOPYR_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\GETCOPYR.C

GETMTIME.OBJ:	..\SRC\GETMTIME.C $(GETMTIME_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\GETMTIME.C

GETPATH.OBJ:	..\SRC\GETPATH.C $(GETPATH_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\GETPATH.C

GETPLATF.OBJ:	..\SRC\GETPLATF.C $(GETPLATF_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\GETPLATF.C

GETVERSI.OBJ:	..\SRC\GETVERSI.C $(GETVERSI_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\GETVERSI.C

GRAMINIT.OBJ:	..\SRC\GRAMINIT.C $(GRAMINIT_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\GRAMINIT.C

GRAMMAR1.OBJ:	..\SRC\GRAMMAR1.C $(GRAMMAR1_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\GRAMMAR1.C

IMAGEOP.OBJ:	..\SRC\IMAGEOP.C $(IMAGEOP_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\IMAGEOP.C

IMPORT.OBJ:	..\SRC\IMPORT.C $(IMPORT_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\IMPORT.C

IMPORTDL.OBJ:	..\SRC\IMPORTDL.C $(IMPORTDL_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\IMPORTDL.C

INTOBJEC.OBJ:	..\SRC\INTOBJEC.C $(INTOBJEC_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\INTOBJEC.C

LISTOBJE.OBJ:	..\SRC\LISTOBJE.C $(LISTOBJE_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\LISTOBJE.C

LONGOBJE.OBJ:	..\SRC\LONGOBJE.C $(LONGOBJE_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\LONGOBJE.C

MAPPINGO.OBJ:	..\SRC\MAPPINGO.C $(MAPPINGO_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\MAPPINGO.C

MARSHAL.OBJ:	..\SRC\MARSHAL.C $(MARSHAL_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\MARSHAL.C

MATHMODU.OBJ:	..\SRC\MATHMODU.C $(MATHMODU_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\MATHMODU.C

MD5C.OBJ:	..\SRC\MD5C.C $(MD5C_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\MD5C.C

MD5MODUL.OBJ:	..\SRC\MD5MODUL.C $(MD5MODUL_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\MD5MODUL.C

METHODOB.OBJ:	..\SRC\METHODOB.C $(METHODOB_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\METHODOB.C

MODSUPPO.OBJ:	..\SRC\MODSUPPO.C $(MODSUPPO_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\MODSUPPO.C

MODULEOB.OBJ:	..\SRC\MODULEOB.C $(MODULEOB_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\MODULEOB.C

MYREADLI.OBJ:	..\SRC\MYREADLI.C $(MYREADLI_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\MYREADLI.C

MYSTRTOU.OBJ:	..\SRC\MYSTRTOU.C $(MYSTRTOU_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\MYSTRTOU.C

NEWMODUL.OBJ:	..\SRC\NEWMODUL.C $(NEWMODUL_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\NEWMODUL.C

NODE.OBJ:	..\SRC\NODE.C $(NODE_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\NODE.C

OBJECT.OBJ:	..\SRC\OBJECT.C $(OBJECT_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\OBJECT.C

PARSER.OBJ:	..\SRC\PARSER.C $(PARSER_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\PARSER.C

PARSETOK.OBJ:	..\SRC\PARSETOK.C $(PARSETOK_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\PARSETOK.C

POSIXMOD.OBJ:	..\SRC\POSIXMOD.C $(POSIXMOD_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\POSIXMOD.C

PYTHONRU.OBJ:	..\SRC\PYTHONRU.C $(PYTHONRU_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\PYTHONRU.C

RANGEOBJ.OBJ:	..\SRC\RANGEOBJ.C $(RANGEOBJ_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\RANGEOBJ.C

REGEXMOD.OBJ:	..\SRC\REGEXMOD.C $(REGEXMOD_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\REGEXMOD.C

REGEXPR.OBJ:	..\SRC\REGEXPR.C $(REGEXPR_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\REGEXPR.C

RGBIMGMO.OBJ:	..\SRC\RGBIMGMO.C $(RGBIMGMO_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\RGBIMGMO.C

ROTORMOD.OBJ:	..\SRC\ROTORMOD.C $(ROTORMOD_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\ROTORMOD.C

SIGNALMO.OBJ:	..\SRC\SIGNALMO.C $(SIGNALMO_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\SIGNALMO.C

SOUNDEX.OBJ:	..\SRC\SOUNDEX.C $(SOUNDEX_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\SOUNDEX.C

STRINGOB.OBJ:	..\SRC\STRINGOB.C $(STRINGOB_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\STRINGOB.C

STROPMOD.OBJ:	..\SRC\STROPMOD.C $(STROPMOD_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\STROPMOD.C

STRUCTME.OBJ:	..\SRC\STRUCTME.C $(STRUCTME_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\STRUCTME.C

STRUCTMO.OBJ:	..\SRC\STRUCTMO.C $(STRUCTMO_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\STRUCTMO.C

SYSMODUL.OBJ:	..\SRC\SYSMODUL.C $(SYSMODUL_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\SYSMODUL.C

TIMEMODU.OBJ:	..\SRC\TIMEMODU.C $(TIMEMODU_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\TIMEMODU.C

TOKENIZE.OBJ:	..\SRC\TOKENIZE.C $(TOKENIZE_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\TOKENIZE.C

TRACEBAC.OBJ:	..\SRC\TRACEBAC.C $(TRACEBAC_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\TRACEBAC.C

TUPLEOBJ.OBJ:	..\SRC\TUPLEOBJ.C $(TUPLEOBJ_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\TUPLEOBJ.C

TYPEOBJE.OBJ:	..\SRC\TYPEOBJE.C $(TYPEOBJE_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\TYPEOBJE.C

YUVCONVE.OBJ:	..\SRC\YUVCONVE.C $(YUVCONVE_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\YUVCONVE.C

COBJECT.OBJ:	..\SRC\COBJECT.C $(COBJECT_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\COBJECT.C

COMPLEXO.OBJ:	..\SRC\COMPLEXO.C $(COMPLEXO_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\COMPLEXO.C

CMATHMOD.OBJ:	..\SRC\CMATHMOD.C $(CMATHMOD_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\CMATHMOD.C

ERRNOMOD.OBJ:	..\SRC\ERRNOMOD.C $(ERRNOMOD_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\ERRNOMOD.C

OPERATOR.OBJ:	..\SRC\OPERATOR.C $(OPERATOR_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\OPERATOR.C

SLICEOBJ.OBJ:	..\SRC\SLICEOBJ.C $(SLICEOBJ_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c ..\SRC\SLICEOBJ.C

$(PROJ).LIB::	ABSTRACT.OBJ ACCELER.OBJ ACCESSOB.OBJ ARRAYMOD.OBJ AUDIOOP.OBJ BINASCII.OBJ \
	BLTINMOD.OBJ CEVAL.OBJ CGENSUPP.OBJ CLASSOBJ.OBJ COMPILE.OBJ CONFIG.OBJ ENVIRONM.OBJ \
	ERRORS.OBJ FILEOBJE.OBJ FLOATOBJ.OBJ FRAMEOBJ.OBJ FROZEN.OBJ FUNCOBJE.OBJ GETARGS.OBJ \
	GETCOMPI.OBJ GETCOPYR.OBJ GETMTIME.OBJ GETPATH.OBJ GETPLATF.OBJ GETVERSI.OBJ GRAMINIT.OBJ \
	GRAMMAR1.OBJ IMAGEOP.OBJ IMPORT.OBJ IMPORTDL.OBJ INTOBJEC.OBJ LISTOBJE.OBJ LONGOBJE.OBJ \
	MAPPINGO.OBJ MARSHAL.OBJ MATHMODU.OBJ MD5C.OBJ MD5MODUL.OBJ METHODOB.OBJ MODSUPPO.OBJ \
	MODULEOB.OBJ MYREADLI.OBJ MYSTRTOU.OBJ NEWMODUL.OBJ NODE.OBJ OBJECT.OBJ PARSER.OBJ \
	PARSETOK.OBJ POSIXMOD.OBJ PYTHONRU.OBJ RANGEOBJ.OBJ REGEXMOD.OBJ REGEXPR.OBJ RGBIMGMO.OBJ \
	ROTORMOD.OBJ SIGNALMO.OBJ SOUNDEX.OBJ STRINGOB.OBJ STROPMOD.OBJ STRUCTME.OBJ STRUCTMO.OBJ \
	SYSMODUL.OBJ TIMEMODU.OBJ TOKENIZE.OBJ TRACEBAC.OBJ TUPLEOBJ.OBJ TYPEOBJE.OBJ YUVCONVE.OBJ \
	COBJECT.OBJ COMPLEXO.OBJ CMATHMOD.OBJ ERRNOMOD.OBJ OPERATOR.OBJ SLICEOBJ.OBJ $(OBJS_EXT)
	echo >NUL @<<$(PROJ).CRF
$@ /PAGESIZE:64
y
+ABSTRACT.OBJ &
+ACCELER.OBJ &
+ACCESSOB.OBJ &
+ARRAYMOD.OBJ &
+AUDIOOP.OBJ &
+BINASCII.OBJ &
+BLTINMOD.OBJ &
+CEVAL.OBJ &
+CGENSUPP.OBJ &
+CLASSOBJ.OBJ &
+COMPILE.OBJ &
+CONFIG.OBJ &
+ENVIRONM.OBJ &
+ERRORS.OBJ &
+FILEOBJE.OBJ &
+FLOATOBJ.OBJ &
+FRAMEOBJ.OBJ &
+FROZEN.OBJ &
+FUNCOBJE.OBJ &
+GETARGS.OBJ &
+GETCOMPI.OBJ &
+GETCOPYR.OBJ &
+GETMTIME.OBJ &
+GETPATH.OBJ &
+GETPLATF.OBJ &
+GETVERSI.OBJ &
+GRAMINIT.OBJ &
+GRAMMAR1.OBJ &
+IMAGEOP.OBJ &
+IMPORT.OBJ &
+IMPORTDL.OBJ &
+INTOBJEC.OBJ &
+LISTOBJE.OBJ &
+LONGOBJE.OBJ &
+MAPPINGO.OBJ &
+MARSHAL.OBJ &
+MATHMODU.OBJ &
+MD5C.OBJ &
+MD5MODUL.OBJ &
+METHODOB.OBJ &
+MODSUPPO.OBJ &
+MODULEOB.OBJ &
+MYREADLI.OBJ &
+MYSTRTOU.OBJ &
+NEWMODUL.OBJ &
+NODE.OBJ &
+OBJECT.OBJ &
+PARSER.OBJ &
+PARSETOK.OBJ &
+POSIXMOD.OBJ &
+PYTHONRU.OBJ &
+RANGEOBJ.OBJ &
+REGEXMOD.OBJ &
+REGEXPR.OBJ &
+RGBIMGMO.OBJ &
+ROTORMOD.OBJ &
+SIGNALMO.OBJ &
+SOUNDEX.OBJ &
+STRINGOB.OBJ &
+STROPMOD.OBJ &
+STRUCTME.OBJ &
+STRUCTMO.OBJ &
+SYSMODUL.OBJ &
+TIMEMODU.OBJ &
+TOKENIZE.OBJ &
+TRACEBAC.OBJ &
+TUPLEOBJ.OBJ &
+TYPEOBJE.OBJ &
+YUVCONVE.OBJ &
+COBJECT.OBJ &
+COMPLEXO.OBJ &
+CMATHMOD.OBJ &
+ERRNOMOD.OBJ &
+OPERATOR.OBJ &
+SLICEOBJ.OBJ &
;
<<
	if exist $@ del $@
	lib @$(PROJ).CRF

$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
