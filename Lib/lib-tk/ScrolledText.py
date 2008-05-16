import sys
from warnings import warnpy3k

warnpy3k("the ScrolledText module has been renamed "
         "to 'tkinter.scrolledtext' in Python 3.0", stacklevel=2)

import tkinter.scrolledtext
sys.modules[__name__] = tkinter.scrolledtext
