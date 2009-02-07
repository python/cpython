import imp
import os
import sys

PATH = None
EXT = None
FILENAME = None
NAME = '_testcapi'
_file_exts = [x[0] for x in imp.get_suffixes() if x[2] == imp.C_EXTENSION]
try:
    for PATH in sys.path:
        for EXT in _file_exts:
            FILENAME = NAME + EXT
            FILEPATH = os.path.join(PATH, FILENAME)
            if os.path.exists(os.path.join(PATH, FILENAME)):
                raise StopIteration
    else:
        PATH = EXT = FILENAME = FILEPATH = None
except StopIteration:
    pass
del _file_exts
