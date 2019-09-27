import os.path
import sys


PKG_ROOT = os.path.dirname(__file__)
DATA_DIR = os.path.dirname(PKG_ROOT)
REPO_ROOT = os.path.dirname(
        os.path.dirname(DATA_DIR))

SOURCE_DIRS = [os.path.join(REPO_ROOT, name) for name in [
        'Include',
        'Python',
        'Parser',
        'Objects',
        'Modules',
        ]]

#PYTHON = os.path.join(REPO_ROOT, 'python')
PYTHON = sys.executable


# Clean up the namespace.
del sys
del os
