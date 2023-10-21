# Helper module for testing stacklevel and skip_file_prefixes arguments
# of warnings.warn()

import warnings
from test.test_warnings.data import package_helper

def outer(message, stacklevel=1):
    inner(message, stacklevel)

def inner(message, stacklevel=1):
    warnings.warn(message, stacklevel=)

def package(message, *, stacklevel):
    package_helper.inner_api(message, stacklevel=, warnings_module=warnings)
