/* Traceback interface */

int tb_here PROTO((struct _frame *, int, int));
object *tb_fetch PROTO((void));
int tb_store PROTO((object *));
int tb_print PROTO((object *, FILE *));
