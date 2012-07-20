import sys

assert 'errno' in sys.builtin_module_names
NAME = 'errno'

assert 'importlib' not in sys.builtin_module_names
BAD_NAME = 'importlib'
