import sys
from collections import deque


class nested(object):
    def __init__(self, *contexts):
        self.contexts = contexts
        self.entered = None

    def __context__(self):
        return self

    def __enter__(self):
        if self.entered is not None:
            raise RuntimeError("Context is not reentrant")
        self.entered = deque()
        vars = []
        try:
            for context in self.contexts:
                mgr = context.__context__()
                vars.append(mgr.__enter__())
                self.entered.appendleft(mgr)
        except:
            self.__exit__(*sys.exc_info())
            raise
        return vars

    def __exit__(self, *exc_info):
        # Behave like nested with statements
        # first in, last out
        # New exceptions override old ones
        ex = exc_info
        for mgr in self.entered:
            try:
                mgr.__exit__(*ex)
            except:
                ex = sys.exc_info()
        self.entered = None
        if ex is not exc_info:
            raise ex[0], ex[1], ex[2]

