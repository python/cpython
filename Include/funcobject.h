/* Function object interface */

extern typeobject Functype;

#define is_funcobject(op) ((op)->ob_type == &Functype)

extern object *newfuncobject PROTO((object *, object *));
extern object *getfunccode PROTO((object *));
extern object *getfuncglobals PROTO((object *));
