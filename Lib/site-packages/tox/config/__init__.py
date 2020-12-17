from __future__ import print_function

import argparse
import itertools
import os
import random
import re
import shlex
import string
import sys
import traceback
import warnings
from collections import OrderedDict
from fnmatch import fnmatchcase
from subprocess import list2cmdline
from threading import Thread

import pluggy
import py
import toml
from packaging import requirements
from packaging.utils import canonicalize_name

import tox
from tox.constants import INFO
from tox.exception import MissingDependency
from tox.interpreters import Interpreters, NoInterpreterInfo
from tox.reporter import (
    REPORTER_TIMESTAMP_ON_ENV,
    error,
    update_default_reporter,
    using,
    verbosity1,
)
from tox.util.path import ensure_empty_dir
from tox.util.stdlib import importlib_metadata

from .parallel import ENV_VAR_KEY_PRIVATE as PARALLEL_ENV_VAR_KEY_PRIVATE
from .parallel import ENV_VAR_KEY_PUBLIC as PARALLEL_ENV_VAR_KEY_PUBLIC
from .parallel import add_parallel_config, add_parallel_flags
from .reporter import add_verbosity_commands

try:
    from shlex import quote as shlex_quote
except ImportError:
    from pipes import quote as shlex_quote


hookimpl = tox.hookimpl
# DEPRECATED - REMOVE - left for compatibility with plugins importing from here.
# Import hookimpl directly from tox instead.


WITHIN_PROVISION = os.environ.get(str("TOX_PROVISION")) == "1"

SUICIDE_TIMEOUT = 0.0
INTERRUPT_TIMEOUT = 0.3
TERMINATE_TIMEOUT = 0.2

_FACTOR_LINE_PATTERN = re.compile(r"^([\w{}\.!,-]+)\:\s+(.+)")
_ENVSTR_SPLIT_PATTERN = re.compile(r"((?:\{[^}]+\})+)|,")
_ENVSTR_EXPAND_PATTERN = re.compile(r"\{([^}]+)\}")
_WHITESPACE_PATTERN = re.compile(r"\s+")


def get_plugin_manager(plugins=()):
    # initialize plugin manager
    import tox.venv

    pm = pluggy.PluginManager("tox")
    pm.add_hookspecs(tox.hookspecs)
    pm.register(tox.config)
    pm.register(tox.interpreters)
    pm.register(tox.venv)
    pm.register(tox.session)
    from tox import package

    pm.register(package)
    pm.load_setuptools_entrypoints("tox")
    for plugin in plugins:
        pm.register(plugin)
    pm.check_pending()
    return pm


class Parser:
    """Command line and ini-parser control object."""

    def __init__(self):
        class HelpFormatter(argparse.ArgumentDefaultsHelpFormatter):
            def __init__(self, prog):
                super(HelpFormatter, self).__init__(prog, max_help_position=35, width=190)

        self.argparser = argparse.ArgumentParser(
            description="tox options",
            add_help=False,
            prog="tox",
            formatter_class=HelpFormatter,
        )
        self._testenv_attr = []

    def add_argument(self, *args, **kwargs):
        """add argument to command line parser.  This takes the
        same arguments that ``argparse.ArgumentParser.add_argument``.
        """
        return self.argparser.add_argument(*args, **kwargs)

    def add_testenv_attribute(self, name, type, help, default=None, postprocess=None):
        """add an ini-file variable for "testenv" section.

        Types are specified as strings like "bool", "line-list", "string", "argv", "path",
        "argvlist".

        The ``postprocess`` function will be called for each testenv
        like ``postprocess(testenv_config=testenv_config, value=value)``
        where ``value`` is the value as read from the ini (or the default value)
        and ``testenv_config`` is a :py:class:`tox.config.TestenvConfig` instance
        which will receive all ini-variables as object attributes.

        Any postprocess function must return a value which will then be set
        as the final value in the testenv section.
        """
        self._testenv_attr.append(VenvAttribute(name, type, default, help, postprocess))

    def add_testenv_attribute_obj(self, obj):
        """add an ini-file variable as an object.

        This works as the ``add_testenv_attribute`` function but expects
        "name", "type", "help", and "postprocess" attributes on the object.
        """
        assert hasattr(obj, "name")
        assert hasattr(obj, "type")
        assert hasattr(obj, "help")
        assert hasattr(obj, "postprocess")
        self._testenv_attr.append(obj)

    def parse_cli(self, args, strict=False):
        args, argv = self.argparser.parse_known_args(args)
        if argv and (strict or WITHIN_PROVISION):
            self.argparser.error("unrecognized arguments: {}".format(" ".join(argv)))
        return args

    def _format_help(self):
        return self.argparser.format_help()


class VenvAttribute:
    def __init__(self, name, type, default, help, postprocess):
        self.name = name
        self.type = type
        self.default = default
        self.help = help
        self.postprocess = postprocess


class DepOption:
    name = "deps"
    type = "line-list"
    help = "each line specifies a dependency in pip/setuptools format."
    default = ()

    def postprocess(self, testenv_config, value):
        deps = []
        config = testenv_config.config
        for depline in value:
            m = re.match(r":(\w+):\s*(\S+)", depline)
            if m:
                iname, name = m.groups()
                ixserver = config.indexserver[iname]
            else:
                name = depline.strip()
                ixserver = None
                # we need to process options, in case they contain a space,
                # as the subprocess call to pip install will otherwise fail.
                # in case of a short option, we remove the space
                for option in tox.PIP.INSTALL_SHORT_OPTIONS_ARGUMENT:
                    if name.startswith(option):
                        name = "{}{}".format(option, name[len(option) :].strip())
                # in case of a long option, we add an equal sign
                for option in tox.PIP.INSTALL_LONG_OPTIONS_ARGUMENT:
                    name_start = "{} ".format(option)
                    if name.startswith(name_start):
                        name = "{}={}".format(option, name[len(option) :].strip())
            name = self._cut_off_dep_comment(name)
            name = self._replace_forced_dep(name, config)
            deps.append(DepConfig(name, ixserver))
        return deps

    def _replace_forced_dep(self, name, config):
        """Override given dependency config name. Take ``--force-dep-version`` option into account.

        :param name: dep config, for example ["pkg==1.0", "other==2.0"].
        :param config: ``Config`` instance
        :return: the new dependency that should be used for virtual environments
        """
        if not config.option.force_dep:
            return name
        for forced_dep in config.option.force_dep:
            if self._is_same_dep(forced_dep, name):
                return forced_dep
        return name

    @staticmethod
    def _cut_off_dep_comment(name):
        return re.sub(r"\s+#.*", "", name).strip()

    @classmethod
    def _is_same_dep(cls, dep1, dep2):
        """Definitions are the same if they refer to the same package, even if versions differ."""
        dep1_name = canonicalize_name(requirements.Requirement(dep1).name)
        try:
            dep2_name = canonicalize_name(requirements.Requirement(dep2).name)
        except requirements.InvalidRequirement:
            # we couldn't parse a version, probably a URL
            return False
        return dep1_name == dep2_name


class PosargsOption:
    name = "args_are_paths"
    type = "bool"
    default = True
    help = "treat positional args in commands as paths"

    def postprocess(self, testenv_config, value):
        config = testenv_config.config
        args = config.option.args
        if args:
            if value:
                args = []
                for arg in config.option.args:
                    if arg and not os.path.isabs(arg):
                        origpath = os.path.join(config.invocationcwd.strpath, arg)
                        if os.path.exists(origpath):
                            arg = os.path.relpath(origpath, testenv_config.changedir.strpath)
                    args.append(arg)
            testenv_config._reader.addsubstitutions(args)
        return value


