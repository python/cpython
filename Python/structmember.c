/* Map C struct members to Python object attributes */

#include "allobjects.h"

#include "structmember.h"

object *
getmember(addr, mlist, name)
	char *addr;
	struct memberlist *mlist;
	char *name;
{
	struct memberlist *l;
	
	for (l = mlist; l->name != NULL; l++) {
		if (strcmp(l->name, name) == 0) {
			object *v;
			addr += l->offset;
			switch (l->type) {
			case T_SHORT:
				v = newintobject((long) *(short*)addr);
				break;
			case T_INT:
				v = newintobject((long) *(int*)addr);
				break;
			case T_LONG:
				v = newintobject(*(long*)addr);
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
	
	err_setstr(NameError, name);
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
				err_setstr(RuntimeError, "readonly attribute");
				return -1;
			}
			addr += l->offset;
			switch (l->type) {
			case T_SHORT:
				if (!is_intobject(v)) {
					err_badarg();
					return -1;
				}
				*(short*)addr = getintvalue(v);
				break;
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
			default:
				err_setstr(SystemError, "bad memberlist type");
				return -1;
			}
			return 0;
		}
	}
	
	err_setstr(NameError, name);
	return NULL;
}
