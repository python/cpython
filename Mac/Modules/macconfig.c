/***********************************************************
Copyright 1991-1997 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Macintosh Python configuration file */

#include "Python.h"
/* Table of built-in modules.
   These are initialized when first imported.
   Note: selection of optional extensions is now generally done by the
   makesetup script. */

extern void initarray();
extern void initmath();
#ifndef WITHOUT_COMPLEX
extern void initcmath();
#endif
extern void initparser();
extern void initmac();
extern void initMacOS();
extern void initregex();
extern void initstrop();
extern void initstruct();
extern void inittime();
extern void initdbm();
extern void initfcntl();
extern void initnis();
extern void initpwd();
extern void initgrp();
extern void initcrypt();
extern void initselect();
extern void init_socket();
extern void initaudioop();
extern void initimageop();
extern void initrgbimg();
extern void initmd5();
extern void initmpz();
extern void initrotor();
extern void inital();
extern void initcd();
extern void initcl();
extern void initfm();
extern void initgl();
extern void initimgfile();
extern void initimgformat();
extern void initsgi();
extern void initsv();
extern void initfl();
extern void initthread();
extern void inittiming();
extern void initsignal();
extern void initnew();
extern void initdl();
extern void initsyslog();
extern void initgestalt();
extern void initmacfs();
extern void initbinascii();
extern void initoperator();
extern void initerrno();
extern void initpcre();
extern void initunicodedata();
extern void init_codecs();
extern void initNav();
extern void inithfsplus();
#ifdef USE_MACCTB
extern void initctb();
#endif
#ifdef USE_MACSPEECH
extern void initmacspeech();
#endif
#ifdef USE_IC
extern void initicglue();
#endif
#ifdef USE_TOOLBOX
#ifndef USE_CORE_TOOLBOX
#define USE_CORE_TOOLBOX
#endif
extern void init_App();
extern void init_Fm();
extern void init_Help();
extern void init_Icn();
extern void init_List();
extern void init_Mlte();
extern void init_Qdoffs();
extern void init_Snd();
extern void init_Sndihooks();
extern void init_Scrap();
extern void init_TE();
extern void initColorPicker();
extern void initPrinting();
extern void init_CF();
#endif
#ifdef USE_CORE_TOOLBOX
extern void init_AE();
extern void init_Ctl();
extern void init_Dlg();
extern void init_Drag();
extern void init_Evt();
extern void init_Menu();
extern void init_Qd();
extern void init_Res();
extern void init_Win();
#endif
#ifdef USE_QT
extern void init_Cm();
extern void init_Qt();
#endif

#ifdef USE_IMG
extern void initimgcolormap();
extern void initimgformat();
extern void initimggif();
extern void initimgjpeg();
extern void initimgpbm();
extern void initimgppm();
extern void initimgpgm();
extern void initimgtiff();
extern void initimgsgi();
extern void initimgpng();
extern void initimgop();
#endif
#ifdef USE_TK
extern void init_tkinter();
#endif
#ifdef USE_GUSI
extern void init_socket();
extern void initselect();
#endif
#ifdef USE_WASTE
extern void initwaste();
#endif
#ifdef USE_GDBM
extern void initgdbm();
#endif
#ifdef USE_ZLIB
extern void initzlib();
#endif
#ifdef WITH_THREAD
extern void initthread();
#endif
#ifdef WITH_HOTSHOT
extern void init_hotshot();
#endif
#ifdef USE_PYEXPAT
extern void initpyexpat();
#endif
#ifdef WITH_CYCLE_GC
extern void initgc();
#endif

extern void initcPickle();
extern void initcStringIO();
extern void init_codecs();
extern void initsha();
extern void init_locale();
extern void init_sre();
extern void initxreadlines();
/* -- ADDMODULE MARKER 1 -- */

extern void PyMarshal_Init();
extern void initimp();

struct _inittab _PyImport_Inittab[] = {