class InstallcmdOption:
    name = "install_command"
    type = "argv"
    default = "python -m pip install {opts} {packages}"
    help = "install command for dependencies and package under test."

    def postprocess(self, testenv_config, value):
        if "{packages}" not in value:
            raise tox.exception.ConfigError(
                "'install_command' must contain '{packages}' substitution",
            )
        return value


def parseconfig(args, plugins=()):
    """Parse the configuration file and create a Config object.

    :param plugins:
    :param list[str] args: list of arguments.
    :rtype: :class:`Config`
    :raise SystemExit: toxinit file is not found
    """
    pm = get_plugin_manager(plugins)
    config, option = parse_cli(args, pm)
    update_default_reporter(config.option.quiet_level, config.option.verbose_level)

    for config_file in propose_configs(option.configfile):
        config_type = config_file.basename

        content = None
        if config_type == "pyproject.toml":
            toml_content = get_py_project_toml(config_file)
            try:
                content = toml_content["tool"]["tox"]["legacy_tox_ini"]
            except KeyError:
                continue
        try:
            ParseIni(config, config_file, content)
        except SkipThisIni:
            continue
        pm.hook.tox_configure(config=config)  # post process config object
        break
    else:
        parser = Parser()
        pm.hook.tox_addoption(parser=parser)
        # if no tox config file, now we need do a strict argument evaluation
        # raise on unknown args
        parser.parse_cli(args, strict=True)
        if option.help or option.helpini:
            return config
        if option.devenv:
            # To load defaults, we parse an empty config
            ParseIni(config, py.path.local(), "")
            pm.hook.tox_configure(config=config)
            return config
        msg = "tox config file (either {}) not found"
        candidates = ", ".join(INFO.CONFIG_CANDIDATES)
        feedback(msg.format(candidates), sysexit=not (option.help or option.helpini))
    return config


def get_py_project_toml(path):
    with open(str(path)) as file_handler:
        config_data = toml.load(file_handler)
        return config_data


def propose_configs(cli_config_file):
    from_folder = py.path.local()
    if cli_config_file is not None:
        if os.path.isfile(cli_config_file):
            yield py.path.local(cli_config_file)
            return
        if os.path.isdir(cli_config_file):
            from_folder = py.path.local(cli_config_file)
        else:
            print(
                "ERROR: {} is neither file or directory".format(cli_config_file),
                file=sys.stderr,
            )
            return
    for basename in INFO.CONFIG_CANDIDATES:
        if from_folder.join(basename).isfile():
            yield from_folder.join(basename)
        for path in from_folder.parts(reverse=True):
            ini_path = path.join(basename)
            if ini_path.check():
                yield ini_path


def parse_cli(args, pm):
    parser = Parser()
    pm.hook.tox_addoption(parser=parser)
    option = parser.parse_cli(args)
    if option.version:
        print(get_version_info(pm))
        raise SystemExit(0)
    interpreters = Interpreters(hook=pm.hook)
    config = Config(
        pluginmanager=pm,
        option=option,
        interpreters=interpreters,
        parser=parser,
        args=args,
    )
    return config, option


def feedback(msg, sysexit=False):
    print("ERROR: {}".format(msg), file=sys.stderr)
    if sysexit:
        raise SystemExit(1)


def get_version_info(pm):
    out = ["{} imported from {}".format(tox.__version__, tox.__file__)]
    plugin_dist_info = pm.list_plugin_distinfo()
    if plugin_dist_info:
        out.append("registered plugins:")
        for mod, egg_info in plugin_dist_info:
            source = getattr(mod, "__file__", repr(mod))
            out.append("    {}-{} at {}".format(egg_info.project_name, egg_info.version, source))
    return "\n".join(out)


class SetenvDict(object):
    _DUMMY = object()

    def __init__(self, definitions, reader):
        self.definitions = definitions
        self.reader = reader
        self.resolved = {}
        self._lookupstack = []

    def __repr__(self):
        return "{}: {}".format(self.__class__.__name__, self.definitions)

    def __contains__(self, name):
        return name in self.definitions

    def get(self, name, default=None):
        try:
            return self.resolved[name]
        except KeyError:
            try:
                if name in self._lookupstack:
                    raise KeyError(name)
                val = self.definitions[name]
            except KeyError:
                return os.environ.get(name, default)
            self._lookupstack.append(name)
            try:
                res = self.reader._replace(val)
                res = res.replace("\\{", "{").replace("\\}", "}")
                self.resolved[name] = res
            finally:
                self._lookupstack.pop()
            return res

    def __getitem__(self, name):
        x = self.get(name, self._DUMMY)
        if x is self._DUMMY:
            raise KeyError(name)
        return x

    def keys(self):
        return self.definitions.keys()

    def __setitem__(self, name, value):
        self.definitions[name] = value
        self.resolved[name] = value


