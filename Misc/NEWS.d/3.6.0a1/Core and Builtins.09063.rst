Issue #25421: __sizeof__ methods of builtin types now use dynamic basic size.
This allows sys.getsize() to work correctly with their subclasses with
__slots__ defined.