	{"array", initarray},
	{"math", initmath},
#ifndef WITHOUT_COMPLEX
	{"cmath", initcmath},
#endif
	{"parser", initparser},
	{"mac", initmac},
	{"MacOS", initMacOS},
	{"regex", initregex},
	{"strop", initstrop},
	{"struct", initstruct},
	{"time", inittime},
	{"audioop", initaudioop},
	{"imageop", initimageop},
	{"rgbimg", initrgbimg},
	{"md5", initmd5},
	{"rotor", initrotor},
	{"new", initnew},
	{"gestalt", initgestalt},
	{"macfs", initmacfs},
	{"binascii", initbinascii},
	{"operator", initoperator},
	{"errno", initerrno},
	{"pcre", initpcre},
	{"unicodedata", initunicodedata},
	{"_codecs", init_codecs},
	{"sha", initsha},
	{"Nav", initNav},
#ifdef USE_MACCTB
	{"ctb", initctb},
#endif
/* This could probably be made to work on other compilers... */
#ifdef USE_MACSPEECH
	{"macspeech", initmacspeech},
#endif
#ifdef USE_IC
	{"icglue", initicglue},
#endif
#ifdef USE_CORE_TOOLBOX
	{"_AE", init_AE},
	{"_Ctl", init_Ctl},
	{"_Dlg", init_Dlg},
	{"_Drag", init_Drag},
	{"_Evt", init_Evt},
	{"_Menu", init_Menu},
	{"_Qd", init_Qd},
	{"_Win", init_Win},
	{"_Res", init_Res},
#endif
#ifdef USE_TOOLBOX
	{"_App", init_App},
	{"_Fm", init_Fm},
	{"_Icn", init_Icn},
	{"_List", init_List},
	{"_Mlte", init_Mlte},
	{"_Qdoffs", init_Qdoffs},
	{"_Snd", init_Snd},
	{"_Sndihooks", init_Sndihooks},
	/* Carbon scrap manager is completely different */
	{"_Scrap", init_Scrap},
	{"_TE", init_TE},
	{"ColorPicker", initColorPicker},
#if !TARGET_API_MAC_CARBON
	{"_Help", init_Help},
	{"Printing", initPrinting},
#endif
#if TARGET_API_MAC_CARBON
	{"_CF", init_CF},
#endif
#if TARGET_API_MAC_CARBON
	{"hfsplus", inithfsplus},
#endif
#endif /* USE_TOOLBOX */
#ifdef USE_QT
	{"_Cm", init_Cm},
	{"_Qt", init_Qt},
#endif
#ifdef USE_IMG
	{"imgcolormap",	initimgcolormap},
	{"imgformat",	initimgformat},
	{"imggif",	initimggif},
	{"imgjpeg",	initimgjpeg},
	{"imgpbm",	initimgpbm},
	{"imgppm",	initimgppm},
	{"imgpgm",	initimgpgm},
	{"imgtiff",	initimgtiff},
	{"imgsgi",	initimgsgi},
	{"imgpng",	initimgpng},
	{"imgop",	initimgop},
#endif
#ifdef USE_TK
	{"_tkinter",	init_tkinter},
#endif
#ifdef USE_GUSI
	{"_socket",	init_socket},
	{"select",	initselect},
#endif
#ifdef USE_WASTE
	{"waste",	initwaste},
#endif
#ifdef USE_GDBM
	{"gdbm",	initgdbm},
#endif /* USE_GDBM */
#ifdef USE_ZLIB
	{"zlib",	initzlib},
#endif
#ifdef WITH_THREAD
	{"thread",	initthread},
#endif
#ifdef WITH_HOTSHOT
	{"_hotshot",	init_hotshot},
#endif
#ifdef USE_PYEXPAT
	{"pyexpat", initpyexpat},
#endif
#ifdef WITH_CYCLE_GC
	{"gc", initgc},
#endif
	{"cPickle",	initcPickle},
	{"cStringIO",	initcStringIO},
	{"_locale", init_locale},
	{"_sre", init_sre},
	{"xreadlines", initxreadlines},
/* -- ADDMODULE MARKER 2 -- */

	/* This module "lives in" with marshal.c */
	{"marshal", PyMarshal_Init},
	
	/* This module "lives in" with import.c */
	{"imp", initimp},

	/* These entries are here for sys.builtin_module_names */
	{"__main__", NULL},
	{"__builtin__", NULL},
	{"exceptions", NULL},
	{"sys", NULL},

	/* Sentinel */
	{0, 0}
};
