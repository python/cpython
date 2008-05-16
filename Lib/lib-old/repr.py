from warnings import warnpy3k
warnpy3k("The repr module has been renamed to 'reprlib' in Python 3.0",
         stacklevel=2)

from sys import modules
import reprlib
modules[__name__] = repr
