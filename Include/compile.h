/* Definitions for compiled intermediate code */


/* An intermediate code fragment contains:
   - a string that encodes the instructions,
   - a list of the constants,
   - and a list of the names used. */

typedef struct {
	OB_HEAD
	stringobject *co_code;	/* instruction opcodes */
	object *co_consts;	/* list of immutable constant objects */
	object *co_names;	/* list of stringobjects */
	object *co_filename;	/* string */
} codeobject;

extern typeobject Codetype;

#define is_codeobject(op) ((op)->ob_type == &Codetype)


/* Public interface */
codeobject *compile PROTO((struct _node *, char *));
