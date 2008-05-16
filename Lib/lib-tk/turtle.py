import sys
from warnings import warnpy3k

warnpy3k("the turtle module has been renamed "
         "to 'tkinter.turtle' in Python 3.0", stacklevel=2)

import tkinter.turtle
sys.modules[__name__] = tkinter.turtle
