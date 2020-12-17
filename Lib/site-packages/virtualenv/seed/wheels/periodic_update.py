"""
Periodically update bundled versions.
"""

from __future__ import absolute_import, unicode_literals

import json
import logging
import os
import ssl
import subprocess
import sys
from datetime import datetime, timedelta
from itertools import groupby
from shutil import copy2
from textwrap import dedent
from threading import Thread

from six.moves.urllib.error import URLError
from six.moves.urllib.request import urlopen

from virtualenv.app_data import AppDataDiskFolder
from virtualenv.info import PY2
from virtualenv.util.path import Path
from virtualenv.util.subprocess import CREATE_NO_WINDOW, Popen

from ..wheels.embed import BUNDLE_SUPPORT
from ..wheels.util import Wheel

if PY2:
    # on Python 2 datetime.strptime throws the error below if the import did not trigger on main thread
    # Failed to import _strptime because the import lock is held by
    try:
        import _strptime  # noqa
    except ImportError:  # pragma: no cov
        pass  # pragma: no cov


def periodic_update(distribution, for_py_version, wheel, search_dirs, app_data, do_periodic_update):
    if do_periodic_update:
        handle_auto_update(distribution, for_py_version, wheel, search_dirs, app_data)

    now = datetime.now()

    u_log = UpdateLog.from_app_data(app_data, distribution, for_py_version)
    u_log_older_than_hour = now - u_log.completed > timedelta(hours=1) if u_log.completed is not None else False
    for _, group in groupby(u_log.versions, key=lambda v: v.wheel.version_tuple[0:2]):
        version = next(group)  # use only latest patch version per minor, earlier assumed to be buggy
        if wheel is not None and Path(version.filename).name == wheel.name:
            break
        if u_log.periodic is False or (u_log_older_than_hour and version.use(now)):
            updated_wheel = Wheel(app_data.house / version.filename)
            logging.debug("using %supdated wheel %s", "periodically " if updated_wheel else "", updated_wheel)
            wheel = updated_wheel
            break

    return wheel


def handle_auto_update(distribution, for_py_version, wheel, search_dirs, app_data):
    embed_update_log = app_data.embed_update_log(distribution, for_py_version)
    u_log = UpdateLog.from_dict(embed_update_log.read())
    if u_log.needs_update:
        u_log.periodic = True
        u_log.started = datetime.now()
        embed_update_log.write(u_log.to_dict())
        trigger_update(distribution, for_py_version, wheel, search_dirs, app_data, periodic=True)


DATETIME_FMT = "%Y-%m-%dT%H:%M:%S.%fZ"


def dump_datetime(value):
    return None if value is None else value.strftime(DATETIME_FMT)


def load_datetime(value):
    return None if value is None else datetime.strptime(value, DATETIME_FMT)


class NewVersion(object):
    def __init__(self, filename, found_date, release_date):
        self.filename = filename
        self.found_date = found_date
        self.release_date = release_date

    @classmethod
    def from_dict(cls, dictionary):
        return cls(
            filename=dictionary["filename"],
            found_date=load_datetime(dictionary["found_date"]),
            release_date=load_datetime(dictionary["release_date"]),
        )

    def to_dict(self):
        return {
            "filename": self.filename,
            "release_date": dump_datetime(self.release_date),
            "found_date": dump_datetime(self.found_date),
        }

    def use(self, now):
        compare_from = self.release_date or self.found_date
        return now - compare_from >= timedelta(days=28)

    def __repr__(self):
        return "{}(filename={}), found_date={}, release_date={})".format(
            self.__class__.__name__,
            self.filename,
            self.found_date,
            self.release_date,
        )

    def __eq__(self, other):
        return type(self) == type(other) and all(
            getattr(self, k) == getattr(other, k) for k in ["filename", "release_date", "found_date"]
        )

    def __ne__(self, other):
        return not (self == other)

    @property
    def wheel(self):
        return Wheel(Path(self.filename))


