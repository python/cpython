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

/* Map C struct members to Python object attributes */

#include "allobjects.h"

#include "structmember.h"

static object *
listmembers(mlist)
	struct memberlist *mlist;
{
	int i, n;
	object *v;
	for (n = 0; mlist[n].name != NULL; n++)
		;
	v = newlistobject(n);
	if (v != NULL) {
		for (i = 0; i < n; i++)
			setlistitem(v, i, newstringobject(mlist[i].name));
		if (err_occurred()) {
			DECREF(v);
			v = NULL;
		}
		else {
			sortlist(v);
		}
	}
	return v;
}

object *
getmember(addr, mlist, name)
	char *addr;
	struct memberlist *mlist;
	char *name;
{
	struct memberlist *l;
	
	if (strcmp(name, "__members__") == 0)
		return listmembers(mlist);
	for (l = mlist; l->name != NULL; l++) {
		if (strcmp(l->name, name) == 0) {
			object *v;
			addr += l->offset;
			switch (l->type) {
			case T_BYTE:
				v = newintobject((long)
						 (((*(char*)addr & 0xff)
						   ^ 0x80) - 0x80));
				break;
			case T_UBYTE:
				v = newintobject((long) *(char*)addr & 0xff);
				break;
			case T_SHORT:
				v = newintobject((long) *(short*)addr);
				break;
			case T_USHORT:
				v = newintobject((long)
						 *(unsigned short*)addr);
				break;
			case T_INT:
				v = newintobject((long) *(int*)addr);
				break;
			case T_UINT:
				v = newintobject((long) *(unsigned int*)addr);
				break;
			case T_LONG:
				v = newintobject(*(long*)addr);
				break;
			case T_ULONG:
				v = dnewlongobject((double)
						   *(unsigned long*)addr);
				break;
			case T_FLOAT:
				v = newfloatobject((double)*(float*)addr);
				break;
			case T_DOUBLE:
				v = newfloatobject(*(double*)addr);
				break;
			case T_STRING:
				if (*(char**)addr == NULL) {
					INCREF(None);
					v = None;
				}
				else
					v = newstringobject(*(char**)addr);
				break;
			case T_CHAR:
				v = newsizedstringobject((char*)addr, 1);
				break;
			case T_OBJECT:
				v = *(object **)addr;
				if (v == NULL)
					v = None;
				INCREF(v);
				break;
			default:
				err_setstr(SystemError, "bad memberlist type");
				v = NULL;
			}
			return v;
		}
	}
	
	err_setstr(AttributeError, name);
	return NULL;
}

int
setmember(addr, mlist, name, v)
	char *addr;
	struct memberlist *mlist;
	char *name;
	object *v;
{
	struct memberlist *l;
	
	for (l = mlist; l->name != NULL; l++) {
		if (strcmp(l->name, name) == 0) {
			if (l->readonly || l->type == T_STRING) {
				err_setstr(TypeError, "readonly attribute");
				return -1;
			}
			addr += l->offset;
			switch (l->type) {
			case T_BYTE:
			case T_UBYTE:
				if (!is_intobject(v)) {
					err_badarg();
					return -1;
				}
				*(char*)addr = getintvalue(v);
				break;
			case T_SHORT:
			case T_USHORT:
				if (!is_intobject(v)) {
					err_badarg();
					return -1;
				}
				*(short*)addr = getintvalue(v);
				break;
			case T_UINT:
			case T_INT:
				if (!is_intobject(v)) {
					err_badarg();
					return -1;
				}
				*(int*)addr = getintvalue(v);
				break;
			case T_LONG:
				if (!is_intobject(v)) {
					err_badarg();
					return -1;
				}
				*(long*)addr = getintvalue(v);
				break;
			case T_ULONG:
				if (is_intobject(v))
					*(long*)addr = getintvalue(v);
				else if (is_longobject(v))
					*(long*)addr = getlongvalue(v);
				else {
					err_badarg();
					return -1;
				}
				break;
			case T_FLOAT:
				if (is_intobject(v))
					*(float*)addr = getintvalue(v);
				else if (is_floatobject(v))
					*(float*)addr = getfloatvalue(v);
				else {
					err_badarg();
					return -1;
				}
				break;
			case T_DOUBLE:
				if (is_intobject(v))
					*(double*)addr = getintvalue(v);
				else if (is_floatobject(v))
					*(double*)addr = getfloatvalue(v);
				else {
					err_badarg();
					return -1;
				}
				break;
			case T_OBJECT:
				XDECREF(*(object **)addr);
				XINCREF(v);
				*(object **)addr = v;
				break;
			case T_CHAR:
				if (is_stringobject(v) &&
				    getstringsize(v) == 1) {
					*(char*)addr =
						getstringvalue(v)[0];
				}
				else {
					err_badarg();
					return -1;
				}
			default:
				err_setstr(SystemError, "bad memberlist type");
				return -1;
			}
			return 0;
		}
	}
	
	err_setstr(AttributeError, name);
	return -1;
}
