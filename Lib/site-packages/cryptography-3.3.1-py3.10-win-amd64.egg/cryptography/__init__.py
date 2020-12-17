# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import sys
import warnings

from cryptography.__about__ import (
    __author__,
    __copyright__,
    __email__,
    __license__,
    __summary__,
    __title__,
    __uri__,
    __version__,
)
from cryptography.utils import CryptographyDeprecationWarning


__all__ = [
    "__title__",
    "__summary__",
    "__uri__",
    "__version__",
    "__author__",
    "__email__",
    "__license__",
    "__copyright__",
]

if sys.version_info[0] == 2:
    warnings.warn(
        "Python 2 is no longer supported by the Python core team. Support for "
        "it is now deprecated in cryptography, and will be removed in the "
        "next release.",
        CryptographyDeprecationWarning,
        stacklevel=2,
    )
