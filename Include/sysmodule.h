/* System module interface */

object *sysget PROTO((char *));
int sysset PROTO((char *, object *));
FILE *sysgetfile PROTO((char *, FILE *));
void initsys PROTO((void));
