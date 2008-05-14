import sys
from warnings import warnpy3k

warnpy3k("the ConfigParser module has been renamed "
         "to 'configparser' in Python 3.0", stacklevel=2)

import configparser
sys.modules[__name__] = configparser
