/* Class object interface */

/*
Classes are really hacked in at the last moment.
It should be possible to use other object types as base classes,
but currently it isn't.  We'll see if we can fix that later, sigh...
*/

extern typeobject Classtype, Classmembertype, Classmethodtype;

#define is_classobject(op) ((op)->ob_type == &Classtype)
#define is_classmemberobject(op) ((op)->ob_type == &Classmembertype)
#define is_classmethodobject(op) ((op)->ob_type == &Classmethodtype)

extern object *newclassobject PROTO((node *, object *, object *));
extern object *newclassmemberobject PROTO((object *));
extern object *newclassmethodobject PROTO((object *, object *));

extern object *classmethodgetfunc PROTO((object *));
extern object *classmethodgetself PROTO((object *));
