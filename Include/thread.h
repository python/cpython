#ifndef Py_THREAD_H
#define Py_THREAD_H

/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Header file for old code (such as ILU) that includes "thread.h"
   instead of "pythread.h", and might use the old function names. */

#include "pythread.h"

#define init_thread PyThread_init_thread
#define start_new_thread PyThread_start_new_thread
#ifndef __BEOS__
#define exit_thread PyThread_exit_thread
#define _exit_thread PyThread__exit_thread
#endif
#define get_thread_ident PyThread_get_thread_ident
#define allocate_lock PyThread_allocate_lock
#define free_lock PyThread_free_lock
#define acquire_lock PyThread_acquire_lock
#define release_lock PyThread_release_lock
#define allocate_sema PyThread_allocate_sema
#define free_sema PyThread_free_sema
#define down_sema PyThread_down_sema
#define up_sema PyThread_up_sema
#define exit_prog PyThread_exit_prog
#define _exit_prog PyThread__exit_prog
#define create_key PyThread_create_key
#define delete_key PyThread_delete_key
#define get_key_value PyThread_get_key_value
#define set_key_value PyThread_set_key_value

#endif /* !Py_THREAD_H */
