import py

import tox
from tox.reporter import error, info, verbosity0, verbosity2, warning
from tox.util.lock import hold_lock

from .builder import build_package
from .local import resolve_package
from .view import create_session_view


@tox.hookimpl
def tox_package(session, venv):
    """Build an sdist at first call return that for all calls"""
    if not hasattr(session, "package"):
        session.package, session.dist = get_package(session)
    return session.package


def get_package(session):
    """"Perform the package operation"""
    config = session.config
    if config.skipsdist:
        info("skipping sdist step")
        return None
    lock_file = session.config.toxworkdir.join("{}.lock".format(session.config.isolated_build_env))

    with hold_lock(lock_file, verbosity0):
        package = acquire_package(config, session)
        session_package = create_session_view(package, config.temp_dir)
        return session_package, package


def acquire_package(config, session):
    """acquire a source distribution (either by loading a local file or triggering a build)"""
    if not config.option.sdistonly and (config.sdistsrc or config.option.installpkg):
        path = get_local_package(config)
    else:
        try:
            path = build_package(config, session)
        except tox.exception.InvocationError as exception:
            error("FAIL could not package project - v = {!r}".format(exception))
            return None
        sdist_file = config.distshare.join(path.basename)
        if sdist_file != path:
            info("copying new sdistfile to {!r}".format(str(sdist_file)))
            try:
                sdist_file.dirpath().ensure(dir=1)
            except py.error.Error:
                warning("could not copy distfile to {}".format(sdist_file.dirpath()))
            else:
                path.copy(sdist_file)
    return path


def get_local_package(config):
    path = config.option.installpkg
    if not path:
        path = config.sdistsrc
    py_path = py.path.local(resolve_package(path))
    info("using package {!r}, skipping 'sdist' activity ".format(str(py_path)))
    return py_path


@tox.hookimpl
def tox_cleanup(session):
    for tox_env in session.venv_dict.values():
        if hasattr(tox_env, "package") and isinstance(tox_env.package, py.path.local):
            package = tox_env.package
            if package.exists():
                verbosity2("cleanup {}".format(package))
                package.remove()
                py.path.local(package.dirname).remove(ignore_errors=True)
