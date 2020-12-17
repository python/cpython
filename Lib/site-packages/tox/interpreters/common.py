import os

from tox.interpreters.py_spec import CURRENT, PythonSpec
from tox.interpreters.via_path import exe_spec


def base_discover(envconfig):
    base_python = envconfig.basepython
    spec = PythonSpec.from_name(base_python)

    # 1. check passed in discover elements
    discovers = envconfig.config.option.discover
    if not discovers:
        discovers = os.environ.get(str("TOX_DISCOVER"), "").split(os.pathsep)
    for discover in discovers:
        if os.path.exists(discover):
            cur_spec = exe_spec(discover, envconfig.basepython)
            if cur_spec is not None and cur_spec.satisfies(spec):
                return spec, cur_spec.path

    # 2. check current
    if spec.name is not None and CURRENT.satisfies(spec):
        return spec, CURRENT.path

    return spec, None
