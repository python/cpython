/* Interfaces to parse and execute pieces of python code */

void initall PROTO((void));

int run PROTO((FILE *, char *));

int run_script PROTO((FILE *, char *));
int run_tty_1 PROTO((FILE *, char *));
int run_tty_loop PROTO((FILE *, char *));

int parse_string PROTO((char *, int, struct _node **));
int parse_file PROTO((FILE *, char *, int, struct _node **));

object *eval_node PROTO((struct _node *, char *, object *, object *));

object *run_string PROTO((char *, int, object *, object *));
object *run_file PROTO((FILE *, char *, int, object *, object *));
object *run_err_node PROTO((int, struct _node *, char *, object *, object *));
object *run_node PROTO((struct _node *, char *, object *, object *));

void print_error PROTO((void));

void goaway PROTO((int));
