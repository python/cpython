"""All non private names (no leading underscore) here are part of the tox API.

They live in the tox namespace and can be accessed as tox.[NAMESPACE.]NAME
"""
import os
import re
import sys

_THIS_FILE = os.path.realpath(os.path.abspath(__file__))


class PYTHON:
    PY_FACTORS_RE = re.compile("^(?!py$)(py|pypy|jython)([2-9][0-9]?[0-9]?)?$")
    CURRENT_RELEASE_ENV = "py37"
    """Should hold currently released py -> for easy updating"""
    QUICKSTART_PY_ENVS = ["py27", "py35", "py36", CURRENT_RELEASE_ENV, "pypy", "jython"]
    """For choices in tox-quickstart"""


class INFO:
    DEFAULT_CONFIG_NAME = "tox.ini"
    CONFIG_CANDIDATES = ("pyproject.toml", "tox.ini", "setup.cfg")
    IS_WIN = sys.platform == "win32"
    IS_PYPY = hasattr(sys, "pypy_version_info")


class PIP:
    SHORT_OPTIONS = ["c", "e", "r", "b", "t", "d"]
    LONG_OPTIONS = [
        "build",
        "cache-dir",
        "client-cert",
        "constraint",
        "download",
        "editable",
        "exists-action",
        "extra-index-url",
        "global-option",
        "find-links",
        "index-url",
        "install-options",
        "prefix",
        "proxy",
        "no-binary",
        "only-binary",
        "requirement",
        "retries",
        "root",
        "src",
        "target",
        "timeout",
        "trusted-host",
        "upgrade-strategy",
    ]
    INSTALL_SHORT_OPTIONS_ARGUMENT = ["-{}".format(option) for option in SHORT_OPTIONS]
    INSTALL_LONG_OPTIONS_ARGUMENT = ["--{}".format(option) for option in LONG_OPTIONS]


_HELP_DIR = os.path.join(os.path.dirname(_THIS_FILE), "helper")
VERSION_QUERY_SCRIPT = os.path.join(_HELP_DIR, "get_version.py")
SITE_PACKAGE_QUERY_SCRIPT = os.path.join(_HELP_DIR, "get_site_package_dir.py")
BUILD_REQUIRE_SCRIPT = os.path.join(_HELP_DIR, "build_requires.py")
BUILD_ISOLATED = os.path.join(_HELP_DIR, "build_isolated.py")
PARALLEL_RESULT_JSON_PREFIX = ".tox-result"
PARALLEL_RESULT_JSON_SUFFIX = ".json"
