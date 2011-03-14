/*
     ---------------------------------------------------------------------
    /                       Copyright (c) 1996.                           \
   |          The Regents of the University of California.                 |
   |                        All rights reserved.                           |
   |                                                                       |
   |   Permission to use, copy, modify, and distribute this software for   |
   |   any purpose without fee is hereby granted, provided that this en-   |
   |   tire notice is included in all copies of any software which is or   |
   |   includes  a  copy  or  modification  of  this software and in all   |
   |   copies of the supporting documentation for such software.           |
   |                                                                       |
   |   This  work was produced at the University of California, Lawrence   |
   |   Livermore National Laboratory under  contract  no.  W-7405-ENG-48   |
   |   between  the  U.S.  Department  of  Energy and The Regents of the   |
   |   University of California for the operation of UC LLNL.              |
   |                                                                       |
   |                              DISCLAIMER                               |
   |                                                                       |
   |   This  software was prepared as an account of work sponsored by an   |
   |   agency of the United States Government. Neither the United States   |
   |   Government  nor the University of California nor any of their em-   |
   |   ployees, makes any warranty, express or implied, or  assumes  any   |
   |   liability  or  responsibility  for the accuracy, completeness, or   |
   |   usefulness of any information,  apparatus,  product,  or  process   |
   |   disclosed,   or  represents  that  its  use  would  not  infringe   |
   |   privately-owned rights. Reference herein to any specific  commer-   |
   |   cial  products,  process,  or  service  by trade name, trademark,   |
   |   manufacturer, or otherwise, does not  necessarily  constitute  or   |
   |   imply  its endorsement, recommendation, or favoring by the United   |
   |   States Government or the University of California. The views  and   |
   |   opinions  of authors expressed herein do not necessarily state or   |
   |   reflect those of the United States Government or  the  University   |
   |   of  California,  and shall not be used for advertising or product   |
    \  endorsement purposes.                                              /
     ---------------------------------------------------------------------
*/

/*
                  Floating point exception control module.

   This Python module provides bare-bones control over floating point
   units from several hardware manufacturers.  Specifically, it allows
   the user to turn on the generation of SIGFPE whenever any of the
   three serious IEEE 754 exceptions (Division by Zero, Overflow,
   Invalid Operation) occurs.  We currently ignore Underflow and
   Inexact Result exceptions, although those could certainly be added
   if desired.

   The module also establishes a signal handler for SIGFPE during
   initialization.  This builds on code found in the Python
   distribution at Include/pyfpe.h and Python/pyfpe.c.  If those files
   are not in your Python distribution, find them in a patch at
   ftp://icf.llnl.gov/pub/python/busby/patches.961108.tgz.

   This module is only useful to you if it happens to include code
   specific for your hardware and software environment.  If you can
   contribute OS-specific code for new platforms, or corrections for
   the code provided, it will be greatly appreciated.

   ** Version 1.0: September 20, 1996.  Lee Busby, LLNL.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "Python.h"
#include <signal.h>

#if defined(__FreeBSD__)
#  include <ieeefp.h>
#elif defined(__VMS)
#define __NEW_STARLET
#include <starlet.h>
#include <ieeedef.h>
#endif

#ifndef WANT_SIGFPE_HANDLER
/* Define locally if they are not defined in Python.  This gives only
 * the limited control to induce a core dump in case of an exception.
 */
#include <setjmp.h>
static jmp_buf PyFPE_jbuf;
static int PyFPE_counter = 0;
#endif

typedef void Sigfunc(int);
static Sigfunc sigfpe_handler;
static void fpe_reset(Sigfunc *);

static PyObject *fpe_error;

PyMODINIT_FUNC PyInit_fpectl(void);
static PyObject *turnon_sigfpe            (PyObject *self,PyObject *args);
static PyObject *turnoff_sigfpe           (PyObject *self,PyObject *args);

static PyMethodDef fpectl_methods[] = {
    {"turnon_sigfpe",            (PyCFunction) turnon_sigfpe,            METH_VARARGS},
    {"turnoff_sigfpe",           (PyCFunction) turnoff_sigfpe,           METH_VARARGS},
    {0,0}
};

