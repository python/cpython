/***********************************************************
Copyright 1991, 1992, 1993, 1994 by Stichting Mathematisch Centrum,
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

/* Thread module */
/* Interface to Sjoerd's portable C thread library */

#include "allobjects.h"
#include "modsupport.h"
#include "ceval.h"

#ifndef WITH_THREAD
Error!  The rest of Python is not compiled with thread support.
Rerun configure, adding a --with-thread option.
#endif

#include "thread.h"

extern int threads_started;

static object *ThreadError;


/* Lock objects */

typedef struct {
	OB_HEAD
	type_lock lock_lock;
} lockobject;

staticforward typeobject Locktype;

#define is_lockobject(v)		((v)->ob_type == &Locktype)

type_lock
getlocklock(lock)
	object *lock;
{
	if (lock == NULL || !is_lockobject(lock))
		return NULL;
	else
		return ((lockobject *) lock)->lock_lock;
}

static lockobject *
newlockobject()
{
	lockobject *self;
	self = NEWOBJ(lockobject, &Locktype);
	if (self == NULL)
		return NULL;
	self->lock_lock = allocate_lock();
	if (self->lock_lock == NULL) {
		DEL(self);
		self = NULL;
		err_setstr(ThreadError, "can't allocate lock");
	}
	return self;
}

static void
lock_dealloc(self)
	lockobject *self;
{
	/* Unlock the lock so it's safe to free it */
	acquire_lock(self->lock_lock, 0);
	release_lock(self->lock_lock);
	
	free_lock(self->lock_lock);
	DEL(self);
}

static object *
lock_acquire_lock(self, args)
	lockobject *self;
	object *args;
{
	int i;

	if (args != NULL) {
		if (!getargs(args, "i", &i))
			return NULL;
	}
	else
		i = 1;

	BGN_SAVE
	i = acquire_lock(self->lock_lock, i);
	END_SAVE

	if (args == NULL) {
		INCREF(None);
		return None;
	}
	else
		return newintobject((long)i);
}

static object *
lock_release_lock(self, args)
	lockobject *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;

	/* Sanity check: the lock must be locked */
	if (acquire_lock(self->lock_lock, 0)) {
		release_lock(self->lock_lock);
		err_setstr(ThreadError, "release unlocked lock");
		return NULL;
	}

	release_lock(self->lock_lock);
	INCREF(None);
	return None;
}

static object *
lock_locked_lock(self, args)
	lockobject *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;

	if (acquire_lock(self->lock_lock, 0)) {
		release_lock(self->lock_lock);
		return newintobject(0L);
	}
	return newintobject(1L);
}

static struct methodlist lock_methods[] = {
	{"acquire_lock",	(method)lock_acquire_lock},
	{"acquire",		(method)lock_acquire_lock},
	{"release_lock",	(method)lock_release_lock},
	{"release",		(method)lock_release_lock},
	{"locked_lock",		(method)lock_locked_lock},
	{"locked",		(method)lock_locked_lock},
	{NULL,			NULL}		/* sentinel */
};

static object *
lock_getattr(self, name)
	lockobject *self;
	char *name;
{
	return findmethod(lock_methods, (object *)self, name);
}

static typeobject Locktype = {
	OB_HEAD_INIT(&Typetype)
	0,				/*ob_size*/
	"lock",				/*tp_name*/
	sizeof(lockobject),		/*tp_size*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)lock_dealloc,	/*tp_dealloc*/
	0,				/*tp_print*/
	(getattrfunc)lock_getattr,	/*tp_getattr*/
	0,				/*tp_setattr*/
	0,				/*tp_compare*/
	0,				/*tp_repr*/
};


/* Module functions */

static void
t_bootstrap(args_raw)
	void *args_raw;
{
	object *args = (object *) args_raw;
	object *func, *arg, *res;

	threads_started++;

	restore_thread((void *)NULL);
	func = gettupleitem(args, 0);
	arg = gettupleitem(args, 1);
	res = call_object(func, arg);
	DECREF(args); /* Matches the INCREF(args) in thread_start_new_thread */
	if (res == NULL) {
		fprintf(stderr, "Unhandled exception in thread:\n");
		print_error(); /* From pythonmain.c */
	}
	else
		DECREF(res);
	(void) save_thread(); /* Should always be NULL */
	exit_thread();
}

static object *
thread_start_new_thread(self, args)
	object *self; /* Not used */
	object *args;
{
	object *func, *arg;

	if (!getargs(args, "(OO)", &func, &arg))
		return NULL;
	INCREF(args);
	/* Initialize the interpreter's stack save/restore mechanism */
	init_save_thread();
	if (!start_new_thread(t_bootstrap, (void*) args)) {
		DECREF(args);
		err_setstr(ThreadError, "can't start new thread\n");
		return NULL;
	}
	/* Otherwise the DECREF(args) is done by t_bootstrap */
	INCREF(None);
	return None;
}

static object *
thread_exit_thread(self, args)
	object *self; /* Not used */
	object *args;
{
	object *frame;
	if (!getnoarg(args))
		return NULL;
	frame = save_thread(); /* Should never be NULL */
	DECREF(frame);
	exit_thread();
	for (;;) { } /* Should not be reached */
}

#ifndef NO_EXIT_PROG
static object *
thread_exit_prog(self, args)
	object *self; /* Not used */
	object *args;
{
	int sts;
	if (!getargs(args, "i", &sts))
		return NULL;
	goaway(sts); /* Calls exit_prog(sts) or _exit_prog(sts) */
	for (;;) { } /* Should not be reached */
}
#endif

static object *
thread_allocate_lock(self, args)
	object *self; /* Not used */
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	return (object *) newlockobject();
}

static object *
thread_get_ident(self, args)
	object *self; /* Not used */
	object *args;
{
	long ident;
	if (!getnoarg(args))
		return NULL;
	ident = get_thread_ident();
	if (ident == -1) {
		err_setstr(ThreadError, "no current thread ident");
		return NULL;
	}
	return newintobject(ident);
}

static struct methodlist thread_methods[] = {
	{"start_new_thread",	(method)thread_start_new_thread},
	{"start_new",		(method)thread_start_new_thread},
	{"allocate_lock",	(method)thread_allocate_lock},
	{"allocate",		(method)thread_allocate_lock},
	{"exit_thread",		(method)thread_exit_thread},
	{"exit",		(method)thread_exit_thread},
	{"get_ident",		(method)thread_get_ident},
#ifndef NO_EXIT_PROG
	{"exit_prog",		(method)thread_exit_prog},
#endif
	{NULL,			NULL}		/* sentinel */
};


/* Initialization function */

void
initthread()
{
	object *m, *d, *x;

	/* Create the module and add the functions */
	m = initmodule("thread", thread_methods);

	/* Add a symbolic constant */
	d = getmoduledict(m);
	ThreadError = newstringobject("thread.error");
	INCREF(ThreadError);
	dictinsert(d, "error", ThreadError);

	/* Check for errors */
	if (err_occurred())
		fatal("can't initialize module thread");

	/* Initialize the C thread library */
	init_thread();
}
