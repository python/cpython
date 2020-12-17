from __future__ import unicode_literals

import tox

from .common import base_discover
from .via_path import check_with_path


@tox.hookimpl
def tox_get_python_executable(envconfig):
    spec, path = base_discover(envconfig)
    if path is not None:
        return path
    # 3. check if the literal base python
    candidates = [envconfig.basepython]
    # 4. check if the un-versioned name is good
    if spec.name is not None and spec.name != envconfig.basepython:
        candidates.append(spec.name)
    return check_with_path(candidates, spec)
