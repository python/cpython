/* Definitions used by cgen output */

typedef char *string;

#define mknewlongobject(x) newintobject(x)
#define mknewshortobject(x) newintobject((long)x)
#define mknewfloatobject(x) newfloatobject(x)

extern object *mknewcharobject PROTO((int c));

extern int getiobjectarg PROTO((object *args, int nargs, int i, object **p_a));
extern int getilongarg PROTO((object *args, int nargs, int i, long *p_a));
extern int getishortarg PROTO((object *args, int nargs, int i, short *p_a));
extern int getifloatarg PROTO((object *args, int nargs, int i, float *p_a));
extern int getistringarg PROTO((object *args, int nargs, int i, string *p_a));
