import inspect
import os

import tox

MAIN_FILE = os.path.join(os.path.dirname(inspect.getfile(tox)), "__main__.py")
