import os
import os.path
import pkgutil
import sys
import tempfile

# TODO: Remove the --pre flag when a pip 1.5 final copy is available


__all__ = ["version", "bootstrap"]


_SETUPTOOLS_VERSION = "2.0.1"

_PIP_VERSION = "1.5rc2"

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


def bootstrap(*, root=None, upgrade=False, user=False,
              altinstall=False, default_pip=False,
              verbosity=0):
    """
    Bootstrap pip into the current Python installation (or the given root
    directory).
    """
    if altinstall and default_pip:
        raise ValueError("Cannot use altinstall and default_pip together")

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
        args = [
            "install", "--no-index", "--find-links", tmpdir,
            # Temporary until pip 1.5 is final
            "--pre",
        ]
        if root:
            args += ["--root", root]
        if upgrade:
            args += ["--upgrade"]
        if user:
            args += ["--user"]
        if verbosity:
            args += ["-" + "v" * verbosity]

        _run_pip(args + [p[0] for p in _PROJECTS], additional_paths)

def _uninstall(*, verbosity=0):
    """Helper to support a clean default uninstall process on Windows"""
    # Nothing to do if pip was never installed, or has been removed
    try:
        import pip
    except ImportError:
        return

    # If the pip version doesn't match the bundled one, leave it alone
    if pip.__version__ != _PIP_VERSION:
        msg = ("ensurepip will only uninstall a matching pip "
               "({!r} installed, {!r} bundled)")
        raise RuntimeError(msg.format(pip.__version__, _PIP_VERSION))

    # Construct the arguments to be passed to the pip command
    args = ["uninstall", "-y"]
    if verbosity:
        args += ["-" + "v" * verbosity]

    _run_pip(args + [p[0] for p in reversed(_PROJECTS)])
