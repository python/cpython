/* Float object interface */

/*
floatobject represents a (double precision) floating point number.
*/

typedef struct {
	OB_HEAD
	double ob_fval;
} floatobject;

extern typeobject Floattype;

#define is_floatobject(op) ((op)->ob_type == &Floattype)

extern object *newfloatobject PROTO((double));
extern double getfloatvalue PROTO((object *));

/* Macro, trading safety for speed */
#define GETFLOATVALUE(op) ((op)->ob_fval)
