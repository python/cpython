/* Error handling definitions */

void err_set PROTO((object *));
void err_setval PROTO((object *, object *));
void err_setstr PROTO((object *, char *));
int err_occurred PROTO((void));
void err_get PROTO((object **, object **));
void err_clear PROTO((void));

/* Predefined exceptions (in run.c) */
object *RuntimeError;		/* Raised by error() */
object *EOFError;		/* Raised by eof_error() */
object *TypeError;		/* Rased by type_error() */
object *MemoryError;		/* Raised by mem_error() */
object *NameError;		/* Raised by name_error() */
object *SystemError;		/* Raised by sys_error() */
object *KeyboardInterrupt;	/* Raised by intr_error() */
