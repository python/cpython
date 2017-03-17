Issue #9232: Modify Python's grammar to allow trailing commas in the
argument list of a function declaration.  For example, "def f(\*, a =
3,): pass" is now legal. Patch from Mark Dickinson.