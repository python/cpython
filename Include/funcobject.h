/* Function object interface */

extern typeobject Functype;

#define is_funcobject(op) ((op)->ob_type == &Functype)

extern object *newfuncobject PROTO((node *, object *));
extern node *getfuncnode PROTO((object *));
extern object *getfuncglobals PROTO((object *));
