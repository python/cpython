/* Module definition and import interface */

void init_modules PROTO(());
void close_modules PROTO(());
object *new_module PROTO((char *name));
void define_module PROTO((struct _context *ctx, char *name));
object *import_module PROTO((struct _context *ctx, char *name));
