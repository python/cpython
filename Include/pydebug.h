/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

#ifndef Py_PYDEBUG_H
#define Py_PYDEBUG_H
#ifdef __cplusplus
extern "C" {
#endif

extern DL_IMPORT(int) Py_DebugFlag;
extern DL_IMPORT(int) Py_VerboseFlag;
extern DL_IMPORT(int) Py_InteractiveFlag;
extern DL_IMPORT(int) Py_OptimizeFlag;
extern DL_IMPORT(int) Py_NoSiteFlag;
extern DL_IMPORT(int) Py_UseClassExceptionsFlag;
extern DL_IMPORT(int) Py_FrozenFlag;
extern DL_IMPORT(int) Py_TabcheckFlag;
extern DL_IMPORT(int) Py_UnicodeFlag;

DL_IMPORT(void) Py_FatalError(char *message);

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYDEBUG_H */
