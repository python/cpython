"""
atexit.py - allow programmer to define multiple exit functions to be executed
upon normal program termination.

One public function, register, is defined.  
"""

_exithandlers = []
def _run_exitfuncs():
    """run any registered exit functions

    _exithandlers is traversed in reverse order so functions are executed
    last in, first out.
    """
    
    while _exithandlers:
        func, targs, kargs = _exithandlers.pop()
        apply(func, targs, kargs)

def register(func, *targs, **kargs):
    """register a function to be executed upon normal program termination

    func - function to be called at exit
    targs - optional arguments to pass to func
    kargs - optional keyword arguments to pass to func
    """
    _exithandlers.append((func, targs, kargs))

import sys
try:
    x = sys.exitfunc
except AttributeError:
    sys.exitfunc = _run_exitfuncs
else:
    # if x isn't our own exit func executive, assume it's another
    # registered exit function - append it to our list...
    if x != _run_exitfuncs:
        register(x)
del sys

if __name__ == "__main__":
    def x1():
        print "running x1"
    def x2(n):
        print "running x2(%s)" % `n`
    def x3(n, kwd=None):
        print "running x3(%s, kwd=%s)" % (`n`, `kwd`)

    register(x1)
    register(x2, 12)
    register(x3, 5, "bar")
    register(x3, "no kwd args")

