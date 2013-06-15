from importlib import machinery
import os
import sys

PATH = None
EXT = None
FILENAME = None
NAME = '_testcapi'
try:
    for PATH in sys.path:
        for EXT in machinery.EXTENSION_SUFFIXES:
            FILENAME = NAME + EXT
            FILEPATH = os.path.join(PATH, FILENAME)
            if os.path.exists(os.path.join(PATH, FILENAME)):
                raise StopIteration
    else:
        PATH = EXT = FILENAME = FILEPATH = None
except StopIteration:
    pass