class UpdateLog(object):
    def __init__(self, started, completed, versions, periodic):
        self.started = started
        self.completed = completed
        self.versions = versions
        self.periodic = periodic

    @classmethod
    def from_dict(cls, dictionary):
        if dictionary is None:
            dictionary = {}
        return cls(
            load_datetime(dictionary.get("started")),
            load_datetime(dictionary.get("completed")),
            [NewVersion.from_dict(v) for v in dictionary.get("versions", [])],
            dictionary.get("periodic"),
        )

    @classmethod
    def from_app_data(cls, app_data, distribution, for_py_version):
        raw_json = app_data.embed_update_log(distribution, for_py_version).read()
        return cls.from_dict(raw_json)

    def to_dict(self):
        return {
            "started": dump_datetime(self.started),
            "completed": dump_datetime(self.completed),
            "periodic": self.periodic,
            "versions": [r.to_dict() for r in self.versions],
        }

    @property
    def needs_update(self):
        now = datetime.now()
        if self.completed is None:  # never completed
            return self._check_start(now)
        else:
            if now - self.completed <= timedelta(days=14):
                return False
            return self._check_start(now)

    def _check_start(self, now):
        return self.started is None or now - self.started > timedelta(hours=1)


def trigger_update(distribution, for_py_version, wheel, search_dirs, app_data, periodic):
    wheel_path = None if wheel is None else str(wheel.path)
    cmd = [
        sys.executable,
        "-c",
        dedent(
            """
        from virtualenv.report import setup_report, MAX_LEVEL
        from virtualenv.seed.wheels.periodic_update import do_update
        setup_report(MAX_LEVEL, show_pid=True)
        do_update({!r}, {!r}, {!r}, {!r}, {!r}, {!r})
        """,
        )
        .strip()
        .format(distribution, for_py_version, wheel_path, str(app_data), [str(p) for p in search_dirs], periodic),
    ]
    debug = os.environ.get(str("_VIRTUALENV_PERIODIC_UPDATE_INLINE")) == str("1")
    pipe = None if debug else subprocess.PIPE
    kwargs = {"stdout": pipe, "stderr": pipe}
    if not debug and sys.platform == "win32":
        kwargs["creationflags"] = CREATE_NO_WINDOW
    process = Popen(cmd, **kwargs)
    logging.info(
        "triggered periodic upgrade of %s%s (for python %s) via background process having PID %d",
        distribution,
        "" if wheel is None else "=={}".format(wheel.version),
        for_py_version,
        process.pid,
    )
    if debug:
        process.communicate()  # on purpose not called to make it a background process


def do_update(distribution, for_py_version, embed_filename, app_data, search_dirs, periodic):
    versions = None
    try:
        versions = _run_do_update(app_data, distribution, embed_filename, for_py_version, periodic, search_dirs)
    finally:
        logging.debug("done %s %s with %s", distribution, for_py_version, versions)
    return versions


def _run_do_update(app_data, distribution, embed_filename, for_py_version, periodic, search_dirs):
    from virtualenv.seed.wheels import acquire

    wheel_filename = None if embed_filename is None else Path(embed_filename)
    embed_version = None if wheel_filename is None else Wheel(wheel_filename).version_tuple
    app_data = AppDataDiskFolder(app_data) if isinstance(app_data, str) else app_data
    search_dirs = [Path(p) if isinstance(p, str) else p for p in search_dirs]
    wheelhouse = app_data.house
    embed_update_log = app_data.embed_update_log(distribution, for_py_version)
    u_log = UpdateLog.from_dict(embed_update_log.read())
    now = datetime.now()
    if wheel_filename is not None:
        dest = wheelhouse / wheel_filename.name
        if not dest.exists():
            copy2(str(wheel_filename), str(wheelhouse))
    last, last_version, versions = None, None, []
    while last is None or not last.use(now):
        download_time = datetime.now()
        dest = acquire.download_wheel(
            distribution=distribution,
            version_spec=None if last_version is None else "<{}".format(last_version),
            for_py_version=for_py_version,
            search_dirs=search_dirs,
            app_data=app_data,
            to_folder=wheelhouse,
        )
        if dest is None or (u_log.versions and u_log.versions[0].filename == dest.name):
            break
        release_date = release_date_for_wheel_path(dest.path)
        last = NewVersion(filename=dest.path.name, release_date=release_date, found_date=download_time)
        logging.info("detected %s in %s", last, datetime.now() - download_time)
        versions.append(last)
        last_wheel = Wheel(Path(last.filename))
        last_version = last_wheel.version
        if embed_version is not None:
            if embed_version >= last_wheel.version_tuple:  # stop download if we reach the embed version
                break
    u_log.periodic = periodic
    if not u_log.periodic:
        u_log.started = now
    u_log.versions = versions + u_log.versions
    u_log.completed = datetime.now()
    embed_update_log.write(u_log.to_dict())
    return versions


