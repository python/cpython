"""
Extraction and file list generation for pip.
"""

__author__ = "Steve Dower <steve.dower@python.org>"
__version__ = "3.8"


import os
import shutil
import subprocess
import sys

from .filesets import *

__all__ = ["extract_pip_files", "get_pip_layout"]


def get_pip_dir(ns):
    if ns.copy:
        if ns.zip_lib:
            return ns.copy / "packages"
        return ns.copy / "Lib" / "site-packages"
    else:
        return ns.temp / "packages"


def get_pip_layout(ns):
    pip_dir = get_pip_dir(ns)
    if not pip_dir.is_dir():
        log_warning("Failed to find {} - pip will not be included", pip_dir)
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
        dest.mkdir(parents=True, exist_ok=False)
    except IOError:
        return

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
        shutil.rmtree(dest / "bin")
    except OSError:
        pass

    for file in wheels:
        try:
            os.remove(file)
        except OSError:
            pass
