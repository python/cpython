/* Module object interface */

extern typeobject Moduletype;

#define is_moduleobject(op) ((op)->ob_type == &Moduletype)

extern object *newmoduleobject PROTO((char *));
extern object *getmoduledict PROTO((object *));
extern char *getmodulename PROTO((object *));
