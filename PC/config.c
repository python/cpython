/* Module configuration */

/* This file contains the table of built-in modules.
   See init_builtin() in import.c. */

#include "Python.h"

extern void initarray(void);
#ifndef MS_WIN64
extern void initaudioop(void);
#endif
extern void initbinascii(void);
extern void initcmath(void);
extern void initerrno(void);
extern void initgc(void);
#ifndef MS_WIN64
extern void initimageop(void);
#endif
extern void initmath(void);
extern void initmd5(void);
extern void initnt(void);
extern void initoperator(void);
extern void initregex(void);
#ifndef MS_WIN64
extern void initrgbimg(void);
#endif
extern void initrotor(void);
extern void initsignal(void);
extern void initsha(void);
extern void initstrop(void);
extern void initstruct(void);
extern void inittime(void);
extern void initthread(void);
extern void initcStringIO(void);
extern void initcPickle(void);
extern void initpcre(void);
#ifdef WIN32
extern void initmsvcrt(void);
extern void init_locale(void);
#endif
extern void init_codecs(void);
extern void initxreadlines(void);
extern void init_weakref(void);
extern void init_hotshot(void);
extern void initxxsubtype(void);
extern void initzipimport(void);
extern void init_random(void);
extern void inititertools(void);
extern void initcollections(void);
extern void initheapq(void);
extern void init_bisect(void);
extern void init_symtable(void);
extern void initmmap(void);
extern void init_csv(void);
extern void init_sre(void);
extern void initparser(void);
extern void init_winreg(void);
extern void initdatetime(void);

extern void init_multibytecodec(void);
extern void init_codecs_mapdata_ja_JP(void);
extern void init_codecs_mapdata_ko_KR(void);
extern void init_codecs_mapdata_zh_CN(void);
extern void init_codecs_mapdata_zh_TW(void);
extern void init_codecs_shift_jis(void);
extern void init_codecs_cp932(void);
extern void init_codecs_euc_jp(void);
extern void init_codecs_iso2022_jp(void);
extern void init_codecs_iso2022_jp_1(void);
extern void init_codecs_iso2022_jp_2(void);
extern void init_codecs_iso2022_jp_3(void);
extern void init_codecs_iso2022_jp_ext(void);
extern void init_codecs_shift_jisx0213(void);
extern void init_codecs_euc_jisx0213(void);
extern void init_codecs_euc_kr(void);
extern void init_codecs_cp949(void);
extern void init_codecs_johab(void);
extern void init_codecs_iso2022_kr(void);
extern void init_codecs_gb2312(void);
extern void init_codecs_gbk(void);
extern void init_codecs_gb18030(void);
extern void init_codecs_hz(void);
extern void init_codecs_big5(void);
extern void init_codecs_cp950(void);

/* tools/freeze/makeconfig.py marker for additional "extern" */
/* -- ADDMODULE MARKER 1 -- */

extern void PyMarshal_Init(void);
extern void initimp(void);

struct _inittab _PyImport_Inittab[] = {

        {"array", initarray},
#ifdef MS_WINDOWS
#ifndef MS_WIN64
        {"audioop", initaudioop},
#endif
#endif
        {"binascii", initbinascii},
        {"cmath", initcmath},
        {"errno", initerrno},
        {"gc", initgc},
#ifndef MS_WIN64
        {"imageop", initimageop},
#endif
        {"math", initmath},
        {"md5", initmd5},
        {"nt", initnt}, /* Use the NT os functions, not posix */
        {"operator", initoperator},
        {"regex", initregex},
#ifndef MS_WIN64
        {"rgbimg", initrgbimg},
#endif
        {"rotor", initrotor},
        {"signal", initsignal},
        {"sha", initsha},
        {"strop", initstrop},
        {"struct", initstruct},
        {"time", inittime},
#ifdef WITH_THREAD
        {"thread", initthread},
#endif
        {"cStringIO", initcStringIO},
        {"cPickle", initcPickle},
        {"pcre", initpcre},
#ifdef WIN32
        {"msvcrt", initmsvcrt},
        {"_locale", init_locale},
#endif

        {"_codecs", init_codecs},
	{"xreadlines", initxreadlines},
	{"_weakref", init_weakref},
	{"_hotshot", init_hotshot},
	{"_random", init_random},
        {"_bisect", init_bisect},
        {"heapq", initheapq},
	{"itertools", inititertools},
        {"collections", initcollections},
	{"_symtable", init_symtable},
	{"mmap", initmmap},
	{"_csv", init_csv},
	{"_sre", init_sre},
	{"parser", initparser},
	{"_winreg", init_winreg},
	{"datetime", initdatetime},

	{"xxsubtype", initxxsubtype},
	{"zipimport", initzipimport},

	/* CJK codecs */
	{"_multibytecodec", init_multibytecodec},
	{"_codecs_mapdata_ja_JP", init_codecs_mapdata_ja_JP},
	{"_codecs_mapdata_ko_KR", init_codecs_mapdata_ko_KR},
	{"_codecs_mapdata_zh_CN", init_codecs_mapdata_zh_CN},
	{"_codecs_mapdata_zh_TW", init_codecs_mapdata_zh_TW},
	{"_codecs_shift_jis", init_codecs_shift_jis},
	{"_codecs_cp932", init_codecs_cp932},
	{"_codecs_euc_jp", init_codecs_euc_jp},
	{"_codecs_iso2022_jp", init_codecs_iso2022_jp},
	{"_codecs_iso2022_jp_1", init_codecs_iso2022_jp_1},
	{"_codecs_iso2022_jp_2", init_codecs_iso2022_jp_2},
	{"_codecs_iso2022_jp_3", init_codecs_iso2022_jp_3},
	{"_codecs_iso2022_jp_ext", init_codecs_iso2022_jp_ext},
	{"_codecs_shift_jisx0213", init_codecs_shift_jisx0213},
	{"_codecs_euc_jisx0213", init_codecs_euc_jisx0213},
	{"_codecs_euc_kr", init_codecs_euc_kr},
	{"_codecs_cp949", init_codecs_cp949},
	{"_codecs_johab", init_codecs_johab},
	{"_codecs_iso2022_kr", init_codecs_iso2022_kr},
	{"_codecs_gb2312", init_codecs_gb2312},
	{"_codecs_gbk", init_codecs_gbk},
	{"_codecs_gb18030", init_codecs_gb18030},
	{"_codecs_hz", init_codecs_hz},
	{"_codecs_big5", init_codecs_big5},
	{"_codecs_cp950", init_codecs_cp950},

/* tools/freeze/makeconfig.py marker for additional "_inittab" entries */
/* -- ADDMODULE MARKER 2 -- */

        /* This module "lives in" with marshal.c */
        {"marshal", PyMarshal_Init},

        /* This lives it with import.c */
        {"imp", initimp},

        /* These entries are here for sys.builtin_module_names */
        {"__main__", NULL},
        {"__builtin__", NULL},
        {"sys", NULL},
	{"exceptions", NULL},

        /* Sentinel */
        {0, 0}
};