@tox.hookimpl
def tox_addoption(parser):
    parser.add_argument(
        "--version",
        action="store_true",
        help="report version information to stdout.",
    )
    parser.add_argument("-h", "--help", action="store_true", help="show help about options")
    parser.add_argument(
        "--help-ini",
        "--hi",
        action="store_true",
        dest="helpini",
        help="show help about ini-names",
    )
    add_verbosity_commands(parser)
    parser.add_argument(
        "--showconfig",
        action="store_true",
        help="show live configuration (by default all env, with -l only default targets,"
        " specific via TOXENV/-e)",
    )
    parser.add_argument(
        "-l",
        "--listenvs",
        action="store_true",
        help="show list of test environments (with description if verbose)",
    )
    parser.add_argument(
        "-a",
        "--listenvs-all",
        action="store_true",
        help="show list of all defined environments (with description if verbose)",
    )
    parser.add_argument(
        "-c",
        dest="configfile",
        help="config file name or directory with 'tox.ini' file.",
    )
    parser.add_argument(
        "-e",
        action="append",
        dest="env",
        metavar="envlist",
        help="work against specified environments (ALL selects all).",
    )
    parser.add_argument(
        "--devenv",
        metavar="ENVDIR",
        help=(
            "sets up a development environment at ENVDIR based on the env's tox "
            "configuration specified by `-e` (-e defaults to py)."
        ),
    )
    parser.add_argument("--notest", action="store_true", help="skip invoking test commands.")
    parser.add_argument(
        "--sdistonly",
        action="store_true",
        help="only perform the sdist packaging activity.",
    )
    parser.add_argument(
        "--skip-pkg-install",
        action="store_true",
        help="skip package installation for this run",
    )
    add_parallel_flags(parser)
    parser.add_argument(
        "--parallel--safe-build",
        action="store_true",
        dest="parallel_safe_build",
        help="(deprecated) ensure two tox builds can run in parallel "
        "(uses a lock file in the tox workdir with .lock extension)",
    )
    parser.add_argument(
        "--installpkg",
        metavar="PATH",
        help="use specified package for installation into venv, instead of creating an sdist.",
    )
    parser.add_argument(
        "--develop",
        action="store_true",
        help="install package in the venv using 'setup.py develop' via 'pip -e .'",
    )
    parser.add_argument(
        "-i",
        "--index-url",
        action="append",
        dest="indexurl",
        metavar="URL",
        help="set indexserver url (if URL is of form name=url set the "
        "url for the 'name' indexserver, specifically)",
    )
    parser.add_argument(
        "--pre",
        action="store_true",
        help="install pre-releases and development versions of dependencies. "
        "This will pass the --pre option to install_command "
        "(pip by default).",
    )
    parser.add_argument(
        "-r",
        "--recreate",
        action="store_true",
        help="force recreation of virtual environments",
    )
    parser.add_argument(
        "--result-json",
        dest="resultjson",
        metavar="PATH",
        help="write a json file with detailed information "
        "about all commands and results involved.",
    )
    parser.add_argument(
        "--discover",
        dest="discover",
        nargs="+",
        metavar="PATH",
        help="for python discovery first try the python executables under these paths",
        default=[],
    )

    # We choose 1 to 4294967295 because it is the range of PYTHONHASHSEED.
    parser.add_argument(
        "--hashseed",
        metavar="SEED",
        help="set PYTHONHASHSEED to SEED before running commands.  "
        "Defaults to a random integer in the range [1, 4294967295] "
        "([1, 1024] on Windows). "
        "Passing 'noset' suppresses this behavior.",
    )
    parser.add_argument(
        "--force-dep",
        action="append",
        metavar="REQ",
        help="Forces a certain version of one of the dependencies "
        "when configuring the virtual environment. REQ Examples "
        "'pytest<2.7' or 'django>=1.6'.",
    )
    parser.add_argument(
        "--sitepackages",
        action="store_true",
        help="override sitepackages setting to True in all envs",
    )
    parser.add_argument(
        "--alwayscopy",
        action="store_true",
        help="override alwayscopy setting to True in all envs",
    )

    cli_skip_missing_interpreter(parser)
    parser.add_argument("--workdir", metavar="PATH", help="tox working directory")

    parser.add_argument(
        "args",
        nargs="*",
        help="additional arguments available to command positional substitution",
    )

    def _set_envdir_from_devenv(testenv_config, value):
        if testenv_config.config.option.devenv is not None:
            return py.path.local(testenv_config.config.option.devenv)
        else:
            return value

    parser.add_testenv_attribute(
        name="envdir",
        type="path",
        default="{toxworkdir}/{envname}",
        help="set venv directory -- be very careful when changing this as tox "
        "will remove this directory when recreating an environment",
        postprocess=_set_envdir_from_devenv,
    )

    # add various core venv interpreter attributes
    def setenv(testenv_config, value):
        setenv = value
        config = testenv_config.config
        if "PYTHONHASHSEED" not in setenv and config.hashseed is not None:
            setenv["PYTHONHASHSEED"] = config.hashseed

        setenv["TOX_ENV_NAME"] = str(testenv_config.envname)
        setenv["TOX_ENV_DIR"] = str(testenv_config.envdir)
        return setenv

    parser.add_testenv_attribute(
        name="setenv",
        type="dict_setenv",
        postprocess=setenv,
        help="list of X=Y lines with environment variable settings",
    )

    def basepython_default(testenv_config, value):
        """either user set or proposed from the factor name

        in both cases we check that the factor name implied python version and the resolved
        python interpreter version match up; if they don't we warn, unless ignore base
        python conflict is set in which case the factor name implied version if forced
        """
        for factor in testenv_config.factors:
            match = tox.PYTHON.PY_FACTORS_RE.match(factor)
            if match:
                base_exe = {"py": "python"}.get(match.group(1), match.group(1))
                version_s = match.group(2)
                if not version_s:
                    version_info = ()
                elif len(version_s) == 1:
                    version_info = (version_s,)
                else:
                    version_info = (version_s[0], version_s[1:])
                implied_version = ".".join(version_info)
                implied_python = "{}{}".format(base_exe, implied_version)
                break
        else:
            implied_python, version_info, implied_version = None, (), ""

        if testenv_config.config.ignore_basepython_conflict and implied_python is not None:
            return implied_python

        proposed_python = (implied_python or sys.executable) if value is None else str(value)
        if implied_python is not None and implied_python != proposed_python:
            testenv_config.basepython = proposed_python
            python_info_for_proposed = testenv_config.python_info
            if not isinstance(python_info_for_proposed, NoInterpreterInfo):
                proposed_version = ".".join(
                    str(x) for x in python_info_for_proposed.version_info[: len(version_info)]
                )
                if proposed_version != implied_version:
                    # TODO(stephenfin): Raise an exception here in tox 4.0
                    warnings.warn(
                        "conflicting basepython version (set {}, should be {}) for env '{}';"
                        "resolve conflict or set ignore_basepython_conflict".format(
                            proposed_version,
                            implied_version,
                            testenv_config.envname,
                        ),
                    )

        return proposed_python

    parser.add_testenv_attribute(
        name="basepython",
        type="basepython",
        default=None,
        postprocess=basepython_default,
        help="executable name or path of interpreter used to create a virtual test environment.",
    )

    def merge_description(testenv_config, value):
        """the reader by default joins generated description with new line,
        replace new line with space"""
        return value.replace("\n", " ")

    parser.add_testenv_attribute(
        name="description",
        type="string",
        default="",
        postprocess=merge_description,
        help="short description of this environment",
    )

    parser.add_testenv_attribute(
        name="envtmpdir",
        type="path",
        default="{envdir}/tmp",
        help="venv temporary directory",
    )

    parser.add_testenv_attribute(
        name="envlogdir",
        type="path",
        default="{envdir}/log",
        help="venv log directory",
    )

    parser.add_testenv_attribute(
        name="downloadcache",
        type="string",
        default=None,
        help="(ignored) has no effect anymore, pip-8 uses local caching by default",
    )

    parser.add_testenv_attribute(
        name="changedir",
        type="path",
        default="{toxinidir}",
        help="directory to change to when running commands",
    )

    parser.add_testenv_attribute_obj(PosargsOption())

    def skip_install_default(testenv_config, value):
        return value is True or testenv_config.config.option.skip_pkg_install is True

    parser.add_testenv_attribute(
        name="skip_install",
        type="bool",
        default=False,
        postprocess=skip_install_default,
        help="Do not install the current package. This can be used when you need the virtualenv "
        "management but do not want to install the current package",
    )

    parser.add_testenv_attribute(
        name="ignore_errors",
        type="bool",
        default=False,
        help="if set to True all commands will be executed irrespective of their result error "
        "status.",
    )

    def recreate(testenv_config, value):
        if testenv_config.config.option.recreate:
            return True
        return value

    parser.add_testenv_attribute(
        name="recreate",
        type="bool",
        default=False,
        postprocess=recreate,
        help="always recreate this test environment.",
    )

    def passenv(testenv_config, value):
        # Flatten the list to deal with space-separated values.
        value = list(itertools.chain.from_iterable([x.split(" ") for x in value]))

        passenv = {
            "CURL_CA_BUNDLE",
            "LANG",
            "LANGUAGE",
            "LD_LIBRARY_PATH",
            "PATH",
            "PIP_INDEX_URL",
            "PIP_EXTRA_INDEX_URL",
            "REQUESTS_CA_BUNDLE",
            "SSL_CERT_FILE",
            "TOX_WORK_DIR",
            "HTTP_PROXY",
            "HTTPS_PROXY",
            "NO_PROXY",
            str(REPORTER_TIMESTAMP_ON_ENV),
            str(PARALLEL_ENV_VAR_KEY_PUBLIC),
        }

        # read in global passenv settings
        p = os.environ.get("TOX_TESTENV_PASSENV", None)
        if p is not None:
            env_values = [x for x in p.split() if x]
            value.extend(env_values)

        # we ensure that tmp directory settings are passed on
        # we could also set it to the per-venv "envtmpdir"
        # but this leads to very long paths when run with jenkins
        # so we just pass it on by default for now.
        if tox.INFO.IS_WIN:
            passenv.add("SYSTEMDRIVE")  # needed for pip6
            passenv.add("SYSTEMROOT")  # needed for python's crypto module
            passenv.add("PATHEXT")  # needed for discovering executables
            passenv.add("COMSPEC")  # needed for distutils cygwincompiler
            passenv.add("TEMP")
            passenv.add("TMP")
            # for `multiprocessing.cpu_count()` on Windows (prior to Python 3.4).
            passenv.add("NUMBER_OF_PROCESSORS")
            passenv.add("PROCESSOR_ARCHITECTURE")  # platform.machine()
            passenv.add("USERPROFILE")  # needed for `os.path.expanduser()`
            passenv.add("MSYSTEM")  # fixes #429
        else:
            passenv.add("TMPDIR")
        for spec in value:
            for name in os.environ:
                if fnmatchcase(name.upper(), spec.upper()):
                    passenv.add(name)
        return passenv

    parser.add_testenv_attribute(
        name="passenv",
        type="line-list",
        postprocess=passenv,
        help="environment variables needed during executing test commands (taken from invocation "
        "environment). Note that tox always  passes through some basic environment variables "
        "which are needed for basic functioning of the Python system. See --showconfig for the "
        "eventual passenv setting.",
    )

    parser.add_testenv_attribute(
        name="whitelist_externals",
        type="line-list",
        help="DEPRECATED: use allowlist_externals",
    )

    parser.add_testenv_attribute(
        name="allowlist_externals",
        type="line-list",
        help="each lines specifies a path or basename for which tox will not warn "
        "about it coming from outside the test environment.",
    )

    parser.add_testenv_attribute(
        name="platform",
        type="string",
        default=".*",
        help="regular expression which must match against ``sys.platform``. "
        "otherwise testenv will be skipped.",
    )

    def sitepackages(testenv_config, value):
        return testenv_config.config.option.sitepackages or value

    def alwayscopy(testenv_config, value):
        return testenv_config.config.option.alwayscopy or value

    parser.add_testenv_attribute(
        name="sitepackages",
        type="bool",
        default=False,
        postprocess=sitepackages,
        help="Set to ``True`` if you want to create virtual environments that also "
        "have access to globally installed packages.",
    )

    parser.add_testenv_attribute(
        "download",
        type="bool",
        default=False,
        help="download the latest pip, setuptools and wheel when creating the virtual"
        "environment (default is to use the one bundled in virtualenv)",
    )

    parser.add_testenv_attribute(
        name="alwayscopy",
        type="bool",
        default=False,
        postprocess=alwayscopy,
        help="Set to ``True`` if you want virtualenv to always copy files rather "
        "than symlinking.",
    )

    def pip_pre(testenv_config, value):
        return testenv_config.config.option.pre or value

    parser.add_testenv_attribute(
        name="pip_pre",
        type="bool",
        default=False,
        postprocess=pip_pre,
        help="If ``True``, adds ``--pre`` to the ``opts`` passed to the install command. ",
    )

    def develop(testenv_config, value):
        option = testenv_config.config.option
        return not option.installpkg and (value or option.develop or option.devenv is not None)

    parser.add_testenv_attribute(
        name="usedevelop",
        type="bool",
        postprocess=develop,
        default=False,
        help="install package in develop/editable mode",
    )

    parser.add_testenv_attribute_obj(InstallcmdOption())

    parser.add_testenv_attribute(
        name="list_dependencies_command",
        type="argv",
        default="python -m pip freeze",
        help="list dependencies for a virtual environment",
    )

    parser.add_testenv_attribute_obj(DepOption())

    parser.add_testenv_attribute(
        name="suicide_timeout",
        type="float",
        default=SUICIDE_TIMEOUT,
        help="timeout to allow process to exit before sending SIGINT",
    )

    parser.add_testenv_attribute(
        name="interrupt_timeout",
        type="float",
        default=INTERRUPT_TIMEOUT,
        help="timeout before sending SIGTERM after SIGINT",
    )

    parser.add_testenv_attribute(
        name="terminate_timeout",
        type="float",
        default=TERMINATE_TIMEOUT,
        help="timeout before sending SIGKILL after SIGTERM",
    )

    parser.add_testenv_attribute(
        name="commands",
        type="argvlist",
        default="",
        help="each line specifies a test command and can use substitution.",
    )

    parser.add_testenv_attribute(
        name="commands_pre",
        type="argvlist",
        default="",
        help="each line specifies a setup command action and can use substitution.",
    )

    parser.add_testenv_attribute(
        name="commands_post",
        type="argvlist",
        default="",
        help="each line specifies a teardown command and can use substitution.",
    )

    parser.add_testenv_attribute(
        "ignore_outcome",
        type="bool",
        default=False,
        help="if set to True a failing result of this testenv will not make "
        "tox fail, only a warning will be produced",
    )

    parser.add_testenv_attribute(
        "extras",
        type="line-list",
        help="list of extras to install with the source distribution or develop install",
    )

    add_parallel_config(parser)


