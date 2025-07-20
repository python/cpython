import os.path


TOOL_ROOT = os.path.normcase(
    os.path.abspath(
        os.path.dirname(  # c-analyzer/
            os.path.dirname(__file__))))  # cpython/
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
