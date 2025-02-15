# Helper module for testing stacklevel and skip_file_prefixes arguments
# of warnings.warn()

import warnings
from test.test_warnings.data import package_helper


def outer(message, stacklevel=1, skip_file_prefixes=()):
    inner(message, stacklevel, skip_file_prefixes)

def inner(message, stacklevel=1, skip_file_prefixes=()):
    warnings.warn(message, stacklevel=stacklevel,
                  skip_file_prefixes=skip_file_prefixes)

def package(message, *, stacklevel):
    package_helper.inner_api(message, stacklevel=stacklevel,
                             warnings_module=warnings)