static PyObject *turnon_sigfpe(PyObject *self,PyObject *args)
{
    /* Do any architecture-specific one-time only initialization here. */

    fpe_reset(sigfpe_handler);
    Py_INCREF (Py_None);
    return Py_None;
}

static void fpe_reset(Sigfunc *handler)
{
    /* Reset the exception handling machinery, and reset the signal
     * handler for SIGFPE to the given handler.
     */

/*-- IRIX -----------------------------------------------------------------*/
#if defined(sgi)
    /* See man page on handle_sigfpes -- must link with -lfpe
     * My usage doesn't follow the man page exactly.  Maybe somebody
     * else can explain handle_sigfpes to me....
     * cc -c -I/usr/local/python/include fpectlmodule.c
     * ld -shared -o fpectlmodule.so fpectlmodule.o -lfpe
     */
#include <sigfpe.h>
    typedef void user_routine (unsigned[5], int[2]);
    typedef void abort_routine (unsigned long);
    handle_sigfpes(_OFF, 0,
                 (user_routine *)0,
                 _TURN_OFF_HANDLER_ON_ERROR,
                 NULL);
    handle_sigfpes(_ON, _EN_OVERFL | _EN_DIVZERO | _EN_INVALID,
                 (user_routine *)0,
                 _ABORT_ON_ERROR,
                 NULL);
    PyOS_setsig(SIGFPE, handler);

/*-- SunOS and Solaris ----------------------------------------------------*/
#elif defined(sun)
    /* References: ieee_handler, ieee_sun, ieee_functions, and ieee_flags
       man pages (SunOS or Solaris)
       cc -c -I/usr/local/python/include fpectlmodule.c
       ld -G -o fpectlmodule.so -L/opt/SUNWspro/lib fpectlmodule.o -lsunmath -lm
     */
#include <math.h>
#ifndef _SUNMATH_H
    extern void nonstandard_arithmetic(void);
    extern int ieee_flags(const char*, const char*, const char*, char **);
    extern long ieee_handler(const char*, const char*, sigfpe_handler_type);
#endif

    char *mode="exception", *in="all", *out;
    (void) nonstandard_arithmetic();
    (void) ieee_flags("clearall",mode,in,&out);
    (void) ieee_handler("set","common",(sigfpe_handler_type)handler);
    PyOS_setsig(SIGFPE, handler);

/*-- HPUX -----------------------------------------------------------------*/
#elif defined(__hppa) || defined(hppa)
    /* References:   fpsetmask man page */
    /* cc -Aa +z -c -I/usr/local/python/include fpectlmodule.c */
    /* ld -b -o fpectlmodule.sl fpectlmodule.o -lm */
#include <math.h>
    fpsetdefaults();
    PyOS_setsig(SIGFPE, handler);

/*-- IBM AIX --------------------------------------------------------------*/
#elif defined(__AIX) || defined(_AIX)
    /* References:   fp_trap, fp_enable man pages */
#include <fptrap.h>
    fp_trap(FP_TRAP_SYNC);
    fp_enable(TRP_INVALID | TRP_DIV_BY_ZERO | TRP_OVERFLOW);
    PyOS_setsig(SIGFPE, handler);

/*-- DEC ALPHA LINUX ------------------------------------------------------*/
#elif defined(__alpha) && defined(linux)
#include <asm/fpu.h>
    unsigned long fp_control =
    IEEE_TRAP_ENABLE_INV | IEEE_TRAP_ENABLE_DZE | IEEE_TRAP_ENABLE_OVF;
    ieee_set_fp_control(fp_control);
    PyOS_setsig(SIGFPE, handler);

/*-- DEC ALPHA VMS --------------------------------------------------------*/
#elif defined(__ALPHA) && defined(__VMS)
        IEEE clrmsk;
        IEEE setmsk;
        clrmsk.ieee$q_flags =
                IEEE$M_TRAP_ENABLE_UNF |  IEEE$M_TRAP_ENABLE_INE |
                 IEEE$M_MAP_UMZ;
        setmsk.ieee$q_flags =
                IEEE$M_TRAP_ENABLE_INV | IEEE$M_TRAP_ENABLE_DZE |
                IEEE$M_TRAP_ENABLE_OVF;
        sys$ieee_set_fp_control(&clrmsk, &setmsk, 0);
        PyOS_setsig(SIGFPE, handler);

/*-- HP IA64 VMS --------------------------------------------------------*/
#elif defined(__ia64) && defined(__VMS)
    PyOS_setsig(SIGFPE, handler);

/*-- Cray Unicos ----------------------------------------------------------*/
#elif defined(cray)
    /* UNICOS delivers SIGFPE by default, but no matherr */
#ifdef HAS_LIBMSET
    libmset(-1);
#endif
    PyOS_setsig(SIGFPE, handler);

/*-- FreeBSD ----------------------------------------------------------------*/
#elif defined(__FreeBSD__)
    fpresetsticky(fpgetsticky());
    fpsetmask(FP_X_INV | FP_X_DZ | FP_X_OFL);
    PyOS_setsig(SIGFPE, handler);

/*-- Linux ----------------------------------------------------------------*/
#elif defined(linux)
#ifdef __GLIBC__
#include <fpu_control.h>
#else
#include <i386/fpu_control.h>
#endif
#ifdef _FPU_SETCW
    {
        fpu_control_t cw = 0x1372;
        _FPU_SETCW(cw);
    }
#else
    __setfpucw(0x1372);
#endif
    PyOS_setsig(SIGFPE, handler);

/*-- Microsoft Windows, NT ------------------------------------------------*/
#elif defined(_MSC_VER)
    /* Reference: Visual C++ Books Online 4.2,
       Run-Time Library Reference, _control87, _controlfp */
#include <float.h>
    unsigned int cw = _EM_INVALID | _EM_ZERODIVIDE | _EM_OVERFLOW;
    (void)_controlfp(0, cw);
    PyOS_setsig(SIGFPE, handler);

/*-- Give Up --------------------------------------------------------------*/
#else
    fputs("Operation not implemented\n", stderr);
#endif

}

