import warnings
warnings.warn("The %s module is deprecated; use Carbon.%s"%(__name__, __name__),
	DeprecationWarning, stacklevel=2)
from Carbon.Fm import *
