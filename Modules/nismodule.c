/***********************************************************
    Written by:
	Fred Gansevles <Fred.Gansevles@cs.utwente.nl>
	B&O group,
	Faculteit der Informatica,
	Universiteit Twente,
	Enschede,
	the Netherlands.
******************************************************************/

/* NIS module implementation */

#include "Python.h"

#include <sys/time.h>
#include <sys/types.h>
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

#ifdef __sgi
/* This is missing from rpcsvc/ypclnt.h */
extern int yp_get_default_domain(char **);
#endif

static PyObject *NisError;

static PyObject *
nis_error (int err)
{
	PyErr_SetString(NisError, yperr_string(err));
	return NULL;
}

static struct nis_map {
	char *alias;
	char *map;
	int  fix;
} aliases [] = {
	{"passwd",	"passwd.byname",	0},
	{"group",	"group.byname",		0},
	{"networks",	"networks.byaddr",	0},
	{"hosts",	"hosts.byname",		0},
	{"protocols",	"protocols.bynumber",	0},
	{"services",	"services.byname",	0},
	{"aliases",	"mail.aliases",		1}, /* created with 'makedbm -a' */
	{"ethers",	"ethers.byname",	0},
	{0L,		0L,			0}
};

static char *
nis_mapname (char *map, int *pfix)
{
	int i;

	*pfix = 0;
	for (i=0; aliases[i].alias != 0L; i++) {
		if (!strcmp (aliases[i].alias, map)) {
			*pfix = aliases[i].fix;
			return aliases[i].map;
		}
		if (!strcmp (aliases[i].map, map)) {
			*pfix = aliases[i].fix;
			return aliases[i].map;
		}
	}

	return map;
}

typedef int (*foreachfunc)(int, char *, int, char *, int, char *);

struct ypcallback_data {
	PyObject	*dict;
	int			fix;
};

static int
nis_foreach (int instatus, char *inkey, int inkeylen, char *inval,
             int invallen, struct ypcallback_data *indata)
{
	if (instatus == YP_TRUE) {
		PyObject *key;
		PyObject *val;
		int err;

		if (indata->fix) {
		    inkeylen--;
		    invallen--;
		}
		key = PyString_FromStringAndSize(inkey, inkeylen);
		val = PyString_FromStringAndSize(inval, invallen);
		if (key == NULL || val == NULL) {
			/* XXX error -- don't know how to handle */
			PyErr_Clear();
			Py_XDECREF(key);
			Py_XDECREF(val);
			return 1;
		}
		err = PyDict_SetItem(indata->dict, key, val);
		Py_DECREF(key);
		Py_DECREF(val);
		if (err != 0) {
			PyErr_Clear();
			return 1;
		}
		return 0;
	}
	return 1;
}

static PyObject *
nis_match (PyObject *self, PyObject *args)
{
	char *match;
	char *domain;
	int keylen, len;
	char *key, *map;
	int err;
	PyObject *res;
	int fix;

	if (!PyArg_Parse(args, "(t#s)", &key, &keylen, &map))
		return NULL;
	if ((err = yp_get_default_domain(&domain)) != 0)
		return nis_error(err);
	map = nis_mapname (map, &fix);
	if (fix)
	    keylen++;
	Py_BEGIN_ALLOW_THREADS
	err = yp_match (domain, map, key, keylen, &match, &len);
	Py_END_ALLOW_THREADS
	if (fix)
	    len--;
	if (err != 0)
		return nis_error(err);
	res = PyString_FromStringAndSize (match, len);
	free (match);
	return res;
}

static PyObject *
nis_cat (PyObject *self, PyObject *args)
{
	char *domain;
	char *map;
	struct ypall_callback cb;
	struct ypcallback_data data;
	PyObject *dict;
	int err;

	if (!PyArg_Parse(args, "s", &map))
		return NULL;
	if ((err = yp_get_default_domain(&domain)) != 0)
		return nis_error(err);
	dict = PyDict_New ();
	if (dict == NULL)
		return NULL;
	cb.foreach = (foreachfunc)nis_foreach;
	data.dict = dict;
	map = nis_mapname (map, &data.fix);
	cb.data = (char *)&data;
	Py_BEGIN_ALLOW_THREADS
	err = yp_all (domain, map, &cb);
	Py_END_ALLOW_THREADS
	if (err != 0) {
		Py_DECREF(dict);
		return nis_error(err);
	}
	return dict;
}

/* These should be u_long on Sun h/w but not on 64-bit h/w.
   This is not portable to machines with 16-bit ints and no prototypes */
#ifndef YPPROC_MAPLIST
#define YPPROC_MAPLIST	11
#endif
#ifndef YPPROG
#define YPPROG		100004
#endif
#ifndef YPVERS
#define YPVERS		2
#endif

typedef char *domainname;
typedef char *mapname;

