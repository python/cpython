/***********************************************************
Copyright 1991 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

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

/* Passwd/group file access module */

#include "allobjects.h"
#include "modsupport.h"

#include <sys/types.h>
#include <pwd.h>
#include <grp.h>


/* Module pwd */


static object *mkpwent(p)
	struct passwd *p;
{
	object *v;
	if ((v = newtupleobject(7)) == NULL)
		return NULL;
#define ISET(i, member) settupleitem(v, i, newintobject((long)p->member))
#define SSET(i, member) settupleitem(v, i, newstringobject(p->member))
	SSET(0, pw_name);
	SSET(1, pw_passwd);
	ISET(2, pw_uid);
	ISET(3, pw_gid);
	SSET(4, pw_gecos);
	SSET(5, pw_dir);
	SSET(6, pw_shell);
#undef SSET
#undef ISET
	if (err_occurred()) {
		DECREF(v);
		return NULL;
	}
	return v;
}

static object *pwd_getpwuid(self, args)
	object *self, *args;
{
	int uid;
	struct passwd *p;
	if (!getintarg(args, &uid))
		return NULL;
	if ((p = getpwuid(uid)) == NULL) {
		err_setstr(KeyError, "getpwuid(): uid not found");
		return NULL;
	}
	return mkpwent(p);
}

static object *pwd_getpwnam(self, args)
	object *self, *args;
{
	object *name;
	struct passwd *p;
	if (!getstrarg(args, &name))
		return NULL;
	if ((p = getpwnam(getstringvalue(name))) == NULL) {
		err_setstr(KeyError, "getpwnam(): name not found");
		return NULL;
	}
	return mkpwent(p);
}

static object *pwd_getpwall(self, args)
	object *self, *args;
{
	object *d;
	struct passwd *p;
	if (!getnoarg(args))
		return NULL;
	if ((d = newlistobject(0)) == NULL)
		return NULL;
	setpwent();
	while ((p = getpwent()) != NULL) {
		object *v = mkpwent(p);
		if (v == NULL || addlistitem(d, v) != 0) {
			XDECREF(v);
			DECREF(d);
			return NULL;
		}
	}
	return d;
}

static struct methodlist pwd_methods[] = {
	{"getpwuid",	pwd_getpwuid},
	{"getpwnam",	pwd_getpwnam},
	{"getpwall",	pwd_getpwall},
	{NULL,		NULL}		/* sentinel */
};

void
initpwd()
{
	initmodule("pwd", pwd_methods);
}


/* Module grp */


static object *mkgrent(p)
	struct group *p;
{
	object *v, *w;
	char **member;
	if ((v = newtupleobject(4)) == NULL)
		return NULL;
#define ISET(i, member) settupleitem(v, i, newintobject((long)p->member))
#define SSET(i, member) settupleitem(v, i, newstringobject(p->member))
	SSET(0, gr_name);
	SSET(1, gr_passwd);
	ISET(2, gr_gid);
#undef SSET
#undef ISET
	if (err_occurred()) {
		DECREF(v);
		return NULL;
	}
	if ((w = newlistobject(0)) == NULL) {
		DECREF(v);
		return NULL;
	}
	(void) settupleitem(v, 3, w); /* Cannot fail; eats refcnt */
	for (member = p->gr_mem; *member != NULL; member++) {
		object *x = newstringobject(*member);
		if (x == NULL || addlistitem(w, x) != 0) {
			XDECREF(x);
			DECREF(v);
			return NULL;
		}
	}
	return v;
}

static object *grp_getgrgid(self, args)
	object *self, *args;
{
	int gid;
	struct group *p;
	if (!getintarg(args, &gid))
		return NULL;
	if ((p = getgrgid(gid)) == NULL) {
		err_setstr(KeyError, "getgrgid(): gid not found");
		return NULL;
	}
	return mkgrent(p);
}

static object *grp_getgrnam(self, args)
	object *self, *args;
{
	object *name;
	struct group *p;
	if (!getstrarg(args, &name))
		return NULL;
	if ((p = getgrnam(getstringvalue(name))) == NULL) {
		err_setstr(KeyError, "getgrnam(): name not found");
		return NULL;
	}
	return mkgrent(p);
}

static object *grp_getgrall(self, args)
	object *self, *args;
{
	object *d;
	struct group *p;
	if (!getnoarg(args))
		return NULL;
	if ((d = newlistobject(0)) == NULL)
		return NULL;
	setgrent();
	while ((p = getgrent()) != NULL) {
		object *v = mkgrent(p);
		if (v == NULL || addlistitem(d, v) != 0) {
			XDECREF(v);
			DECREF(d);
			return NULL;
		}
	}
	return d;
}

static struct methodlist grp_methods[] = {
	{"getgrgid",	grp_getgrgid},
	{"getgrnam",	grp_getgrnam},
	{"getgrall",	grp_getgrall},
	{NULL,		NULL}		/* sentinel */
};

void
initgrp()
{
	initmodule("grp", grp_methods);
}
