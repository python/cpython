/***********************************************************
    Written by:
	Fred Gansevles <Fred.Gansevles@cs.utwente.nl>
	Vakgroep Spa,
	Faculteit der Informatica,
	Universiteit Twente,
	Enschede,
	the Netherlands.
******************************************************************/

/* NIS module implementation */

#include "allobjects.h"
#include "modsupport.h"
#include "ceval.h"

#include <rpcsvc/ypclnt.h>
#include <sys/time.h>
#include <sys/types.h>
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>

static struct nis_map {
	char *alias;
	char *map;
} aliases [] = {
{"passwd", "passwd.byname"},
{"group", "group.byname"},
{"networks", "networks.byaddr"},
{"hosts", "hosts.byname"},
{"protocols", "protocols.bynumber"},
{"services", "services.byname"},
{"aliases", "mail.aliases"},
{"ethers", "ethers.byname"},
{0L, 0L}
};

static char *
nis_mapname (map)
char *map;
{
int i;

	for (i=0; aliases[i].alias != 0L; i++)
		if (!strcmp (aliases[i].alias, map))
			map = aliases[i].map;
	return map;
}

static int
nis_foreach (instatus, inkey, inkeylen, inval, invallen,indata)
int instatus;
char *inkey;
int inkeylen;
char *inval;
int invallen;
object *indata;
{
	if (instatus == YP_TRUE) {
		inkey[inkeylen]=0;
		inval[invallen]=0;
		dictinsert (indata, inkey, newstringobject (inval));
		return 0;
	}
	return 1;
}

static object *
nis_match (self, args)
object *self;
object *args;
{
char *match;
char *domain;
int len;
char *key, *map;

	if (!getstrstrarg(args, &key, &map))
		return NULL;
	map = nis_mapname (map);
	BGN_SAVE
	yp_get_default_domain(&domain);
	if (yp_match (domain, map, key, strlen (key), &match, &len) == 0)
		match[len] = 0;
	END_SAVE
	return newstringobject (match);
}

static object *
nis_cat (self, args)
object *self;
object *args;
{
char *domain;
char *map;
struct ypall_callback cb;
object *cat;

	if (!getstrarg(args, &map))
		return NULL;
	cat = newdictobject ();
	if (cat == NULL)
		return NULL;
	map = nis_mapname (map);
	cb.foreach = nis_foreach;
	cb.data = (char *)cat;
	yp_get_default_domain(&domain);
	BGN_SAVE
	yp_all (domain, map, &cb);
	END_SAVE
	return cat;
}

#define YPPROC_MAPLIST ((u_long)11)
#define YPPROG ((u_long)100004)
#define YPVERS ((u_long)2)

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
nis_xdr_domainname(xdrs, objp)
    XDR *xdrs;
    domainname *objp;
{
    if (!xdr_string(xdrs, objp, YPMAXDOMAIN)) {
        return (FALSE);
    }
    return (TRUE);
}

static
bool_t
nis_xdr_mapname(xdrs, objp)
    XDR *xdrs;
    mapname *objp;
{
    if (!xdr_string(xdrs, objp, YPMAXMAP)) {
        return (FALSE);
    }
    return (TRUE);
}

static
bool_t
nis_xdr_ypmaplist(xdrs, objp)
    XDR *xdrs;
    nismaplist *objp;
{
    if (!nis_xdr_mapname(xdrs, &objp->map)) {
        return (FALSE);
    }
    if (!xdr_pointer(xdrs, (char **)&objp->next, sizeof(nismaplist), nis_xdr_ypmaplist)) {
        return (FALSE);
    }
    return (TRUE);
}

static
bool_t
nis_xdr_ypstat(xdrs, objp)
    XDR *xdrs;
    nisstat *objp;
{
    if (!xdr_enum(xdrs, (enum_t *)objp)) {
        return (FALSE);
    }
    return (TRUE);
}


static
bool_t
nis_xdr_ypresp_maplist(xdrs, objp)
    XDR *xdrs;
    nisresp_maplist *objp;
{
    if (!nis_xdr_ypstat(xdrs, &objp->stat)) {
        return (FALSE);
    }
    if (!xdr_pointer(xdrs, (char **)&objp->maps, sizeof(nismaplist), nis_xdr_ypmaplist)) {
        return (FALSE);
    }
    return (TRUE);
}


static
nisresp_maplist *
nisproc_maplist_2(argp, clnt)
    domainname *argp;
    CLIENT *clnt;
{
    static nisresp_maplist res;

#ifdef hpux
    memset(&res, 0, sizeof(res));
#else  hpux
    memset(&res, sizeof(res));
#endif hpux
    if (clnt_call(clnt, YPPROC_MAPLIST, nis_xdr_domainname, argp, nis_xdr_ypresp_maplist
, &res, TIMEOUT) != RPC_SUCCESS) {
        return (NULL);
    }
    return (&res);
}

static
nismaplist *
nis_maplist ()
{
nisresp_maplist *list;
char *dom;
CLIENT *cl, *clnt_create();
char *server;

	yp_get_default_domain (&dom);
	yp_master (dom, aliases[0].map, &server);
	cl = clnt_create(server, YPPROG, YPVERS, "tcp");
	if (cl == NULL) {
		clnt_pcreateerror(server);
		return NULL;
	}
	list = nisproc_maplist_2 (&dom, cl);
	if (list == NULL)
		return NULL;
	if (list->stat != NIS_TRUE)
		return NULL;
	return list->maps;
}

static object *
nis_maps (self, args)
    object *self;
	object *args;
{
nismaplist *maps;
object *list;

	if ((maps = nis_maplist ()) == NULL)
		return NULL;
	if ((list = newlistobject(0)) == NULL)
		return NULL;
	BGN_SAVE
	for (maps = maps->next; maps; maps = maps->next)
		addlistitem (list, newstringobject (maps->map));
	END_SAVE
	return list;
}

static struct methodlist nis_methods[] = {
	{"match",	nis_match},
	{"cat",		nis_cat},
	{"maps",	nis_maps},
	{NULL,		NULL}		 /* Sentinel */
};

void
initnis ()
{
	(void) initmodule("nis", nis_methods);
}