enum nisstat {
	NIS_TRUE = 1,
	NIS_NOMORE = 2,
	NIS_FALSE = 0,
	NIS_NOMAP = -1,
	NIS_NODOM = -2,
	NIS_NOKEY = -3,
	NIS_BADOP = -4,
	NIS_BADDB = -5,
	NIS_YPERR = -6,
	NIS_BADARGS = -7,
	NIS_VERS = -8
};
typedef enum nisstat nisstat;

struct nismaplist {
	mapname map;
	struct nismaplist *next;
};
typedef struct nismaplist nismaplist;

struct nisresp_maplist {
	nisstat stat;
	nismaplist *maps;
};
typedef struct nisresp_maplist nisresp_maplist;

static struct timeval TIMEOUT = { 25, 0 };

static
bool_t
nis_xdr_domainname(XDR *xdrs, domainname *objp)
{
	if (!xdr_string(xdrs, objp, YPMAXDOMAIN)) {
		return (FALSE);
	}
	return (TRUE);
}

static
bool_t
nis_xdr_mapname(XDR *xdrs, mapname *objp)
{
	if (!xdr_string(xdrs, objp, YPMAXMAP)) {
		return (FALSE);
	}
	return (TRUE);
}

static
bool_t
nis_xdr_ypmaplist(XDR *xdrs, nismaplist *objp)
{
	if (!nis_xdr_mapname(xdrs, &objp->map)) {
		return (FALSE);
	}
	if (!xdr_pointer(xdrs, (char **)&objp->next,
			 sizeof(nismaplist), (xdrproc_t)nis_xdr_ypmaplist))
	{
		return (FALSE);
	}
	return (TRUE);
}

static
bool_t
nis_xdr_ypstat(XDR *xdrs, nisstat *objp)
{
	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}


static
bool_t
nis_xdr_ypresp_maplist(XDR *xdrs, nisresp_maplist *objp)
{
	if (!nis_xdr_ypstat(xdrs, &objp->stat)) {
		return (FALSE);
	}
	if (!xdr_pointer(xdrs, (char **)&objp->maps,
			 sizeof(nismaplist), (xdrproc_t)nis_xdr_ypmaplist))
	{
		return (FALSE);
	}
	return (TRUE);
}


static
nisresp_maplist *
nisproc_maplist_2(domainname *argp, CLIENT *clnt)
{
	static nisresp_maplist res;

	memset(&res, 0, sizeof(res));
	if (clnt_call(clnt, YPPROC_MAPLIST,
		      (xdrproc_t)nis_xdr_domainname, (caddr_t)argp,
		      (xdrproc_t)nis_xdr_ypresp_maplist, (caddr_t)&res,
		      TIMEOUT) != RPC_SUCCESS)
	{
		return (NULL);
	}
	return (&res);
}

static
nismaplist *
nis_maplist (void)
{
	nisresp_maplist *list;
	char *dom;
	CLIENT *cl, *clnt_create();
	char *server = NULL;
	int mapi = 0;
        int err;

	if ((err = yp_get_default_domain (&dom)) != 0) {
		nis_error(err);
		return NULL;
	}

	while (!server && aliases[mapi].map != 0L) {
		yp_master (dom, aliases[mapi].map, &server);
		mapi++;
	}
        if (!server) {
            PyErr_SetString(NisError, "No NIS master found for any map");
            return NULL;
        }
	cl = clnt_create(server, YPPROG, YPVERS, "tcp");
	if (cl == NULL) {
		PyErr_SetString(NisError, clnt_spcreateerror(server));
		goto finally;
	}
	list = nisproc_maplist_2 (&dom, cl);
	clnt_destroy(cl);
	if (list == NULL)
		goto finally;
	if (list->stat != NIS_TRUE)
		goto finally;

	free(server);
	return list->maps;

  finally:
	free(server);
	return NULL;
}

static PyObject *
nis_maps (PyObject *self, PyObject *args)
{
	nismaplist *maps;
	PyObject *list;

        if (!PyArg_NoArgs(args))
		return NULL;
	if ((maps = nis_maplist ()) == NULL)
		return NULL;
	if ((list = PyList_New(0)) == NULL)
		return NULL;
	for (maps = maps; maps; maps = maps->next) {
		PyObject *str = PyString_FromString(maps->map);
		if (!str || PyList_Append(list, str) < 0)
		{
			Py_DECREF(list);
			list = NULL;
			break;
		}
		Py_DECREF(str);
	}
	/* XXX Shouldn't we free the list of maps now? */
	return list;
}

static PyMethodDef nis_methods[] = {
	{"match",	nis_match},
	{"cat",		nis_cat},
	{"maps",	nis_maps},
	{NULL,		NULL}		 /* Sentinel */
};

void
initnis (void)
{
	PyObject *m, *d;
	m = Py_InitModule("nis", nis_methods);
	d = PyModule_GetDict(m);
	NisError = PyErr_NewException("nis.error", NULL, NULL);
	if (NisError != NULL)
		PyDict_SetItemString(d, "error", NisError);
}
