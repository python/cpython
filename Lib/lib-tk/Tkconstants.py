import sys
from warnings import warnpy3k

warnpy3k("the Tkconstants module has been renamed "
         "to 'tkinter.constants' in Python 3.0", stacklevel=2)

import tkinter.constants
sys.modules[__name__] = tkinter.constants