def cli_skip_missing_interpreter(parser):
    class SkipMissingInterpreterAction(argparse.Action):
        def __call__(self, parser, namespace, values, option_string=None):
            value = "true" if values is None else values
            if value not in ("config", "true", "false"):
                raise argparse.ArgumentTypeError("value must be config, true or false")
            setattr(namespace, self.dest, value)

    parser.add_argument(
        "-s",
        "--skip-missing-interpreters",
        default="config",
        metavar="val",
        nargs="?",
        action=SkipMissingInterpreterAction,
        help="don't fail tests for missing interpreters: {config,true,false} choice",
    )


class Config(object):
    """Global Tox config object."""

    def __init__(self, pluginmanager, option, interpreters, parser, args):
        self.envconfigs = OrderedDict()
        """Mapping envname -> envconfig"""
        self.invocationcwd = py.path.local()
        self.interpreters = interpreters
        self.pluginmanager = pluginmanager
        self.option = option
        self._parser = parser
        self._testenv_attr = parser._testenv_attr
        self.args = args

        """option namespace containing all parsed command line options"""

    @property
    def homedir(self):
        homedir = get_homedir()
        if homedir is None:
            homedir = self.toxinidir  # FIXME XXX good idea?
        return homedir


class TestenvConfig:
    """Testenv Configuration object.

    In addition to some core attributes/properties this config object holds all
    per-testenv ini attributes as attributes, see "tox --help-ini" for an overview.
    """

    def __init__(self, envname, config, factors, reader):
        #: test environment name
        self.envname = envname
        #: global tox config object
        self.config = config
        #: set of factors
        self.factors = factors
        self._reader = reader
        self._missing_subs = []
        """Holds substitutions that could not be resolved.

        Pre 2.8.1 missing substitutions crashed with a ConfigError although this would not be a
        problem if the env is not part of the current testrun. So we need to remember this and
        check later when the testenv is actually run and crash only then.
        """

    def get_envbindir(self):
        """Path to directory where scripts/binaries reside."""
        is_bin = (
            isinstance(self.python_info, NoInterpreterInfo)
            or tox.INFO.IS_WIN is False
            or self.python_info.implementation == "Jython"
            or (
                tox.INFO.IS_WIN
                and self.python_info.implementation == "PyPy"
                and self.python_info.extra_version_info < (7, 3, 1)
            )
        )
        return self.envdir.join("bin" if is_bin else "Scripts")

    @property
    def envbindir(self):
        return self.get_envbindir()

    @property
    def envpython(self):
        """Path to python executable."""
        return self.get_envpython()

    def get_envpython(self):
        """ path to python/jython executable. """
        if "jython" in str(self.basepython):
            name = "jython"
        else:
            name = "python"
        return self.envbindir.join(name)

    def get_envsitepackagesdir(self):
        """Return sitepackagesdir of the virtualenv environment.

        NOTE: Only available during execution, not during parsing.
        """
        x = self.config.interpreters.get_sitepackagesdir(info=self.python_info, envdir=self.envdir)
        return x

    @property
    def python_info(self):
        """Return sitepackagesdir of the virtualenv environment."""
        return self.config.interpreters.get_info(envconfig=self)

    def getsupportedinterpreter(self):
        if tox.INFO.IS_WIN and self.basepython and "jython" in self.basepython:
            raise tox.exception.UnsupportedInterpreter(
                "Jython/Windows does not support installing scripts",
            )
        info = self.config.interpreters.get_info(envconfig=self)
        if not info.executable:
            raise tox.exception.InterpreterNotFound(self.basepython)
        if not info.version_info:
            raise tox.exception.InvocationError(
                "Failed to get version_info for {}: {}".format(info.name, info.err),
            )
        return info.executable