static PyObject *turnoff_sigfpe(PyObject *self,PyObject *args)
{
#ifdef __FreeBSD__
    fpresetsticky(fpgetsticky());
    fpsetmask(0);
#elif defined(__VMS)
        IEEE clrmsk;
         clrmsk.ieee$q_flags =
                IEEE$M_TRAP_ENABLE_UNF |  IEEE$M_TRAP_ENABLE_INE |
                IEEE$M_MAP_UMZ | IEEE$M_TRAP_ENABLE_INV |
                IEEE$M_TRAP_ENABLE_DZE | IEEE$M_TRAP_ENABLE_OVF |
                IEEE$M_INHERIT;
        sys$ieee_set_fp_control(&clrmsk, 0, 0);
#else
    fputs("Operation not implemented\n", stderr);
#endif
    Py_INCREF(Py_None);
    return Py_None;
}

static void sigfpe_handler(int signo)
{
    fpe_reset(sigfpe_handler);
    if(PyFPE_counter) {
        longjmp(PyFPE_jbuf, 1);
    } else {
        Py_FatalError("Unprotected floating point exception");
    }
}

static struct PyModuleDef fpectlmodule = {
        PyModuleDef_HEAD_INIT,
        "fpectl",
        NULL,
        -1,
        fpectl_methods,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC PyInit_fpectl(void)
{
    PyObject *m, *d;
    m = PyModule_Create(&fpectlmodule);
    if (m == NULL)
        return NULL;
    d = PyModule_GetDict(m);
    fpe_error = PyErr_NewException("fpectl.error", NULL, NULL);
    if (fpe_error != NULL)
        PyDict_SetItemString(d, "error", fpe_error);
    return m;
}

#ifdef __cplusplus
}
#endif
