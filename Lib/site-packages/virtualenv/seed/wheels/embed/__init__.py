from __future__ import absolute_import, unicode_literals

from virtualenv.seed.wheels.util import Wheel
from virtualenv.util.path import Path

BUNDLE_FOLDER = Path(__file__).absolute().parent
BUNDLE_SUPPORT = {
    "3.10": {
        "pip": "pip-20.3.1-py2.py3-none-any.whl",
        "setuptools": "setuptools-51.0.0-py3-none-any.whl",
        "wheel": "wheel-0.36.1-py2.py3-none-any.whl",
    },
    "3.9": {
        "pip": "pip-20.3.1-py2.py3-none-any.whl",
        "setuptools": "setuptools-51.0.0-py3-none-any.whl",
        "wheel": "wheel-0.36.1-py2.py3-none-any.whl",
    },
    "3.8": {
        "pip": "pip-20.3.1-py2.py3-none-any.whl",
        "setuptools": "setuptools-51.0.0-py3-none-any.whl",
        "wheel": "wheel-0.36.1-py2.py3-none-any.whl",
    },
    "3.7": {
        "pip": "pip-20.3.1-py2.py3-none-any.whl",
        "setuptools": "setuptools-51.0.0-py3-none-any.whl",
        "wheel": "wheel-0.36.1-py2.py3-none-any.whl",
    },
    "3.6": {
        "pip": "pip-20.3.1-py2.py3-none-any.whl",
        "setuptools": "setuptools-51.0.0-py3-none-any.whl",
        "wheel": "wheel-0.36.1-py2.py3-none-any.whl",
    },
    "3.5": {
        "pip": "pip-20.3.1-py2.py3-none-any.whl",
        "setuptools": "setuptools-50.3.2-py3-none-any.whl",
        "wheel": "wheel-0.36.1-py2.py3-none-any.whl",
    },
    "3.4": {
        "pip": "pip-19.1.1-py2.py3-none-any.whl",
        "setuptools": "setuptools-43.0.0-py2.py3-none-any.whl",
        "wheel": "wheel-0.33.6-py2.py3-none-any.whl",
    },
    "2.7": {
        "pip": "pip-20.3.1-py2.py3-none-any.whl",
        "setuptools": "setuptools-44.1.1-py2.py3-none-any.whl",
        "wheel": "wheel-0.36.1-py2.py3-none-any.whl",
    },
}
MAX = "3.10"


def get_embed_wheel(distribution, for_py_version):
    path = BUNDLE_FOLDER / (BUNDLE_SUPPORT.get(for_py_version, {}) or BUNDLE_SUPPORT[MAX]).get(distribution)
    return Wheel.from_path(path)


__all__ = (
    "get_embed_wheel",
    "BUNDLE_SUPPORT",
    "MAX",
    "BUNDLE_FOLDER",
)
