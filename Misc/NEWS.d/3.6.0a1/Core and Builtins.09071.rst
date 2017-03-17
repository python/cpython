Issue #26154: Add a new private _PyThreadState_UncheckedGet() function to get
the current Python thread state, but don't issue a fatal error if it is NULL.
This new function must be used instead of accessing directly the
_PyThreadState_Current variable.  The variable is no more exposed since
Python 3.5.1 to hide the exact implementation of atomic C types, to avoid
compiler issues.