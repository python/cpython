import sys
from warnings import warnpy3k

warnpy3k("the copy_reg module has been renamed "
         "to 'copyreg' in Python 3.0", stacklevel=2)

import copyreg
sys.modules[__name__] = copyreg
