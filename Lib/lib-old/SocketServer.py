import sys
from warnings import warnpy3k

warnpy3k("the SocketServer module has been renamed "
         "to 'socketserver' in Python 3.0", stacklevel=2)

import socketserver
sys.modules[__name__] = socketserver
