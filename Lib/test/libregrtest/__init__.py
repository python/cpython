# We import importlib *ASAP* in order to test #15386
import importlib

from test.libregrtest.cmdline import _parse_args, RESOURCE_NAMES, ALL_RESOURCES
from test.libregrtest.main import main
