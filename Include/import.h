/* Module definition and import interface */

object *get_modules PROTO((void));
object *add_module PROTO((char *name));
object *import_module PROTO((char *name));
object *reload_module PROTO((object *m));
void doneimport PROTO((void));
