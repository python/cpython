import sys
from warnings import warnpy3k

warnpy3k("the FileDialog module has been renamed "
         "to 'tkinter.filedialog' in Python 3.0", stacklevel=2)

import tkinter.filedialog
sys.modules[__name__] = tkinter.filedialog
