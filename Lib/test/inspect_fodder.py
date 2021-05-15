# line 1
'A module docstring.'

import sys, inspect
# line 5

# line 7
def spam(a, /, b, c, d=3, e=4, f=5, *g, **h):
    eggs(b + d, c + f)

# line 11
def eggs(x, y):
    "A docstring."
    global fr, st
    fr = inspect.currentframe()
    st = inspect.stack()
    p = x
    q = y / 0

# line 20
class StupidGit:
    """A longer,

    indented

    docstring."""
# line 27

    def abuse(self, a, b, c):
        """Another

\tdocstring

        containing

\ttabs
\t
        """
        self.argue(a, b, c)
# line 40
    def argue(self, a, b, c):
        try:
            spam(a, b, c)
        except:
            self.ex = sys.exc_info()
            self.tr = inspect.trace()

    @property
    def contradiction(self):
        'The automatic gainsaying.'
        pass

# line 53
class MalodorousPervert(StupidGit):
    def abuse(self, a, b, c):
        pass

    @property
    def contradiction(self):
        pass

Tit = MalodorousPervert

class ParrotDroppings:
    pass

class FesteringGob(MalodorousPervert, ParrotDroppings):
    def abuse(self, a, b, c):
        pass

    @property
    def contradiction(self):
        pass

async def lobbest(grenade):
    pass

currentframe = inspect.currentframe()
try:
    raise Exception()
except:
    tb = sys.exc_info()[2]

class Callable:
    def __call__(self, *args):
        return args

    def as_method_of(self, obj):
        from types import MethodType
        return MethodType(self, obj)

custom_method = Callable().as_method_of(42)
del Callable

# line 95
class WhichComments:
  # line 97
    # before f
    def f(self):
      # line 100
        # start f
        return 1
        # line 103
        # end f
       # line 105
    # after f

    # before asyncf - line 108
    async def asyncf(self):
        # start asyncf
        return 2
        # end asyncf
       # after asyncf - line 113
    # end of WhichComments - line 114
  # after WhichComments - line 115
