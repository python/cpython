/* List object interface */

/*
123456789-123456789-123456789-123456789-123456789-123456789-123456789-12

Another generally useful object type is an list of object pointers.
This is a mutable type: the list items can be changed, and items can be
added or removed.  Out-of-range indices or non-list objects are ignored.

*** WARNING *** setlistitem does not increment the new item's reference
count, but does decrement the reference count of the item it replaces,
if not nil.  It does *decrement* the reference count if it is *not*
inserted in the list.  Similarly, getlistitem does not increment the
returned item's reference count.
*/

typedef struct {
	OB_VARHEAD
	object **ob_item;
} listobject;

extern typeobject Listtype;

#define is_listobject(op) ((op)->ob_type == &Listtype)

extern object *newlistobject PROTO((int size));
extern int getlistsize PROTO((object *));
extern object *getlistitem PROTO((object *, int));
extern int setlistitem PROTO((object *, int, object *));
extern int inslistitem PROTO((object *, int, object *));
extern int addlistitem PROTO((object *, object *));
extern int sortlist PROTO((object *));

/* Macro, trading safety for speed */
#define GETLISTITEM(op, i) ((op)->ob_item[i])
