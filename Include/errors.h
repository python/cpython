/* Error handling definitions */

void err_set PROTO((object *));
void err_setval PROTO((object *, object *));
void err_setstr PROTO((object *, char *));
int err_occurred PROTO((void));
void err_get PROTO((object **, object **));
void err_clear PROTO((void));

/* Predefined exceptions (in run.c) */

extern object *RuntimeError;
extern object *EOFError;
extern object *TypeError;
extern object *MemoryError;
extern object *NameError;
extern object *SystemError;
extern object *KeyboardInterrupt;

/* Convenience functions */

extern int err_badarg PROTO((void));
extern object *err_nomem PROTO((void));
extern object *err_errno PROTO((object *));
