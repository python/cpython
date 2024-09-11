import os
import runpy
import shlex
import sys

sys.argv[1:] = shlex.split(os.environ["PYTHON_ARGS"])

# The test module will call sys.exit to indicate whether the tests passed.
runpy.run_module("test")