def release_date_for_wheel_path(dest):
    wheel = Wheel(dest)
    # the most accurate is to ask PyPi - e.g. https://pypi.org/pypi/pip/json,
    # see https://warehouse.pypa.io/api-reference/json/ for more details
    content = _pypi_get_distribution_info_cached(wheel.distribution)
    if content is not None:
        try:
            upload_time = content["releases"][wheel.version][0]["upload_time"]
            return datetime.strptime(upload_time, "%Y-%m-%dT%H:%M:%S")
        except Exception as exception:
            logging.error("could not load release date %s because %r", content, exception)
    return None


def _request_context():
    yield None
    # fallback to non verified HTTPS (the information we request is not sensitive, so fallback)
    yield ssl._create_unverified_context()  # noqa


_PYPI_CACHE = {}


def _pypi_get_distribution_info_cached(distribution):
    if distribution not in _PYPI_CACHE:
        _PYPI_CACHE[distribution] = _pypi_get_distribution_info(distribution)
    return _PYPI_CACHE[distribution]


def _pypi_get_distribution_info(distribution):
    content, url = None, "https://pypi.org/pypi/{}/json".format(distribution)
    try:
        for context in _request_context():
            try:
                with urlopen(url, context=context) as file_handler:
                    content = json.load(file_handler)
                break
            except URLError as exception:
                logging.error("failed to access %s because %r", url, exception)
    except Exception as exception:
        logging.error("failed to access %s because %r", url, exception)
    return content


def manual_upgrade(app_data):
    threads = []

    for for_py_version, distribution_to_package in BUNDLE_SUPPORT.items():
        # load extra search dir for the given for_py
        for distribution in distribution_to_package.keys():
            thread = Thread(target=_run_manual_upgrade, args=(app_data, distribution, for_py_version))
            thread.start()
            threads.append(thread)

    for thread in threads:
        thread.join()


def _run_manual_upgrade(app_data, distribution, for_py_version):
    start = datetime.now()
    from .bundle import from_bundle

    current = from_bundle(
        distribution=distribution,
        version=None,
        for_py_version=for_py_version,
        search_dirs=[],
        app_data=app_data,
        do_periodic_update=False,
    )
    logging.warning(
        "upgrade %s for python %s with current %s",
        distribution,
        for_py_version,
        "" if current is None else current.name,
    )
    versions = do_update(
        distribution=distribution,
        for_py_version=for_py_version,
        embed_filename=current.path,
        app_data=app_data,
        search_dirs=[],
        periodic=False,
    )
    msg = "upgraded %s for python %s in %s {}".format(
        "new entries found:\n%s" if versions else "no new versions found",
    )
    args = [
        distribution,
        for_py_version,
        datetime.now() - start,
    ]
    if versions:
        args.append("\n".join("\t{}".format(v) for v in versions))
    logging.warning(msg, *args)


__all__ = (
    "periodic_update",
    "do_update",
    "manual_upgrade",
    "NewVersion",
    "UpdateLog",
    "load_datetime",
    "dump_datetime",
    "trigger_update",
    "release_date_for_wheel_path",
)