testenvprefix = "testenv:"


def get_homedir():
    try:
        return py.path.local._gethomedir()
    except Exception:
        return None


def make_hashseed():
    max_seed = 4294967295
    if tox.INFO.IS_WIN:
        max_seed = 1024
    return str(random.randint(1, max_seed))


class SkipThisIni(Exception):
    """Internal exception to indicate the parsed ini file should be skipped"""


class ParseIni(object):
    def __init__(self, config, ini_path, ini_data):  # noqa
        config.toxinipath = ini_path
        using("tox.ini: {} (pid {})".format(config.toxinipath, os.getpid()))
        config.toxinidir = config.toxinipath.dirpath() if ini_path.check(file=True) else ini_path

        self._cfg = py.iniconfig.IniConfig(config.toxinipath, ini_data)

        if ini_path.basename == "setup.cfg" and "tox:tox" not in self._cfg:
            verbosity1("Found no [tox:tox] section in setup.cfg, skipping.")
            raise SkipThisIni()

        previous_line_of = self._cfg.lineof

        self.expand_section_names(self._cfg)

        def line_of_default_to_zero(section, name=None):
            at = previous_line_of(section, name=name)
            if at is None:
                at = 0
            return at

        self._cfg.lineof = line_of_default_to_zero
        config._cfg = self._cfg
        self.config = config

        prefix = "tox" if ini_path.basename == "setup.cfg" else None
        fallbacksection = "tox:tox" if ini_path.basename == "setup.cfg" else "tox"

        context_name = getcontextname()
        if context_name == "jenkins":
            reader = SectionReader(
                "tox:jenkins",
                self._cfg,
                prefix=prefix,
                fallbacksections=[fallbacksection],
            )
            dist_share_default = "{toxworkdir}/distshare"
        elif not context_name:
            reader = SectionReader("tox", self._cfg, prefix=prefix)
            dist_share_default = "{homedir}/.tox/distshare"
        else:
            raise ValueError("invalid context")

        if config.option.hashseed is None:
            hash_seed = make_hashseed()
        elif config.option.hashseed == "noset":
            hash_seed = None
        else:
            hash_seed = config.option.hashseed
        config.hashseed = hash_seed

        reader.addsubstitutions(toxinidir=config.toxinidir, homedir=config.homedir)

        if config.option.workdir is None:
            config.toxworkdir = reader.getpath("toxworkdir", "{toxinidir}/.tox")
        else:
            config.toxworkdir = config.toxinidir.join(config.option.workdir, abs=True)

        if os.path.exists(str(config.toxworkdir)):
            config.toxworkdir = config.toxworkdir.realpath()

        reader.addsubstitutions(toxworkdir=config.toxworkdir)
        config.ignore_basepython_conflict = reader.getbool("ignore_basepython_conflict", False)

        config.distdir = reader.getpath("distdir", "{toxworkdir}/dist")

        reader.addsubstitutions(distdir=config.distdir)
        config.distshare = reader.getpath("distshare", dist_share_default)
        reader.addsubstitutions(distshare=config.distshare)
        config.temp_dir = reader.getpath("temp_dir", "{toxworkdir}/.tmp")
        reader.addsubstitutions(temp_dir=config.temp_dir)
        config.sdistsrc = reader.getpath("sdistsrc", None)
        config.setupdir = reader.getpath("setupdir", "{toxinidir}")
        config.logdir = config.toxworkdir.join("log")
        within_parallel = PARALLEL_ENV_VAR_KEY_PRIVATE in os.environ
        if not within_parallel and not WITHIN_PROVISION:
            ensure_empty_dir(config.logdir)

        # determine indexserver dictionary
        config.indexserver = {"default": IndexServerConfig("default")}
        prefix = "indexserver"
        for line in reader.getlist(prefix):
            name, url = map(lambda x: x.strip(), line.split("=", 1))
            config.indexserver[name] = IndexServerConfig(name, url)

        if config.option.skip_missing_interpreters == "config":
            val = reader.getbool("skip_missing_interpreters", False)
            config.option.skip_missing_interpreters = "true" if val else "false"

        override = False
        if config.option.indexurl:
            for url_def in config.option.indexurl:
                m = re.match(r"\W*(\w+)=(\S+)", url_def)
                if m is None:
                    url = url_def
                    name = "default"
                else:
                    name, url = m.groups()
                    if not url:
                        url = None
                if name != "ALL":
                    config.indexserver[name].url = url
                else:
                    override = url
        # let ALL override all existing entries
        if override:
            for name in config.indexserver:
                config.indexserver[name] = IndexServerConfig(name, override)

        self.handle_provision(config, reader)

        self.parse_build_isolation(config, reader)
        res = self._getenvdata(reader, config)
        config.envlist, all_envs, config.envlist_default, config.envlist_explicit = res

        # factors used in config or predefined
        known_factors = self._list_section_factors("testenv")
        known_factors.update({"py", "python"})

        # factors stated in config envlist
        stated_envlist = reader.getstring("envlist", replace=False)
        if stated_envlist:
            for env in _split_env(stated_envlist):
                known_factors.update(env.split("-"))

        # configure testenvs
        to_do = []
        failures = OrderedDict()
        results = {}
        cur_self = self

        def run(name, section, subs, config):
            try:
                results[name] = cur_self.make_envconfig(name, section, subs, config)
            except Exception as exception:
                failures[name] = (exception, traceback.format_exc())

        order = []
        for name in all_envs:
            section = "{}{}".format(testenvprefix, name)
            factors = set(name.split("-"))
            if (
                section in self._cfg
                or factors <= known_factors
                or all(
                    tox.PYTHON.PY_FACTORS_RE.match(factor) for factor in factors - known_factors
                )
            ):
                order.append(name)
                thread = Thread(target=run, args=(name, section, reader._subs, config))
                thread.daemon = True
                thread.start()
                to_do.append(thread)
        for thread in to_do:
            while thread.is_alive():
                thread.join(timeout=20)
        if failures:
            raise tox.exception.ConfigError(
                "\n".join(
                    "{} failed with {} at {}".format(key, exc, trace)
                    for key, (exc, trace) in failures.items()
                ),
            )
        for name in order:
            config.envconfigs[name] = results[name]
        all_develop = all(
            name in config.envconfigs and config.envconfigs[name].usedevelop
            for name in config.envlist
        )

        config.skipsdist = reader.getbool("skipsdist", all_develop)

        if config.option.devenv is not None:
            config.option.notest = True

        if config.option.devenv is not None and len(config.envlist) != 1:
            feedback("--devenv requires only a single -e", sysexit=True)

    def handle_provision(self, config, reader):
        requires_list = reader.getlist("requires")
        config.minversion = reader.getstring("minversion", None)
        config.provision_tox_env = name = reader.getstring("provision_tox_env", ".tox")
        min_version = "tox >= {}".format(config.minversion or tox.__version__)
        deps = self.ensure_requires_satisfied(config, requires_list, min_version)
        if config.run_provision:
            section_name = "testenv:{}".format(name)
            if section_name not in self._cfg.sections:
                self._cfg.sections[section_name] = {}
            self._cfg.sections[section_name]["description"] = "meta tox"
            env_config = self.make_envconfig(
                name,
                "{}{}".format(testenvprefix, name),
                reader._subs,
                config,
            )
            env_config.deps = deps
            config.envconfigs[config.provision_tox_env] = env_config
            raise tox.exception.MissingRequirement(config)
        # if provisioning is not on, now we need do a strict argument evaluation
        # raise on unknown args
        self.config._parser.parse_cli(args=self.config.args, strict=True)

    @staticmethod
    def ensure_requires_satisfied(config, requires, min_version):
        missing_requirements = []
        failed_to_parse = False
        deps = []
        exists = set()
        for require in requires + [min_version]:
            # noinspection PyBroadException
            try:
                package = requirements.Requirement(require)
                # check if the package even applies
                if package.marker and not package.marker.evaluate({"extra": ""}):
                    continue
                package_name = canonicalize_name(package.name)
                if package_name not in exists:
                    deps.append(DepConfig(require, None))
                    exists.add(package_name)
                    dist = importlib_metadata.distribution(package.name)
                    if not package.specifier.contains(dist.version, prereleases=True):
                        raise MissingDependency(package)
            except requirements.InvalidRequirement as exception:
                failed_to_parse = True
                error("failed to parse {!r}".format(exception))
            except Exception as exception:
                verbosity1("could not satisfy requires {!r}".format(exception))
                missing_requirements.append(str(requirements.Requirement(require)))
        if failed_to_parse:
            raise tox.exception.BadRequirement()
        if WITHIN_PROVISION and missing_requirements:
            msg = "break infinite loop provisioning within {} missing {}"
            raise tox.exception.Error(msg.format(sys.executable, missing_requirements))
        config.run_provision = bool(len(missing_requirements))
        return deps

    def parse_build_isolation(self, config, reader):
        config.isolated_build = reader.getbool("isolated_build", False)
        config.isolated_build_env = reader.getstring("isolated_build_env", ".package")
        if config.isolated_build is True:
            name = config.isolated_build_env
            section_name = "testenv:{}".format(name)
            if section_name not in self._cfg.sections:
                self._cfg.sections[section_name] = {}
            self._cfg.sections[section_name]["deps"] = ""
            self._cfg.sections[section_name]["sitepackages"] = "False"
            self._cfg.sections[section_name]["description"] = "isolated packaging environment"
            config.envconfigs[name] = self.make_envconfig(
                name,
                "{}{}".format(testenvprefix, name),
                reader._subs,
                config,
            )

    def _list_section_factors(self, section):
        factors = set()
        if section in self._cfg:
            for _, value in self._cfg[section].items():
                exprs = re.findall(r"^([\w{}\.!,-]+)\:\s+", value, re.M)
                factors.update(*mapcat(_split_factor_expr_all, exprs))
        return factors

    def make_envconfig(self, name, section, subs, config, replace=True):
        factors = set(name.split("-"))
        reader = SectionReader(section, self._cfg, fallbacksections=["testenv"], factors=factors)
        tc = TestenvConfig(name, config, factors, reader)
        reader.addsubstitutions(
            envname=name,
            envbindir=tc.get_envbindir,
            envsitepackagesdir=tc.get_envsitepackagesdir,
            envpython=tc.get_envpython,
            **subs
        )
        for env_attr in config._testenv_attr:
            atype = env_attr.type
            try:
                if atype in (
                    "bool",
                    "float",
                    "path",
                    "string",
                    "dict",
                    "dict_setenv",
                    "argv",
                    "argvlist",
                ):
                    meth = getattr(reader, "get{}".format(atype))
                    res = meth(env_attr.name, env_attr.default, replace=replace)
                elif atype == "basepython":
                    no_fallback = name in (config.provision_tox_env,)
                    res = reader.getstring(
                        env_attr.name,
                        env_attr.default,
                        replace=replace,
                        no_fallback=no_fallback,
                    )
                elif atype == "space-separated-list":
                    res = reader.getlist(env_attr.name, sep=" ")
                elif atype == "line-list":
                    res = reader.getlist(env_attr.name, sep="\n")
                elif atype == "env-list":
                    res = reader.getstring(env_attr.name, replace=False)
                    res = tuple(_split_env(res))
                else:
                    raise ValueError("unknown type {!r}".format(atype))
                if env_attr.postprocess:
                    res = env_attr.postprocess(testenv_config=tc, value=res)
            except tox.exception.MissingSubstitution as e:
                tc._missing_subs.append(e.name)
                res = e.FLAG
            setattr(tc, env_attr.name, res)
            if atype in ("path", "string", "basepython"):
                reader.addsubstitutions(**{env_attr.name: res})
        return tc

    def _getallenvs(self, reader, extra_env_list=None):
        extra_env_list = extra_env_list or []
        env_str = reader.getstring("envlist", replace=False)
        env_list = _split_env(env_str)
        for env in extra_env_list:
            if env not in env_list:
                env_list.append(env)

        all_envs = OrderedDict((i, None) for i in env_list)
        for section in self._cfg:
            if section.name.startswith(testenvprefix):
                all_envs[section.name[len(testenvprefix) :]] = None
        if not all_envs:
            all_envs["python"] = None
        return list(all_envs.keys())

    def _getenvdata(self, reader, config):
        from_option = self.config.option.env
        from_environ = os.environ.get("TOXENV")
        from_config = reader.getstring("envlist", replace=False)

        env_list = []
        envlist_explicit = False
        if (from_option and "ALL" in from_option) or (
            not from_option and from_environ and "ALL" in from_environ.split(",")
        ):
            all_envs = self._getallenvs(reader)
        else:
            candidates = (
                (os.environ.get(PARALLEL_ENV_VAR_KEY_PRIVATE), True),
                (from_option, True),
                (from_environ, True),
                ("py" if self.config.option.devenv is not None else None, False),
                (from_config, False),
            )
            env_str, envlist_explicit = next(((i, e) for i, e in candidates if i), ([], False))
            env_list = _split_env(env_str)
            all_envs = self._getallenvs(reader, env_list)

        if not env_list:
            env_list = all_envs

        package_env = config.isolated_build_env
        if config.isolated_build is True and package_env in all_envs:
            all_envs.remove(package_env)

        if config.isolated_build is True and package_env in env_list:
            msg = "isolated_build_env {} cannot be part of envlist".format(package_env)
            raise tox.exception.ConfigError(msg)
        return env_list, all_envs, _split_env(from_config), envlist_explicit

    @staticmethod
    def expand_section_names(config):
        """Generative section names.

        Allow writing section as [testenv:py{36,37}-cov]
        The parser will see it as two different sections: [testenv:py36-cov], [testenv:py37-cov]

        """
        factor_re = re.compile(r"\{\s*([\w\s,-]+)\s*\}")
        split_re = re.compile(r"\s*,\s*")
        to_remove = set()
        for section in list(config.sections):
            split_section = factor_re.split(section)
            for parts in itertools.product(*map(split_re.split, split_section)):
                section_name = "".join(parts)
                if section_name not in config.sections:
                    config.sections[section_name] = config.sections[section]
                    to_remove.add(section)

        for section in to_remove:
            del config.sections[section]


