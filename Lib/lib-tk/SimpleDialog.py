import sys
from warnings import warnpy3k

warnpy3k("the SimpleDialog module has been renamed "
         "to 'tkinter.simpledialog' in Python 3.0", stacklevel=2)

import tkinter.simpledialog
sys.modules[__name__] = tkinter.simpledialog
