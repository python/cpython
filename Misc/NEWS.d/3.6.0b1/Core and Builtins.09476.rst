Issue #23722: The __class__ cell used by zero-argument super() is now
initialized from type.__new__ rather than __build_class__, so class methods
relying on that will now work correctly when called from metaclass methods
during class creation. Patch by Martin Teichmann.