import sys
import os
_maclib = os.path.join(sys.prefix, 'Mac', 'Lib')
_scriptlib = os.path.join(_maclib, 'lib-scriptpackages')
sys.path.append(_maclib)
sys.path.append(_scriptlib)
