"""
Extraction and file list generation for pip.
"""

__author__ = "Steve Dower <steve.dower@python.org>"
__version__ = "3.8"


import os
import shutil
import subprocess
import sys

__all__ = []


def public(f):
    __all__.append(f.__name__)
    return f


@public
def get_pip_dir(ns):
    if ns.copy:
        if ns.zip_lib:
            return ns.copy / "packages"
        return ns.copy / "Lib" / "site-packages"
    else:
        return ns.temp / "packages"


@public
def extract_pip_files(ns):
    dest = get_pip_dir(ns)
    dest.mkdir(parents=True, exist_ok=True)

    src = ns.source / "Lib" / "ensurepip" / "_bundled"

    ns.temp.mkdir(parents=True, exist_ok=True)
    wheels = [shutil.copy(whl, ns.temp) for whl in src.glob("*.whl")]
    search_path = os.pathsep.join(wheels)
    if os.environ.get("PYTHONPATH"):
        search_path += ";" + os.environ["PYTHONPATH"]

    env = os.environ.copy()
    env["PYTHONPATH"] = search_path

    output = subprocess.check_output(
        [
            sys.executable,
            "-m",
            "pip",
            "--no-color",
            "install",
            "pip",
            "setuptools",
            "--upgrade",
            "--target",
            str(dest),
            "--no-index",
            "--no-cache-dir",
            "-f",
            str(src),
            "--only-binary",
            ":all:",
        ],
        env=env,
    )

    try:
        shutil.rmtree(dest / "bin")
    except OSError:
        pass

    for file in wheels:
        try:
            os.remove(file)
        except OSError:
            pass
