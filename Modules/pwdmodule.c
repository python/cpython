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
	return mkvalue("(ssllsss)",
		       p->pw_name,
		       p->pw_passwd,
		       (long)p->pw_uid,
		       (long)p->pw_gid,
		       p->pw_gecos,
		       p->pw_dir,
		       p->pw_shell);
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
	char *name;
	struct passwd *p;
	if (!getstrarg(args, &name))
		return NULL;
	if ((p = getpwnam(name)) == NULL) {
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
	if ((w = newlistobject(0)) == NULL) {
		return NULL;
	}
	for (member = p->gr_mem; *member != NULL; member++) {
		object *x = newstringobject(*member);
		if (x == NULL || addlistitem(w, x) != 0) {
			XDECREF(x);
			DECREF(w);
			return NULL;
		}
	}
	v = mkvalue("(sslO)",
		       p->gr_name,
		       p->gr_passwd,
		       (long)p->gr_gid,
		       w);
	DECREF(w);
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
	char *name;
	struct group *p;
	if (!getstrarg(args, &name))
		return NULL;
	if ((p = getgrnam(name)) == NULL) {
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
