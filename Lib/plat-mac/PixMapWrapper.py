import sys
from warnings import warnpy3k

warnpy3k("the PixMapWrapper module has been renamed "
         "to 'pixmapwrapper' in Python 3.0", stacklevel=2)

import pixmapwrapper
sys.modules[__name__] = pixmapwrapper
