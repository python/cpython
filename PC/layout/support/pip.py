"""
Extraction and file list generation for pip.
"""

__author__ = "Jeremy Paige <jeremyp@activestate.com>"
__version__ = "2.7"


import os
import shutil
import subprocess
import sys
from glob import glob

from .filesets import *

__all__ = ["extract_pip_files", "get_pip_layout"]


def get_pip_dir(ns):
    if ns.copy:
        if ns.zip_lib:
            return os.path.join(ns.copy, "packages")
        return os.path.join(ns.copy, "Lib", "site-packages")
    else:
        return os.path.join(ns.temp, "packages")


def get_pip_layout(ns):
    pip_dir = get_pip_dir(ns)
    if not os.path.isdir(pip_dir):
        log_warning("Failed to find %s - pip will not be included", pip_dir)
    else:
        pkg_root = "packages/{}" if ns.zip_lib else "Lib/site-packages/{}"
        for dest, src in rglob(pip_dir, "**/*"):
            yield pkg_root.format(dest), src
        if ns.include_pip_user:
            content = "\n".join(
                "[{}]\nuser=yes".format(n)
                for n in ["install", "uninstall", "freeze", "list"]
            )
            yield "pip.ini", ("pip.ini", content.encode())


def extract_pip_files(ns):
    dest = get_pip_dir(ns)
    try:
        os.makedirs(dest)
    except IOError:
        return

    src = os.path.join(ns.source, "Lib", "ensurepip", "_bundled")

    if not os.path.exists(ns.temp):
        os.makedirs(ns.temp)
    wheels = []
    for whl in glob(os.path.join(src, "*.whl")):
        shutil.copy(whl, ns.temp)
        wheels.append(os.path.join(ns.temp, whl))
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
            dest,
            "--no-index",
            "--no-compile",
            "--no-cache-dir",
            "-f",
            str(src),
            "--only-binary",
            ":all:",
        ],
        env=env,
    )

    try:
        shutil.rmtree(os.path.join(dest, "bin"))
    except OSError:
        pass

    for file in wheels:
        try:
            os.remove(file)
        except OSError:
            pass
