/* File object interface */

extern typeobject Filetype;

#define is_fileobject(op) ((op)->ob_type == &Filetype)

extern object *newfileobject PROTO((char *, char *));
extern object *newopenfileobject PROTO((FILE *, char *, char *));
extern FILE *getfilefile PROTO((object *));