def _split_env(env):
    """if handed a list, action="append" was used for -e """
    if env is None:
        return []
    if not isinstance(env, list):
        env = [e.split("#", 1)[0].strip() for e in env.split("\n")]
        env = ",".join([e for e in env if e])
        env = [env]
    return mapcat(_expand_envstr, env)


def _is_negated_factor(factor):
    return factor.startswith("!")


def _base_factor_name(factor):
    return factor[1:] if _is_negated_factor(factor) else factor


def _split_factor_expr(expr):
    def split_single(e):
        raw = e.split("-")
        included = {_base_factor_name(factor) for factor in raw if not _is_negated_factor(factor)}
        excluded = {_base_factor_name(factor) for factor in raw if _is_negated_factor(factor)}
        return included, excluded

    partial_envs = _expand_envstr(expr)
    return [split_single(e) for e in partial_envs]


def _split_factor_expr_all(expr):
    partial_envs = _expand_envstr(expr)
    return [{_base_factor_name(factor) for factor in e.split("-")} for e in partial_envs]


def _expand_envstr(envstr):
    # split by commas not in groups
    tokens = _ENVSTR_SPLIT_PATTERN.split(envstr)
    envlist = ["".join(g).strip() for k, g in itertools.groupby(tokens, key=bool) if k]

    def expand(env):
        tokens = _ENVSTR_EXPAND_PATTERN.split(env)
        parts = [_WHITESPACE_PATTERN.sub("", token).split(",") for token in tokens]
        return ["".join(variant) for variant in itertools.product(*parts)]

    return mapcat(expand, envlist)


