from __future__ import absolute_import, unicode_literals

from tox.interpreters.via_path import get_python_info

from .command import CommandLog


class EnvLog(object):
    """Report the status of a tox environment"""

    def __init__(self, result_log, name, dict):
        self.reportlog = result_log
        self.name = name
        self.dict = dict

    def set_python_info(self, python_executable):
        answer = get_python_info(str(python_executable))
        answer["executable"] = python_executable
        self.dict["python"] = answer

    def get_commandlog(self, name):
        """get the command log for a given group name"""
        data = self.dict.setdefault(name, [])
        return CommandLog(self, data)

    def set_installed(self, packages):
        self.dict["installed_packages"] = packages

    def set_header(self, installpkg):
        """
        :param py.path.local installpkg: Path ot the package.
        """
        self.dict["installpkg"] = {
            "sha256": installpkg.computehash("sha256"),
            "basename": installpkg.basename,
        }
