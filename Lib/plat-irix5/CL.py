# Backward compatible module CL.
# All relevant symbols are now defined in the module cl.
from warnings import warnpy3k
warnpy3k("the CL module has been removed in Python 3.0", stacklevel=2)
del warnpy3k

try:
    from cl import *
except ImportError:
    from CL_old import *
else:
    del CompressImage
    del DecompressImage
    del GetAlgorithmName
    del OpenCompressor
    del OpenDecompressor
    del QueryAlgorithms
    del QueryMaxHeaderSize
    del QueryScheme
    del QuerySchemeFromName
    del SetDefault
    del SetMax
    del SetMin
    try:
        del cvt_type
    except NameError:
        pass
    del error
