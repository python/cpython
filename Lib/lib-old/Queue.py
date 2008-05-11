import sys
from warnings import warnpy3k

warnpy3k("the Queue module has been renamed "
         "to 'queue' in Python 3.0", stacklevel=2)

import queue
sys.modules[__name__] = queue
