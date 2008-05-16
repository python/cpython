import sys
from warnings import warnpy3k

warnpy3k("the tkCommonDialog module has been renamed "
         "to 'tkinter.commondialog' in Python 3.0", stacklevel=2)

import tkinter.commondialog
sys.modules[__name__] = tkinter.commondialog
