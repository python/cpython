"""
Automatically package and test a Python project against configurable
Python2 and Python3 based virtual environments. Environments are
setup by using virtualenv. Configuration is generally done through an
INI-style "tox.ini" file.
"""
from __future__ import absolute_import, unicode_literals

import json
import os
import re
import subprocess
import sys
from collections import OrderedDict
from contextlib import contextmanager

import py

import tox
from tox import reporter
from tox.action import Action
from tox.config import INTERRUPT_TIMEOUT, SUICIDE_TIMEOUT, TERMINATE_TIMEOUT, parseconfig
from tox.config.parallel import ENV_VAR_KEY_PRIVATE as PARALLEL_ENV_VAR_KEY_PRIVATE
from tox.config.parallel import OFF_VALUE as PARALLEL_OFF
from tox.logs.result import ResultLog
from tox.reporter import update_default_reporter
from tox.util import set_os_env_var
from tox.util.graph import stable_topological_sort
from tox.util.stdlib import suppress_output
from tox.venv import VirtualEnv

from .commands.help import show_help
from .commands.help_ini import show_help_ini
from .commands.provision import provision_tox
from .commands.run.parallel import run_parallel
from .commands.run.sequential import run_sequential
from .commands.show_config import show_config
from .commands.show_env import show_envs


def cmdline(args=None):
    if args is None:
        args = sys.argv[1:]
    main(args)


def setup_reporter(args):
    from argparse import ArgumentParser

    from tox.config.reporter import add_verbosity_commands

    parser = ArgumentParser(add_help=False)
    add_verbosity_commands(parser)
    with suppress_output():
        try:
            options, _ = parser.parse_known_args(args)
            update_default_reporter(options.quiet_level, options.verbose_level)
        except SystemExit:
            pass


def main(args):
    setup_reporter(args)
    try:
        config = load_config(args)
        config.logdir.ensure(dir=1)
        with set_os_env_var(str("TOX_WORK_DIR"), config.toxworkdir):
            session = build_session(config)
            exit_code = session.runcommand()
        if exit_code is None:
            exit_code = 0
        raise SystemExit(exit_code)
    except tox.exception.BadRequirement:
        raise SystemExit(1)
    except KeyboardInterrupt:
        raise SystemExit(2)


def load_config(args):
    try:
        config = parseconfig(args)
        if config.option.help:
            show_help(config)
            raise SystemExit(0)
        elif config.option.helpini:
            show_help_ini(config)
            raise SystemExit(0)
    except tox.exception.MissingRequirement as exception:
        config = exception.config
    return config


def build_session(config):
    return Session(config)


