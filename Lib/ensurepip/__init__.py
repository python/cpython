import collections
import os.path
import re
import runpy
import subprocess
import sys
import sysconfig
import tempfile
from importlib import resources



__all__ = ["version", "bootstrap"]
_PACKAGE_NAMES = ('setuptools', 'pip')
_Package = collections.namedtuple('Package', ('version', 'filename'))

# Directory of system wheel packages. Some Linux distribution packaging
# policies recommand against bundling dependencies. For example, Fedora
# installs wheel packages in the /usr/share/python-wheels/ directory and don't
# install the ensurepip._bundled package.
_WHEEL_PKG_DIR = sysconfig.get_config_var('WHEEL_PKG_DIR')


def _get_versions(path):
    versions = {}
    try:
        names = os.listdir(path)
    except OSError:
        # Ignore: path doesn't exist or permission error
        names = ()
    for name in names:
        # name is like 'pip-20.2.3-py2.py3-none-any.whl'
        if not name.endswith(".whl"):
            continue
        for package in _PACKAGE_NAMES:
            prefix = package + '-'
            if not name.startswith(prefix):
                continue
            part = name.removeprefix(prefix)
            break
        else:
            continue

        # Extract '20.2.3' from '20.2.3-py2.py3-none-any.whl'
        version = []
        part = part.split('-', 1)[0]
        for part in part.split('.'):
            try:
                number = int(part)
            except ValueError:
                break
            version.append(number)
        if not version:
            # failed to parse the version: ignore the package
            continue

        fullname = os.path.join(path, name)
        versions[package] = _Package(tuple(version), fullname)
    return versions


def _get_package_search_paths():
    # last item has the highest priority
    paths = []
    if _WHEEL_PKG_DIR:
        paths.append(_WHEEL_PKG_DIR)
    try:
        from . import _bundled
    except ImportError:
        pass
    else:
        paths.extend(reversed(_bundled.__path__))
    return paths


def _find_packages(paths):
    # package => (version, filename): (None, None) if no package is found
    packages = {name: _Package(None, None) for name in _PACKAGE_NAMES}
    for path in paths:
        versions = _get_versions(path)
        for name, item in versions.items():
            if (packages[name].version is None
               or packages[name][0] < item.version):
                packages[name] = item

    return packages


def _get_packages():
    global _PACKAGES
    if _PACKAGES is None:
        paths = _get_package_search_paths()
        _PACKAGES = _find_packages(paths)
    return _PACKAGES
_PACKAGES = None


def _run_pip(args, additional_paths=None):
    # Run the bootstraping in a subprocess to avoid leaking any state that happens
    # after pip has executed. Particulary, this avoids the case when pip holds onto
    # the files in *additional_paths*, preventing us to remove them at the end of the
    # invocation.
    code = f"""
import runpy
import sys
sys.path = {additional_paths or []} + sys.path
sys.argv[1:] = {args}
runpy.run_module("pip", run_name="__main__", alter_sys=True)
"""
    return subprocess.run([sys.executable, "-c", code], check=True).returncode


def version():
    """
    Returns a string specifying the bundled version of pip.
    """
    version = _get_packages()['pip'].version
    if version is None:
        return None
    return '.'.join(map(str, version))

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


def bootstrap(*, root=None, upgrade=False, user=False,
              altinstall=False, default_pip=False,
              verbosity=0):
    """
    Bootstrap pip into the current Python installation (or the given root
    directory).

    Note that calling this function will alter both sys.path and os.environ.
    """
    # Discard the return value
    _bootstrap(root=root, upgrade=upgrade, user=user,
               altinstall=altinstall, default_pip=default_pip,
               verbosity=verbosity)


def _bootstrap(*, root=None, upgrade=False, user=False,
              altinstall=False, default_pip=False,
              verbosity=0):
    """
    Bootstrap pip into the current Python installation (or the given root
    directory). Returns pip command status code.

    Note that calling this function will alter both sys.path and os.environ.
    """
    if altinstall and default_pip:
        raise ValueError("Cannot use altinstall and default_pip together")

    sys.audit("ensurepip.bootstrap", root)

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

    with tempfile.TemporaryDirectory() as tmpdir:
        # Put our bundled wheels into a temporary directory and construct the
        # additional paths that need added to sys.path
        additional_paths = []
        for name, package in _get_packages().items():
            if package.filename is None:
                raise ValueError(f"cannot find {name} wheel package")

            with open(package.filename, "rb") as fp:
                whl = fp.read()

            wheel_name = os.path.basename(package.filename)
            with open(os.path.join(tmpdir, wheel_name), "wb") as fp:
                fp.write(whl)

            additional_paths.append(os.path.join(tmpdir, wheel_name))

        # Construct the arguments to be passed to the pip command
        args = ["install", "--no-cache-dir", "--no-index", "--find-links", tmpdir]
        if root:
            args += ["--root", root]
        if upgrade:
            args += ["--upgrade"]
        if user:
            args += ["--user"]
        if verbosity:
            args += ["-" + "v" * verbosity]

        return _run_pip([*args, *_PACKAGE_NAMES], additional_paths)

def _uninstall_helper(*, verbosity=0):
    """Helper to support a clean default uninstall process on Windows

    Note that calling this function may alter os.environ.
    """
    # Nothing to do if pip was never installed, or has been removed
    try:
        import pip
    except ImportError:
        return

    # If the pip version doesn't match the bundled one, leave it alone
    if pip.__version__ != version():
        print(f"ensurepip will only uninstall a matching version "
              f"({pip.__version__!r} installed, {version()!r} bundled)",
              file=sys.stderr)
        return

    _disable_pip_configuration_settings()

    # Construct the arguments to be passed to the pip command
    args = ["uninstall", "-y", "--disable-pip-version-check"]
    if verbosity:
        args += ["-" + "v" * verbosity]

    return _run_pip([*args, reversed(_PACKAGE_NAMES)])


def _main(argv=None):
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
        help=("Make an alternate install, installing only the X.Y versioned "
              "scripts (Default: pipX, pipX.Y, easy_install-X.Y)."),
    )
    parser.add_argument(
        "--default-pip",
        action="store_true",
        default=False,
        help=("Make a default pip install, installing the unqualified pip "
              "and easy_install in addition to the versioned scripts."),
    )

    args = parser.parse_args(argv)

    return _bootstrap(
        root=args.root,
        upgrade=args.upgrade,
        user=args.user,
        verbosity=args.verbosity,
        altinstall=args.altinstall,
        default_pip=args.default_pip,
    )
