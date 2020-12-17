"""
Application data stored by virtualenv.
"""
from __future__ import absolute_import, unicode_literals

import logging
import os

from appdirs import user_data_dir

from .na import AppDataDisabled
from .read_only import ReadOnlyAppData
from .via_disk_folder import AppDataDiskFolder
from .via_tempdir import TempAppData


def _default_app_data_dir():  # type: () -> str
    key = str("VIRTUALENV_OVERRIDE_APP_DATA")
    if key in os.environ:
        return os.environ[key]
    else:
        return user_data_dir(appname="virtualenv", appauthor="pypa")


def make_app_data(folder, **kwargs):
    read_only = kwargs.pop("read_only")
    if kwargs:  # py3+ kwonly
        raise TypeError("unexpected keywords: {}")

    if folder is None:
        folder = _default_app_data_dir()
    folder = os.path.abspath(folder)

    if read_only:
        return ReadOnlyAppData(folder)

    if not os.path.isdir(folder):
        try:
            os.makedirs(folder)
            logging.debug("created app data folder %s", folder)
        except OSError as exception:
            logging.info("could not create app data folder %s due to %r", folder, exception)

    if os.access(folder, os.W_OK):
        return AppDataDiskFolder(folder)
    else:
        logging.debug("app data folder %s has no write access", folder)
        return TempAppData()


__all__ = (
    "AppDataDisabled",
    "AppDataDiskFolder",
    "ReadOnlyAppData",
    "TempAppData",
    "make_app_data",
)
