#ifndef Py_CEVAL_H
#define Py_CEVAL_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright 1991, 1992, 1993 by Stichting Mathematisch Centrum,
Amsterdam, The Netherlands.

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

/* Interface to random parts in ceval.c */

object *call_object PROTO((object *, object *));

object *getglobals PROTO((void));
object *getlocals PROTO((void));
object *getowner PROTO((void));
void mergelocals PROTO((void));

void printtraceback PROTO((object *));
void flushline PROTO((void));


/* Interface for threads.

   A module that plans to do a blocking system call (or something else
   that lasts a long time and doesn't touch Python data) can allow other
   threads to run as follows:

	...preparations here...
	BGN_SAVE
	...blocking system call here...
	END_SAVE
	...interpretr result here...

   The BGN_SAVE/END_SAVE pair expands to a {}-surrounded block.
   To leave the block in the middle (e.g., with return), you must insert
   a line containing RET_SAVE before the return, e.g.

	if (...premature_exit...) {
		RET_SAVE
		err_errno(IOError);
		return NULL;
	}

   An alternative is:

	RET_SAVE
	if (...premature_exit...) {
		err_errno(IOError);
		return NULL;
	}
	RES_SAVE

   For convenience, that the value of 'errno' is restored across
   END_SAVE and RET_SAVE.

   WARNING: NEVER NEST CALLS TO BGN_SAVE AND END_SAVE!!!

   The function init_save_thread() should be called only from
   initthread() in "threadmodule.c".

   Note that not yet all candidates have been converted to use this
   mechanism!
*/

extern void init_save_thread PROTO((void));
extern object *save_thread PROTO((void));
extern void restore_thread PROTO((object *));

#ifdef USE_THREAD

#define BGN_SAVE { \
			object *_save; \
			_save = save_thread();
#define RET_SAVE	restore_thread(_save);
#define RES_SAVE	_save = save_thread();
#define END_SAVE	restore_thread(_save); \
		 }

#else /* !USE_THREAD */

#define BGN_SAVE {
#define RET_SAVE
#define RES_SAVE
#define END_SAVE }

#endif /* !USE_THREAD */

#ifdef __cplusplus
}
#endif
#endif /* !Py_CEVAL_H */
