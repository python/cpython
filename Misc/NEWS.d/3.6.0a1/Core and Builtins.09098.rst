Issue #26718: super.__init__ no longer leaks memory if called multiple times.
NOTE: A direct call of super.__init__ is not endorsed!