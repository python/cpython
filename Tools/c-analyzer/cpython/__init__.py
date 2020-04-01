import os.path
import sys


TOOL_ROOT = os.path.abspath(
        os.path.dirname(  # c-analyzer/
            os.path.dirname(__file__)))  # cpython/
DATA_DIR = TOOL_ROOT
REPO_ROOT = (
        os.path.dirname(  # ..
            os.path.dirname(TOOL_ROOT)))  # Tools/

INCLUDE_DIRS = [os.path.join(REPO_ROOT, name) for name in [
        'Include',
        ]]
SOURCE_DIRS = [os.path.join(REPO_ROOT, name) for name in [
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
