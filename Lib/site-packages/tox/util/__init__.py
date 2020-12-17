from __future__ import absolute_import, unicode_literals

import os
from contextlib import contextmanager


@contextmanager
def set_os_env_var(env_var_name, value):
    """Set an environment variable with unrolling once the context exists"""
    prev_value = os.environ.get(env_var_name)
    try:
        os.environ[env_var_name] = str(value)
        yield
    finally:
        if prev_value is None:
            del os.environ[env_var_name]
        else:
            os.environ[env_var_name] = prev_value
