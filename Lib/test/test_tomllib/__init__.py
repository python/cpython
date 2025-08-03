# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2021 Taneli Hukkinen
# Licensed to PSF under a Contributor Agreement.

__all__ = ("tomllib",)

# By changing this one line, we can run the tests against
# a different module name.
import os

import tomllib

from test.support import load_package_tests


def load_tests(*args):
    return load_package_tests(os.path.dirname(__file__), *args)
