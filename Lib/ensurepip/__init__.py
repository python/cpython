#!/usr/bin/env python2
from __future__ import print_function

import os
import os.path
import pkgutil
import shutil
import sys
import tempfile


__all__ = ["version", "bootstrap"]


_SETUPTOOLS_VERSION = "19.4"

_PIP_VERSION = "8.0.0"

# pip currently requires ssl support, so we try to provide a nicer
# error message when that is missing (http://bugs.python.org/issue19744)
_MISSING_SSL_MESSAGE = ("pip {} requires SSL/TLS".format(_PIP_VERSION))
try:
    import ssl
except ImportError:
    ssl = None

    def _require_ssl_for_pip():
        raise RuntimeError(_MISSING_SSL_MESSAGE)
else:
    def _require_ssl_for_pip():
        pass

_PROJECTS = [
    ("setuptools", _SETUPTOOLS_VERSION),
    ("pip", _PIP_VERSION),
]


def _run_pip(args, additional_paths=None):
    # Add our bundled software to the sys.path so we can import it
    if additional_paths is not None:
        sys.path = additional_paths + sys.path

    # Install the bundled software
    import pip
    pip.main(args)


def version():
    """
    Returns a string specifying the bundled version of pip.
    """
    return _PIP_VERSION


def _disable_pip_configuration_settings():
    # We deliberately ignore all pip environment variables
    # when invoking pip
    # See http://bugs.python.org/issue19734 for details
    keys_to_remove = [k for k in os.environ if k.startswith("PIP_")]
    for k in keys_to_remove:
        del os.environ[k]
    # We also ignore the settings in the default pip configuration file
    # See http://bugs.python.org/issue20053 for details
    os.environ['PIP_CONFIG_FILE'] = os.devnull


def bootstrap(root=None, upgrade=False, user=False,
              altinstall=False, default_pip=True,
              verbosity=0):
    """
    Bootstrap pip into the current Python installation (or the given root
    directory).

    Note that calling this function will alter both sys.path and os.environ.
    """
    if altinstall and default_pip:
        raise ValueError("Cannot use altinstall and default_pip together")

    _require_ssl_for_pip()
    _disable_pip_configuration_settings()

    # By default, installing pip and setuptools installs all of the
    # following scripts (X.Y == running Python version):
    #
    #   pip, pipX, pipX.Y, easy_install, easy_install-X.Y
    #
    # pip 1.5+ allows ensurepip to request that some of those be left out
    if altinstall:
        # omit pip, pipX and easy_install
        os.environ["ENSUREPIP_OPTIONS"] = "altinstall"
    elif not default_pip:
        # omit pip and easy_install
        os.environ["ENSUREPIP_OPTIONS"] = "install"

    tmpdir = tempfile.mkdtemp()
    try:
        # Put our bundled wheels into a temporary directory and construct the
        # additional paths that need added to sys.path
        additional_paths = []
        for project, version in _PROJECTS:
            wheel_name = "{}-{}-py2.py3-none-any.whl".format(project, version)
            whl = pkgutil.get_data(
                "ensurepip",
                "_bundled/{}".format(wheel_name),
            )
            with open(os.path.join(tmpdir, wheel_name), "wb") as fp:
                fp.write(whl)

            additional_paths.append(os.path.join(tmpdir, wheel_name))

        # Construct the arguments to be passed to the pip command
        args = ["install", "--no-index", "--find-links", tmpdir]
        if root:
            args += ["--root", root]
        if upgrade:
            args += ["--upgrade"]
        if user:
            args += ["--user"]
        if verbosity:
            args += ["-" + "v" * verbosity]

        _run_pip(args + [p[0] for p in _PROJECTS], additional_paths)
    finally:
        shutil.rmtree(tmpdir, ignore_errors=True)


def _uninstall_helper(verbosity=0):
    """Helper to support a clean default uninstall process on Windows

    Note that calling this function may alter os.environ.
    """
    # Nothing to do if pip was never installed, or has been removed
    try:
        import pip
    except ImportError:
        return

    # If the pip version doesn't match the bundled one, leave it alone
    if pip.__version__ != _PIP_VERSION:
        msg = ("ensurepip will only uninstall a matching version "
               "({!r} installed, {!r} bundled)")
        print(msg.format(pip.__version__, _PIP_VERSION), file=sys.stderr)
        return

    _require_ssl_for_pip()
    _disable_pip_configuration_settings()

    # Construct the arguments to be passed to the pip command
    args = ["uninstall", "-y", "--disable-pip-version-check"]
    if verbosity:
        args += ["-" + "v" * verbosity]

    _run_pip(args + [p[0] for p in reversed(_PROJECTS)])


def _main(argv=None):
    if ssl is None:
        print("Ignoring ensurepip failure: {}".format(_MISSING_SSL_MESSAGE),
              file=sys.stderr)
        return

    import argparse
    parser = argparse.ArgumentParser(prog="python -m ensurepip")
    parser.add_argument(
        "--version",
        action="version",
        version="pip {}".format(version()),
        help="Show the version of pip that is bundled with this Python.",
    )
    parser.add_argument(
        "-v", "--verbose",
        action="count",
        default=0,
        dest="verbosity",
        help=("Give more output. Option is additive, and can be used up to 3 "
              "times."),
    )
    parser.add_argument(
        "-U", "--upgrade",
        action="store_true",
        default=False,
        help="Upgrade pip and dependencies, even if already installed.",
    )
    parser.add_argument(
        "--user",
        action="store_true",
        default=False,
        help="Install using the user scheme.",
    )
    parser.add_argument(
        "--root",
        default=None,
        help="Install everything relative to this alternate root directory.",
    )
    parser.add_argument(
        "--altinstall",
        action="store_true",
        default=False,
        help=("Make an alternate install, installing only the X.Y versioned"
              "scripts (Default: pipX, pipX.Y, easy_install-X.Y)"),
    )
    parser.add_argument(
        "--default-pip",
        action="store_true",
        default=True,
        dest="default_pip",
        help=argparse.SUPPRESS,
    )
    parser.add_argument(
        "--no-default-pip",
        action="store_false",
        dest="default_pip",
        help=("Make a non default install, installing only the X and X.Y "
              "versioned scripts."),
    )

    args = parser.parse_args(argv)

    bootstrap(
        root=args.root,
        upgrade=args.upgrade,
        user=args.user,
        verbosity=args.verbosity,
        altinstall=args.altinstall,
        default_pip=args.default_pip,
    )
