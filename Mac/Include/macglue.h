char *macstrerror PROTO((int));				/* strerror with mac errors */
object *PyErr_Mac PROTO((object *, int));	/* Exception with a mac error */
int PyMac_Idle PROTO((void));				/* Idle routine */