"""Test module for the custom examples

Custom 1:

>>> import custom
>>> c1 = custom.Custom()
>>> c2 = custom.Custom()
>>> del c1
>>> del c2


Custom 2

>>> import custom2
>>> c1 = custom2.Custom('jim', 'fulton', 42)
>>> c1.first
'jim'
>>> c1.last
'fulton'
>>> c1.number
42
>>> c1.name()
'jim fulton'
>>> c1.first = 'will'
>>> c1.name()
'will fulton'
>>> c1.last = 'tell'
>>> c1.name()
'will tell'
>>> del c1.first
>>> c1.name()
Traceback (most recent call last):
...
AttributeError: first
>>> c1.first
Traceback (most recent call last):
...
AttributeError: first
>>> c1.first = 'drew'
>>> c1.first
'drew'
>>> del c1.number
Traceback (most recent call last):
...
TypeError: can't delete numeric/char attribute
>>> c1.number=2
>>> c1.number
2
>>> c1.first = 42
>>> c1.name()
'42 tell'
>>> c2 = custom2.Custom()
>>> c2.name()
' '
>>> c2.first
''
>>> c2.last
''
>>> del c2.first
>>> c2.first
Traceback (most recent call last):
...
AttributeError: first
>>> c2.first
Traceback (most recent call last):
...
AttributeError: first
>>> c2.name()
Traceback (most recent call last):
  File "<stdin>", line 1, in ?
AttributeError: first
>>> c2.number
0
>>> n3 = custom2.Custom('jim', 'fulton', 'waaa')
Traceback (most recent call last):
  File "<stdin>", line 1, in ?
TypeError: an integer is required (got type str)
>>> del c1
>>> del c2


Custom 3

>>> import custom3
>>> c1 = custom3.Custom('jim', 'fulton', 42)
>>> c1 = custom3.Custom('jim', 'fulton', 42)
>>> c1.name()
'jim fulton'
>>> del c1.first
Traceback (most recent call last):
  File "<stdin>", line 1, in ?
TypeError: Cannot delete the first attribute
>>> c1.first = 42
Traceback (most recent call last):
  File "<stdin>", line 1, in ?
TypeError: The first attribute value must be a string
>>> c1.first = 'will'
>>> c1.name()
'will fulton'
>>> c2 = custom3.Custom()
>>> c2 = custom3.Custom()
>>> c2 = custom3.Custom()
>>> n3 = custom3.Custom('jim', 'fulton', 'waaa')
Traceback (most recent call last):
  File "<stdin>", line 1, in ?
TypeError: an integer is required (got type str)
>>> del c1
>>> del c2

Custom 4

>>> import custom4
>>> c1 = custom4.Custom('jim', 'fulton', 42)
>>> c1.first
'jim'
>>> c1.last
'fulton'
>>> c1.number
42
>>> c1.name()
'jim fulton'
>>> c1.first = 'will'
>>> c1.name()
'will fulton'
>>> c1.last = 'tell'
>>> c1.name()
'will tell'
>>> del c1.first
Traceback (most recent call last):
...
TypeError: Cannot delete the first attribute
>>> c1.name()
'will tell'
>>> c1.first = 'drew'
>>> c1.first
'drew'
>>> del c1.number
Traceback (most recent call last):
...
TypeError: can't delete numeric/char attribute
>>> c1.number=2
>>> c1.number
2
>>> c1.first = 42
Traceback (most recent call last):
...
TypeError: The first attribute value must be a string
>>> c1.name()
'drew tell'
>>> c2 = custom4.Custom()
>>> c2 = custom4.Custom()
>>> c2 = custom4.Custom()
>>> c2 = custom4.Custom()
>>> c2.name()
' '
>>> c2.first
''
>>> c2.last
''
>>> c2.number
0
>>> n3 = custom4.Custom('jim', 'fulton', 'waaa')
Traceback (most recent call last):
...
TypeError: an integer is required (got type str)


Test cyclic gc(?)

>>> import gc
>>> gc.disable()

>>> class Subclass(custom4.Custom): pass
...
>>> s = Subclass()
>>> s.cycle = [s]
>>> s.cycle.append(s.cycle)
>>> x = object()
>>> s.x = x
>>> del s
>>> sys.getrefcount(x)
3
>>> ignore = gc.collect()
>>> sys.getrefcount(x)
2

>>> gc.enable()
"""

import os
import sys
from distutils.util import get_platform
PLAT_SPEC = "%s-%d.%d" % (get_platform(), *sys.version_info[:2])
src = os.path.join("build", "lib.%s" % PLAT_SPEC)
sys.path.append(src)

if __name__ == "__main__":
    import doctest, __main__
    doctest.testmod(__main__)
