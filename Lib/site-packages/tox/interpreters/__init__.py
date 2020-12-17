from __future__ import unicode_literals

import json
import sys

import tox
from tox import reporter
from tox.constants import SITE_PACKAGE_QUERY_SCRIPT
from tox.interpreters.via_path import get_python_info


class Interpreters:
    def __init__(self, hook):
        self.name2executable = {}
        self.executable2info = {}
        self.hook = hook

    def get_executable(self, envconfig):
        """return path object to the executable for the given
        name (e.g. python2.7, python3.6, python etc.)
        if name is already an existing path, return name.
        If an interpreter cannot be found, return None.
        """
        try:
            return self.name2executable[envconfig.envname]
        except KeyError:
            exe = self.hook.tox_get_python_executable(envconfig=envconfig)
            reporter.verbosity2("{} uses {}".format(envconfig.envname, exe))
            self.name2executable[envconfig.envname] = exe
            return exe

    def get_info(self, envconfig):
        executable = self.get_executable(envconfig)
        name = envconfig.basepython
        if not executable:
            return NoInterpreterInfo(name=name)
        try:
            return self.executable2info[executable]
        except KeyError:
            info = run_and_get_interpreter_info(name, executable)
            self.executable2info[executable] = info
            return info

    def get_sitepackagesdir(self, info, envdir):
        if not info.executable:
            return ""
        envdir = str(envdir)
        try:
            res = exec_on_interpreter(str(info.executable), SITE_PACKAGE_QUERY_SCRIPT, str(envdir))
        except ExecFailed as e:
            reporter.verbosity1("execution failed: {} -- {}".format(e.out, e.err))
            return ""
        else:
            return res["dir"]


def run_and_get_interpreter_info(name, executable):
    assert executable
    try:
        result = get_python_info(str(executable))
        result["version_info"] = tuple(result["version_info"])  # fix json dump transformation
        if result["extra_version_info"] is not None:
            result["extra_version_info"] = tuple(
                result["extra_version_info"],
            )  # fix json dump transformation
        del result["version"]
        result["executable"] = str(executable)
    except ExecFailed as e:
        return NoInterpreterInfo(name, executable=e.executable, out=e.out, err=e.err)
    else:
        return InterpreterInfo(**result)


def exec_on_interpreter(*args):
    from subprocess import PIPE, Popen

    popen = Popen(args, stdout=PIPE, stderr=PIPE, universal_newlines=True)
    out, err = popen.communicate()
    if popen.returncode:
        raise ExecFailed(args[0], args[1:], out, err)
    if err:
        sys.stderr.write(err)
    try:
        result = json.loads(out)
    except Exception:
        raise ExecFailed(args[0], args[1:], out, "could not decode {!r}".format(out))
    return result


class ExecFailed(Exception):
    def __init__(self, executable, source, out, err):
        self.executable = executable
        self.source = source
        self.out = out
        self.err = err


class InterpreterInfo:
    def __init__(
        self,
        implementation,
        executable,
        version_info,
        sysplatform,
        is_64,
        extra_version_info,
    ):
        self.implementation = implementation
        self.executable = executable

        self.version_info = version_info
        self.sysplatform = sysplatform
        self.is_64 = is_64
        self.extra_version_info = extra_version_info

    def __str__(self):
        return "<executable at {}, version_info {}>".format(self.executable, self.version_info)


class NoInterpreterInfo:
    def __init__(self, name, executable=None, out=None, err="not found"):
        self.name = name
        self.executable = executable
        self.version_info = None
        self.out = out
        self.err = err

    def __str__(self):
        if self.executable:
            return "<executable at {}, not runnable>".format(self.executable)
        else:
            return "<executable not found for: {}>".format(self.name)


if tox.INFO.IS_WIN:
    from .windows import tox_get_python_executable
else:
    from .unix import tox_get_python_executable
assert tox_get_python_executable
