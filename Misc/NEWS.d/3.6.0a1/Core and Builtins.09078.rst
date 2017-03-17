Issue #26204: The compiler now ignores all constant statements: bytes, str,
int, float, complex, name constants (None, False, True), Ellipsis
and ast.Constant; not only str and int. For example, ``1.0`` is now ignored
in ``def f(): 1.0``.