class Session(object):
    """The session object that ties together configuration, reporting, venv creation, testing."""

    def __init__(self, config, popen=subprocess.Popen):
        self._reset(config, popen)

    def _reset(self, config, popen=subprocess.Popen):
        self.config = config
        self.popen = popen
        self.resultlog = ResultLog()
        self.existing_venvs = OrderedDict()
        self.venv_dict = {} if self.config.run_provision else self._build_venvs()

    def _build_venvs(self):
        try:
            need_to_run = OrderedDict((v, self.getvenv(v)) for v in self._evaluated_env_list)
            try:
                venv_order = stable_topological_sort(
                    OrderedDict((name, v.envconfig.depends) for name, v in need_to_run.items()),
                )

                venvs = OrderedDict((v, need_to_run[v]) for v in venv_order)
                return venvs
            except ValueError as exception:
                reporter.error("circular dependency detected: {}".format(exception))
        except LookupError:
            pass
        except tox.exception.ConfigError as exception:
            reporter.error(str(exception))
        raise SystemExit(1)

    def getvenv(self, name):
        if name in self.existing_venvs:
            return self.existing_venvs[name]
        env_config = self.config.envconfigs.get(name, None)
        if env_config is None:
            reporter.error("unknown environment {!r}".format(name))
            raise LookupError(name)
        elif env_config.envdir == self.config.toxinidir:
            reporter.error("venv {!r} in {} would delete project".format(name, env_config.envdir))
            raise tox.exception.ConfigError("envdir must not equal toxinidir")
        env_log = self.resultlog.get_envlog(name)
        venv = VirtualEnv(envconfig=env_config, popen=self.popen, env_log=env_log)
        self.existing_venvs[name] = venv
        return venv

    @property
    def _evaluated_env_list(self):
        tox_env_filter = os.environ.get("TOX_SKIP_ENV")
        tox_env_filter_re = re.compile(tox_env_filter) if tox_env_filter is not None else None
        visited = set()
        for name in self.config.envlist:
            if name in visited:
                continue
            visited.add(name)
            if tox_env_filter_re is not None and tox_env_filter_re.match(name):
                msg = "skip environment {}, matches filter {!r}".format(
                    name,
                    tox_env_filter_re.pattern,
                )
                reporter.verbosity1(msg)
                continue
            yield name

    @property
    def hook(self):
        return self.config.pluginmanager.hook

    def newaction(self, name, msg, *args):
        return Action(
            name,
            msg,
            args,
            self.config.logdir,
            self.config.option.resultjson,
            self.resultlog.command_log,
            self.popen,
            sys.executable,
            SUICIDE_TIMEOUT,
            INTERRUPT_TIMEOUT,
            TERMINATE_TIMEOUT,
        )

    def runcommand(self):
        reporter.using(
            "tox-{} from {} (pid {})".format(tox.__version__, tox.__file__, os.getpid()),
        )
        show_description = reporter.has_level(reporter.Verbosity.DEFAULT)
        if self.config.run_provision:
            provision_tox_venv = self.getvenv(self.config.provision_tox_env)
            return provision_tox(provision_tox_venv, self.config.args)
        else:
            if self.config.option.showconfig:
                self.showconfig()
            elif self.config.option.listenvs:
                self.showenvs(all_envs=False, description=show_description)
            elif self.config.option.listenvs_all:
                self.showenvs(all_envs=True, description=show_description)
            else:
                with self.cleanup():
                    return self.subcommand_test()

    @contextmanager
    def cleanup(self):
        self.config.temp_dir.ensure(dir=True)
        try:
            yield
        finally:
            self.hook.tox_cleanup(session=self)

    def subcommand_test(self):
        if self.config.skipsdist:
            reporter.info("skipping sdist step")
        else:
            for venv in self.venv_dict.values():
                if not venv.envconfig.skip_install:
                    venv.package = self.hook.tox_package(session=self, venv=venv)
                    if not venv.package:
                        return 2
                    venv.envconfig.setenv[str("TOX_PACKAGE")] = str(venv.package)
        if self.config.option.sdistonly:
            return

        within_parallel = PARALLEL_ENV_VAR_KEY_PRIVATE in os.environ
        try:
            if not within_parallel and self.config.option.parallel != PARALLEL_OFF:
                run_parallel(self.config, self.venv_dict)
            else:
                run_sequential(self.config, self.venv_dict)
        finally:
            retcode = self._summary()
        return retcode

    def _add_parallel_summaries(self):
        if self.config.option.parallel != PARALLEL_OFF and "testenvs" in self.resultlog.dict:
            result_log = self.resultlog.dict["testenvs"]
            for tox_env in self.venv_dict.values():
                data = self._load_parallel_env_report(tox_env)
                if data and "testenvs" in data and tox_env.name in data["testenvs"]:
                    result_log[tox_env.name] = data["testenvs"][tox_env.name]

    @staticmethod
    def _load_parallel_env_report(tox_env):
        """Load report data into memory, remove disk file"""
        result_json_path = tox_env.get_result_json_path()
        if result_json_path and result_json_path.exists():
            with result_json_path.open("r") as file_handler:
                data = json.load(file_handler)
            result_json_path.remove()
            return data

    def _summary(self):
        is_parallel_child = PARALLEL_ENV_VAR_KEY_PRIVATE in os.environ
        if not is_parallel_child:
            reporter.separator("_", "summary", reporter.Verbosity.QUIET)
        exit_code = 0
        for venv in self.venv_dict.values():
            report = reporter.good
            status = getattr(venv, "status", "undefined")
            if isinstance(status, tox.exception.InterpreterNotFound):
                msg = " {}: {}".format(venv.envconfig.envname, str(status))
                if self.config.option.skip_missing_interpreters == "true":
                    report = reporter.skip
                else:
                    exit_code = 1
                    report = reporter.error
            elif status == "platform mismatch":
                msg = " {}: {} ({!r} does not match {!r})".format(
                    venv.envconfig.envname,
                    str(status),
                    sys.platform,
                    venv.envconfig.platform,
                )
                report = reporter.skip
            elif status and status == "ignored failed command":
                msg = "  {}: {}".format(venv.envconfig.envname, str(status))
            elif status and status != "skipped tests":
                msg = "  {}: {}".format(venv.envconfig.envname, str(status))
                report = reporter.error
                exit_code = 1
            else:
                if not status:
                    status = "commands succeeded"
                msg = "  {}: {}".format(venv.envconfig.envname, status)
            if not is_parallel_child:
                report(msg)
        if not exit_code and not is_parallel_child:
            reporter.good("  congratulations :)")
        path = self.config.option.resultjson
        if path:
            if not is_parallel_child:
                self._add_parallel_summaries()
            path = py.path.local(path)
            data = self.resultlog.dumps_json()
            reporter.line("write json report at: {}".format(path))
            path.write(data)
        return exit_code

    def showconfig(self):
        show_config(self.config)

    def showenvs(self, all_envs=False, description=False):
        show_envs(self.config, all_envs=all_envs, description=description)