def mapcat(f, seq):
    return list(itertools.chain.from_iterable(map(f, seq)))


class DepConfig:
    def __init__(self, name, indexserver=None):
        self.name = name
        self.indexserver = indexserver

    def __repr__(self):
        if self.indexserver:
            if self.indexserver.name == "default":
                return self.name
            return ":{}:{}".format(self.indexserver.name, self.name)
        return str(self.name)


class IndexServerConfig:
    def __init__(self, name, url=None):
        self.name = name
        self.url = url

    def __repr__(self):
        return "IndexServerConfig(name={}, url={})".format(self.name, self.url)


is_section_substitution = re.compile(r"{\[[^{}\s]+\]\S+?}").match
# Check value matches substitution form of referencing value from other section.
# E.g. {[base]commands}


class SectionReader:
    def __init__(self, section_name, cfgparser, fallbacksections=None, factors=(), prefix=None):
        if prefix is None:
            self.section_name = section_name
        else:
            self.section_name = "{}:{}".format(prefix, section_name)
        self._cfg = cfgparser
        self.fallbacksections = fallbacksections or []
        self.factors = factors
        self._subs = {}
        self._subststack = []
        self._setenv = None

    def get_environ_value(self, name):
        if self._setenv is None:
            return os.environ.get(name)
        return self._setenv.get(name)

    def addsubstitutions(self, _posargs=None, **kw):
        self._subs.update(kw)
        if _posargs:
            self.posargs = _posargs

    def getpath(self, name, defaultpath, replace=True):
        path = self.getstring(name, defaultpath, replace=replace)
        if path is not None:
            toxinidir = self._subs["toxinidir"]
            return toxinidir.join(path, abs=True)

    def getlist(self, name, sep="\n"):
        s = self.getstring(name, None)
        if s is None:
            return []
        return [x.strip() for x in s.split(sep) if x.strip()]

    def getdict(self, name, default=None, sep="\n", replace=True):
        value = self.getstring(name, None, replace=replace)
        return self._getdict(value, default=default, sep=sep, replace=replace)

    def getdict_setenv(self, name, default=None, sep="\n", replace=True):
        value = self.getstring(name, None, replace=replace, crossonly=True)
        definitions = self._getdict(value, default=default, sep=sep, replace=replace)
        self._setenv = SetenvDict(definitions, reader=self)
        return self._setenv

    def _getdict(self, value, default, sep, replace=True):
        if value is None or not replace:
            return default or {}

        env_values = {}
        for line in value.split(sep):
            if line.strip():
                if line.startswith("#"):  # comment lines are ignored
                    pass
                elif line.startswith("file|"):  # file markers contain paths to env files
                    file_path = line[5:].strip()
                    if os.path.exists(file_path):
                        with open(file_path, "rt") as file_handler:
                            content = file_handler.read()
                        env_values.update(self._getdict(content, "", sep, replace))
                else:
                    name, value = line.split("=", 1)
                    env_values[name.strip()] = value.strip()
        return env_values

    def getfloat(self, name, default=None, replace=True):
        s = self.getstring(name, default, replace=replace)
        if not s or not replace:
            s = default
        if s is None:
            raise KeyError("no config value [{}] {} found".format(self.section_name, name))

        if not isinstance(s, float):
            try:
                s = float(s)
            except ValueError:
                raise tox.exception.ConfigError("{}: invalid float {!r}".format(name, s))
        return s

    def getbool(self, name, default=None, replace=True):
        s = self.getstring(name, default, replace=replace)
        if not s or not replace:
            s = default
        if s is None:
            raise KeyError("no config value [{}] {} found".format(self.section_name, name))

        if not isinstance(s, bool):
            if s.lower() == "true":
                s = True
            elif s.lower() == "false":
                s = False
            else:
                raise tox.exception.ConfigError(
                    "{}: boolean value {!r} needs to be 'True' or 'False'".format(name, s),
                )
        return s

    def getargvlist(self, name, default="", replace=True):
        s = self.getstring(name, default, replace=False)
        return _ArgvlistReader.getargvlist(self, s, replace=replace)

    def getargv(self, name, default="", replace=True):
        return self.getargvlist(name, default, replace=replace)[0]

    def getstring(self, name, default=None, replace=True, crossonly=False, no_fallback=False):
        x = None
        sections = [self.section_name] + ([] if no_fallback else self.fallbacksections)
        for s in sections:
            try:
                x = self._cfg[s][name]
                break
            except KeyError:
                continue

        if x is None:
            x = default
        else:
            # It is needed to apply factors before unwrapping
            # dependencies, otherwise it can break the substitution
            # process. Once they are unwrapped, we call apply factors
            # again for those new dependencies.
            x = self._apply_factors(x)
            x = self._replace_if_needed(x, name, replace, crossonly)
            x = self._apply_factors(x)

        x = self._replace_if_needed(x, name, replace, crossonly)
        return x

    def _replace_if_needed(self, x, name, replace, crossonly):
        if replace and x and hasattr(x, "replace"):
            x = self._replace(x, name=name, crossonly=crossonly)
        return x

    def _apply_factors(self, s):
        def factor_line(line):
            m = _FACTOR_LINE_PATTERN.search(line)
            if not m:
                return line

            expr, line = m.groups()
            if any(
                included <= self.factors and not any(x in self.factors for x in excluded)
                for included, excluded in _split_factor_expr(expr)
            ):
                return line

        lines = s.strip().splitlines()
        return "\n".join(filter(None, map(factor_line, lines)))

    def _replace(self, value, name=None, section_name=None, crossonly=False):
        if "{" not in value:
            return value

        section_name = section_name if section_name else self.section_name
        self._subststack.append((section_name, name))
        try:
            replaced = Replacer(self, crossonly=crossonly).do_replace(value)
            assert self._subststack.pop() == (section_name, name)
        except tox.exception.MissingSubstitution:
            if not section_name.startswith(testenvprefix):
                raise tox.exception.ConfigError(
                    "substitution env:{!r}: unknown or recursive definition in"
                    " section {!r}.".format(value, section_name),
                )
            raise
        return replaced


