/* Module definition and import interface */

object *new_module PROTO((char *name));
object *import_module PROTO((struct _context *ctx, char *name));
object *reload_module PROTO((struct _context *ctx, object *m));
