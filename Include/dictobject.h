/*
Dictionary object type -- mapping from char * to object.
NB: the key is given as a char *, not as a stringobject.
These functions set errno for errors.  Functions dictremove() and
dictinsert() return nonzero for errors, getdictsize() returns -1,
the others NULL.  A successful call to dictinsert() calls INCREF()
for the inserted item.
*/

extern typeobject Dicttype;

#define is_dictobject(op) ((op)->ob_type == &Dicttype)

extern object *newdictobject PROTO((void));
extern object *dictlookup PROTO((object *dp, char *key));
extern int dictinsert PROTO((object *dp, char *key, object *item));
extern int dictremove PROTO((object *dp, char *key));
extern int getdictsize PROTO((object *dp));
extern char *getdictkey PROTO((object *dp, int i));