class Replacer:
    RE_ITEM_REF = re.compile(
        r"""
        (?<!\\)[{]
        (?:(?P<sub_type>[^[:{}]+):)?    # optional sub_type for special rules
        (?P<substitution_value>(?:\[[^,{}]*\])?[^:,{}]*)  # substitution key
        (?::(?P<default_value>[^{}]*))?   # default value
        [}]
        """,
        re.VERBOSE,
    )

    def __init__(self, reader, crossonly=False):
        self.reader = reader
        self.crossonly = crossonly

    def do_replace(self, value):
        """
        Recursively expand substitutions starting from the innermost expression
        """

        def substitute_once(x):
            return self.RE_ITEM_REF.sub(self._replace_match, x)

        expanded = substitute_once(value)

        while expanded != value:  # substitution found
            value = expanded
            expanded = substitute_once(value)

        return expanded

    def _replace_match(self, match):
        g = match.groupdict()
        sub_value = g["substitution_value"]
        if self.crossonly:
            if sub_value.startswith("["):
                return self._substitute_from_other_section(sub_value)
            # in crossonly we return all other hits verbatim
            start, end = match.span()
            return match.string[start:end]

        # special case: all empty values means ":" which is os.pathsep
        if not any(g.values()):
            return os.pathsep

        # special case: opts and packages. Leave {opts} and
        # {packages} intact, they are replaced manually in
        # _venv.VirtualEnv.run_install_command.
        if sub_value in ("opts", "packages"):
            return "{{{}}}".format(sub_value)

        try:
            sub_type = g["sub_type"]
        except KeyError:
            raise tox.exception.ConfigError(
                "Malformed substitution; no substitution type provided",
            )

        if sub_type == "env":
            return self._replace_env(match)
        if sub_type == "tty":
            if is_interactive():
                return match.group("substitution_value")
            return match.group("default_value")
        if sub_type is not None:
            raise tox.exception.ConfigError(
                "No support for the {} substitution type".format(sub_type),
            )
        return self._replace_substitution(match)

    def _replace_env(self, match):
        key = match.group("substitution_value")
        if not key:
            raise tox.exception.ConfigError("env: requires an environment variable name")
        default = match.group("default_value")
        value = self.reader.get_environ_value(key)
        if value is not None:
            return value
        if default is not None:
            return default
        raise tox.exception.MissingSubstitution(key)

    def _substitute_from_other_section(self, key):
        if key.startswith("[") and "]" in key:
            i = key.find("]")
            section, item = key[1:i], key[i + 1 :]
            cfg = self.reader._cfg
            if section in cfg and item in cfg[section]:
                if (section, item) in self.reader._subststack:
                    raise ValueError(
                        "{} already in {}".format((section, item), self.reader._subststack),
                    )
                x = str(cfg[section][item])
                return self.reader._replace(
                    x,
                    name=item,
                    section_name=section,
                    crossonly=self.crossonly,
                )

        raise tox.exception.ConfigError("substitution key {!r} not found".format(key))

    def _replace_substitution(self, match):
        sub_key = match.group("substitution_value")
        val = self.reader._subs.get(sub_key, None)
        if val is None:
            val = self._substitute_from_other_section(sub_key)
        if callable(val):
            val = val()
        return str(val)


def is_interactive():
    return sys.stdin.isatty()


class _ArgvlistReader:
    @classmethod
    def getargvlist(cls, reader, value, replace=True):
        """Parse ``commands`` argvlist multiline string.

        :param SectionReader reader: reader to be used.
        :param str value: Content stored by key.

        :rtype: list[list[str]]
        :raise :class:`tox.exception.ConfigError`:
            line-continuation ends nowhere while resolving for specified section
        """
        commands = []
        current_command = ""
        for line in value.splitlines():
            line = line.rstrip()
            if not line:
                continue
            if line.endswith("\\"):
                current_command += " {}".format(line[:-1])
                continue
            current_command += line

            if is_section_substitution(current_command):
                replaced = reader._replace(current_command, crossonly=True)
                commands.extend(cls.getargvlist(reader, replaced))
            else:
                commands.append(cls.processcommand(reader, current_command, replace))
            current_command = ""
        else:
            if current_command:
                raise tox.exception.ConfigError(
                    "line-continuation ends nowhere while resolving for [{}] {}".format(
                        reader.section_name,
                        "commands",
                    ),
                )
        return commands

    @classmethod
    def processcommand(cls, reader, command, replace=True):
        posargs = getattr(reader, "posargs", "")
        if sys.platform.startswith("win"):
            posargs_string = list2cmdline([x for x in posargs if x])
        else:
            posargs_string = " ".join([shlex_quote(x) for x in posargs if x])

        # Iterate through each word of the command substituting as
        # appropriate to construct the new command string. This
        # string is then broken up into exec argv components using
        # shlex.
        if replace:
            newcommand = ""
            for word in CommandParser(command).words():
                if word == "{posargs}" or word == "[]":
                    newcommand += posargs_string
                    continue
                elif word.startswith("{posargs:") and word.endswith("}"):
                    if posargs:
                        newcommand += posargs_string
                        continue
                    else:
                        word = word[9:-1]
                new_arg = ""
                new_word = reader._replace(word)
                new_word = reader._replace(new_word)
                new_word = new_word.replace("\\{", "{").replace("\\}", "}")
                new_arg += new_word
                newcommand += new_arg
        else:
            newcommand = command

        # Construct shlex object that will not escape any values,
        # use all values as is in argv.
        shlexer = shlex.shlex(newcommand, posix=True)
        shlexer.whitespace_split = True
        shlexer.escape = ""
        return list(shlexer)


class CommandParser(object):
    class State(object):
        def __init__(self):
            self.word = ""
            self.depth = 0
            self.yield_words = []

    def __init__(self, command):
        self.command = command

    def words(self):
        ps = CommandParser.State()

        def word_has_ended():
            return (
                (
                    cur_char in string.whitespace
                    and ps.word
                    and ps.word[-1] not in string.whitespace
                )
                or (cur_char == "{" and ps.depth == 0 and not ps.word.endswith("\\"))
                or (ps.depth == 0 and ps.word and ps.word[-1] == "}")
                or (cur_char not in string.whitespace and ps.word and ps.word.strip() == "")
            )

        def yield_this_word():
            yieldword = ps.word
            ps.word = ""
            if yieldword:
                ps.yield_words.append(yieldword)

        def yield_if_word_ended():
            if word_has_ended():
                yield_this_word()

        def accumulate():
            ps.word += cur_char

        def push_substitution():
            ps.depth += 1

        def pop_substitution():
            ps.depth -= 1

        for cur_char in self.command:
            if cur_char in string.whitespace:
                if ps.depth == 0:
                    yield_if_word_ended()
                accumulate()
            elif cur_char == "{":
                yield_if_word_ended()
                accumulate()
                push_substitution()
            elif cur_char == "}":
                accumulate()
                pop_substitution()
            else:
                yield_if_word_ended()
                accumulate()

        if ps.word.strip():
            yield_this_word()
        return ps.yield_words


def getcontextname():
    if any(env in os.environ for env in ["JENKINS_URL", "HUDSON_URL"]):
        return "jenkins"
    return None
