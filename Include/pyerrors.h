/* Error handling definitions */

void err_set PROTO((object *));
void err_setval PROTO((object *, object *));
void err_setstr PROTO((object *, char *));
int err_occurred PROTO((void));
void err_get PROTO((object **, object **));
void err_clear PROTO((void));

/* Predefined exceptions */

extern object *RuntimeError;
extern object *EOFError;
extern object *TypeError;
extern object *MemoryError;
extern object *NameError;
extern object *SystemError;
extern object *KeyboardInterrupt;

/* Some more planned for the future */

#define IndexError		RuntimeError
#define KeyError		RuntimeError
#define ZeroDivisionError	RuntimeError
#define OverflowError		RuntimeError

/* Convenience functions */

extern int err_badarg PROTO((void));
extern object *err_nomem PROTO((void));
extern object *err_errno PROTO((object *));
extern void err_input PROTO((int));

extern void err_badcall PROTO((void));
