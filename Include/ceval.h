/* Interface to execute compiled code */
/* This header depends on "compile.h" */

object *eval_code PROTO((codeobject *, object *, object *, object *));

object *getglobals PROTO((void));
object *getlocals PROTO((void));

void printtraceback PROTO((FILE *));
