import codecs
import json
import os
import pipes
import re
import sys
from itertools import chain

import py

import tox
from tox import reporter
from tox.action import Action
from tox.config.parallel import ENV_VAR_KEY_PRIVATE as PARALLEL_ENV_VAR_KEY_PRIVATE
from tox.constants import INFO, PARALLEL_RESULT_JSON_PREFIX, PARALLEL_RESULT_JSON_SUFFIX
from tox.package.local import resolve_package
from tox.util.lock import get_unique_file
from tox.util.path import ensure_empty_dir

from .config import DepConfig


class CreationConfig:
    def __init__(
        self,
        base_resolved_python_sha256,
        base_resolved_python_path,
        tox_version,
        sitepackages,
        usedevelop,
        deps,
        alwayscopy,
    ):
        self.base_resolved_python_sha256 = base_resolved_python_sha256
        self.base_resolved_python_path = base_resolved_python_path
        self.tox_version = tox_version
        self.sitepackages = sitepackages
        self.usedevelop = usedevelop
        self.alwayscopy = alwayscopy
        self.deps = deps

    def writeconfig(self, path):
        lines = [
            "{} {}".format(self.base_resolved_python_sha256, self.base_resolved_python_path),
            "{} {:d} {:d} {:d}".format(
                self.tox_version,
                self.sitepackages,
                self.usedevelop,
                self.alwayscopy,
            ),
        ]
        for dep in self.deps:
            lines.append("{} {}".format(*dep))
        content = "\n".join(lines)
        path.ensure()
        path.write(content)
        return content

    @classmethod
    def readconfig(cls, path):
        try:
            lines = path.readlines(cr=0)
            base_resolved_python_info = lines.pop(0).split(None, 1)
            tox_version, sitepackages, usedevelop, alwayscopy = lines.pop(0).split(None, 4)
            sitepackages = bool(int(sitepackages))
            usedevelop = bool(int(usedevelop))
            alwayscopy = bool(int(alwayscopy))
            deps = []
            for line in lines:
                base_resolved_python_sha256, depstring = line.split(None, 1)
                deps.append((base_resolved_python_sha256, depstring))
            base_resolved_python_sha256, base_resolved_python_path = base_resolved_python_info
            return CreationConfig(
                base_resolved_python_sha256,
                base_resolved_python_path,
                tox_version,
                sitepackages,
                usedevelop,
                deps,
                alwayscopy,
            )
        except Exception:
            return None

    def matches_with_reason(self, other, deps_matches_subset=False):
        for attr in (
            "base_resolved_python_sha256",
            "base_resolved_python_path",
            "tox_version",
            "sitepackages",
            "usedevelop",
            "alwayscopy",
        ):
            left = getattr(self, attr)
            right = getattr(other, attr)
            if left != right:
                return False, "attr {} {!r}!={!r}".format(attr, left, right)
        self_deps = set(self.deps)
        other_deps = set(other.deps)
        if self_deps != other_deps:
            if deps_matches_subset:
                diff = other_deps - self_deps
                if diff:
                    return False, "missing in previous {!r}".format(diff)
            else:
                return False, "{!r}!={!r}".format(self_deps, other_deps)
        return True, None

    def matches(self, other, deps_matches_subset=False):
        outcome, _ = self.matches_with_reason(other, deps_matches_subset)
        return outcome


class VirtualEnv(object):
    def __init__(self, envconfig=None, popen=None, env_log=None):
        self.envconfig = envconfig
        self.popen = popen
        self._actions = []
        self.env_log = env_log
        self._result_json_path = None

    def new_action(self, msg, *args):
        config = self.envconfig.config
        command_log = self.env_log.get_commandlog(
            "test" if msg in ("run-test", "run-test-pre", "run-test-post") else "setup",
        )
        return Action(
            self.name,
            msg,
            args,
            self.envconfig.envlogdir,
            config.option.resultjson,
            command_log,
            self.popen,
            self.envconfig.envpython,
            self.envconfig.suicide_timeout,
            self.envconfig.interrupt_timeout,
            self.envconfig.terminate_timeout,
        )

    def get_result_json_path(self):
        if self._result_json_path is None:
            if self.envconfig.config.option.resultjson:
                self._result_json_path = get_unique_file(
                    self.path,
                    PARALLEL_RESULT_JSON_PREFIX,
                    PARALLEL_RESULT_JSON_SUFFIX,
                )
        return self._result_json_path

    @property
    def hook(self):
        return self.envconfig.config.pluginmanager.hook

    @property
    def path(self):
        """ Path to environment base dir. """
        return self.envconfig.envdir

    @property
    def path_config(self):
        return self.path.join(".tox-config1")

    @property
    def name(self):
        """ test environment name. """
        return self.envconfig.envname

    def __repr__(self):
        return "<VirtualEnv at {!r}>".format(self.path)

    def getcommandpath(self, name, venv=True, cwd=None):
        """Return absolute path (str or localpath) for specified command name.

        - If it's a local path we will rewrite it as as a relative path.
        - If venv is True we will check if the command is coming from the venv
          or is allowed to come from external.
        """
        name = str(name)
        if os.path.isabs(name):
            return name
        if os.path.split(name)[0] == ".":
            path = cwd.join(name)
            if path.check():
                return str(path)

        if venv:
            path = self._venv_lookup_and_check_external_allowlist(name)
        else:
            path = self._normal_lookup(name)

        if path is None:
            raise tox.exception.InvocationError(
                "could not find executable {}".format(pipes.quote(name)),
            )

        return str(path)  # will not be rewritten for reporting

    def _venv_lookup_and_check_external_allowlist(self, name):
        path = self._venv_lookup(name)
        if path is None:
            path = self._normal_lookup(name)
            if path is not None:
                self._check_external_allowed_and_warn(path)
        return path

    def _venv_lookup(self, name):
        return py.path.local.sysfind(name, paths=[self.envconfig.envbindir])

    def _normal_lookup(self, name):
        return py.path.local.sysfind(name)

    def _check_external_allowed_and_warn(self, path):
        if not self.is_allowed_external(path):
            reporter.warning(
                "test command found but not installed in testenv\n"
                "  cmd: {}\n"
                "  env: {}\n"
                "Maybe you forgot to specify a dependency? "
                "See also the allowlist_externals envconfig setting.\n\n"
                "DEPRECATION WARNING: this will be an error in tox 4 and above!".format(
                    path,
                    self.envconfig.envdir,
                ),
            )

    def is_allowed_external(self, p):
        tryadd = [""]
        if tox.INFO.IS_WIN:
            tryadd += [os.path.normcase(x) for x in os.environ["PATHEXT"].split(os.pathsep)]
            p = py.path.local(os.path.normcase(str(p)))

        if self.envconfig.allowlist_externals and self.envconfig.whitelist_externals:
            raise tox.exception.ConfigError(
                "Either whitelist_externals or allowlist_externals might be specified, not both",
            )

        allowed_externals = (
            self.envconfig.whitelist_externals or self.envconfig.allowlist_externals
        )
        for x in allowed_externals:
            for add in tryadd:
                if p.fnmatch(x + add):
                    return True
        return False

    def update(self, action):
        """return status string for updating actual venv to match configuration.
        if status string is empty, all is ok.
        """
        rconfig = CreationConfig.readconfig(self.path_config)
        if self.envconfig.recreate:
            reason = "-r flag"
        else:
            if rconfig is None:
                reason = "no previous config {}".format(self.path_config)
            else:
                live_config = self._getliveconfig()
                deps_subset_match = getattr(self.envconfig, "deps_matches_subset", False)
                outcome, reason = rconfig.matches_with_reason(live_config, deps_subset_match)
        if reason is None:
            action.info("reusing", self.envconfig.envdir)
            return
        action.info("cannot reuse", reason)
        if rconfig is None:
            action.setactivity("create", self.envconfig.envdir)
        else:
            action.setactivity("recreate", self.envconfig.envdir)
        try:
            self.hook.tox_testenv_create(action=action, venv=self)
            self.just_created = True
        except tox.exception.UnsupportedInterpreter as exception:
            return exception
        try:
            self.hook.tox_testenv_install_deps(action=action, venv=self)
        except tox.exception.InvocationError as exception:
            return "could not install deps {}; v = {!r}".format(self.envconfig.deps, exception)

    def _getliveconfig(self):
        base_resolved_python_path = self.envconfig.python_info.executable
        version = tox.__version__
        sitepackages = self.envconfig.sitepackages
        develop = self.envconfig.usedevelop
        alwayscopy = self.envconfig.alwayscopy
        deps = []
        for dep in self.get_resolved_dependencies():
            dep_name_sha256 = getdigest(dep.name)
            deps.append((dep_name_sha256, dep.name))
        base_resolved_python_sha256 = getdigest(base_resolved_python_path)
        return CreationConfig(
            base_resolved_python_sha256,
            base_resolved_python_path,
            version,
            sitepackages,
            develop,
            deps,
            alwayscopy,
        )

    def get_resolved_dependencies(self):
        dependencies = []
        for dependency in self.envconfig.deps:
            if dependency.indexserver is None:
                package = resolve_package(package_spec=dependency.name)
                if package != dependency.name:
                    dependency = dependency.__class__(package)
            dependencies.append(dependency)
        return dependencies

    def getsupportedinterpreter(self):
        return self.envconfig.getsupportedinterpreter()

    def matching_platform(self):
        return re.match(self.envconfig.platform, sys.platform)

    def finish(self):
        previous_config = CreationConfig.readconfig(self.path_config)
        live_config = self._getliveconfig()
        if previous_config is None or not previous_config.matches(live_config):
            content = live_config.writeconfig(self.path_config)
            reporter.verbosity1("write config to {} as {!r}".format(self.path_config, content))

    def _needs_reinstall(self, setupdir, action):
        setup_py = setupdir.join("setup.py")
        setup_cfg = setupdir.join("setup.cfg")
        args = [self.envconfig.envpython, str(setup_py), "--name"]
        env = self._get_os_environ()
        output = action.popen(
            args,
            cwd=setupdir,
            redirect=False,
            returnout=True,
            env=env,
            capture_err=False,
        )
        name = next(
            (i for i in output.split("\n") if i and not i.startswith("pydev debugger:")),
            "",
        )
        args = [
            self.envconfig.envpython,
            "-c",
            "import sys;  import json; print(json.dumps(sys.path))",
        ]
        out = action.popen(args, redirect=False, returnout=True, env=env)
        try:
            sys_path = json.loads(out)
        except ValueError:
            sys_path = []
        egg_info_fname = ".".join((name.replace("-", "_"), "egg-info"))
        for d in reversed(sys_path):
            egg_info = py.path.local(d).join(egg_info_fname)
            if egg_info.check():
                break
        else:
            return True
        needs_reinstall = any(
            conf_file.check() and conf_file.mtime() > egg_info.mtime()
            for conf_file in (setup_py, setup_cfg)
        )

        # Ensure the modification time of the egg-info folder is updated so we
        # won't need to do this again.
        # TODO(stephenfin): Remove once the minimum version of setuptools is
        # high enough to include https://github.com/pypa/setuptools/pull/1427/
        if needs_reinstall:
            egg_info.setmtime()

        return needs_reinstall

    def install_pkg(self, dir, action, name, is_develop=False):
        assert action is not None

        if getattr(self, "just_created", False):
            action.setactivity(name, dir)
            self.finish()
            pip_flags = ["--exists-action", "w"]
        else:
            if is_develop and not self._needs_reinstall(dir, action):
                action.setactivity("{}-noop".format(name), dir)
                return
            action.setactivity("{}-nodeps".format(name), dir)
            pip_flags = ["--no-deps"] + ([] if is_develop else ["-U"])
        pip_flags.extend(["-v"] * min(3, reporter.verbosity() - 2))
        if self.envconfig.extras:
            dir += "[{}]".format(",".join(self.envconfig.extras))
        target = [dir]
        if is_develop:
            target.insert(0, "-e")
        self._install(target, extraopts=pip_flags, action=action)

    def developpkg(self, setupdir, action):
        self.install_pkg(setupdir, action, "develop-inst", is_develop=True)

    def installpkg(self, sdistpath, action):
        self.install_pkg(sdistpath, action, "inst")

    def _installopts(self, indexserver):
        options = []
        if indexserver:
            options += ["-i", indexserver]
        if self.envconfig.pip_pre:
            options.append("--pre")
        return options

    def run_install_command(self, packages, action, options=()):
        def expand(val):
            # expand an install command
            if val == "{packages}":
                for package in packages:
                    yield package
            elif val == "{opts}":
                for opt in options:
                    yield opt
            else:
                yield val

        cmd = list(chain.from_iterable(expand(val) for val in self.envconfig.install_command))

        env = self._get_os_environ()
        self.ensure_pip_os_environ_ok(env)

        old_stdout = sys.stdout
        sys.stdout = codecs.getwriter("utf8")(sys.stdout)
        try:
            self._pcall(
                cmd,
                cwd=self.envconfig.config.toxinidir,
                action=action,
                redirect=reporter.verbosity() < reporter.Verbosity.DEBUG,
                env=env,
            )
        finally:
            sys.stdout = old_stdout

    def ensure_pip_os_environ_ok(self, env):
        for key in ("PIP_RESPECT_VIRTUALENV", "PIP_REQUIRE_VIRTUALENV", "__PYVENV_LAUNCHER__"):
            env.pop(key, None)
        if all("PYTHONPATH" not in i for i in (self.envconfig.passenv, self.envconfig.setenv)):
            # If PYTHONPATH not explicitly asked for, remove it.
            if "PYTHONPATH" in env:
                if sys.version_info < (3, 4) or bool(env["PYTHONPATH"]):
                    # https://docs.python.org/3/whatsnew/3.4.html#changes-in-python-command-behavior
                    # In a posix shell, setting the PATH environment variable to an empty value is
                    # equivalent to not setting it at all.
                    reporter.warning(
                        "Discarding $PYTHONPATH from environment, to override "
                        "specify PYTHONPATH in 'passenv' in your configuration.",
                    )
                env.pop("PYTHONPATH")

        # installing packages at user level may mean we're not installing inside the venv
        env["PIP_USER"] = "0"

        # installing without dependencies may lead to broken packages
        env["PIP_NO_DEPS"] = "0"

    def _install(self, deps, extraopts=None, action=None):
        if not deps:
            return
        d = {}
        ixservers = []
        for dep in deps:
            if isinstance(dep, (str, py.path.local)):
                dep = DepConfig(str(dep), None)
            assert isinstance(dep, DepConfig), dep
            if dep.indexserver is None:
                ixserver = self.envconfig.config.indexserver["default"]
            else:
                ixserver = dep.indexserver
            d.setdefault(ixserver, []).append(dep.name)
            if ixserver not in ixservers:
                ixservers.append(ixserver)
            assert ixserver.url is None or isinstance(ixserver.url, str)

        for ixserver in ixservers:
            packages = d[ixserver]
            options = self._installopts(ixserver.url)
            if extraopts:
                options.extend(extraopts)
            self.run_install_command(packages=packages, options=options, action=action)

    def _get_os_environ(self, is_test_command=False):
        if is_test_command:
            # for executing tests we construct a clean environment
            env = {}
            for env_key in self.envconfig.passenv:
                if env_key in os.environ:
                    env[env_key] = os.environ[env_key]
        else:
            # for executing non-test commands we use the full
            # invocation environment
            env = os.environ.copy()

        # in any case we honor per-testenv setenv configuration
        env.update(self.envconfig.setenv)

        env["VIRTUAL_ENV"] = str(self.path)
        return env

    def test(
        self,
        redirect=False,
        name="run-test",
        commands=None,
        ignore_outcome=None,
        ignore_errors=None,
        display_hash_seed=False,
    ):
        if commands is None:
            commands = self.envconfig.commands
        if ignore_outcome is None:
            ignore_outcome = self.envconfig.ignore_outcome
        if ignore_errors is None:
            ignore_errors = self.envconfig.ignore_errors
        with self.new_action(name) as action:
            cwd = self.envconfig.changedir
            if display_hash_seed:
                env = self._get_os_environ(is_test_command=True)
                # Display PYTHONHASHSEED to assist with reproducibility.
                action.setactivity(name, "PYTHONHASHSEED={!r}".format(env.get("PYTHONHASHSEED")))
            for i, argv in enumerate(filter(bool, commands)):
                # have to make strings as _pcall changes argv[0] to a local()
                # happens if the same environment is invoked twice
                message = "commands[{}] | {}".format(
                    i,
                    " ".join([pipes.quote(str(x)) for x in argv]),
                )
                action.setactivity(name, message)
                # check to see if we need to ignore the return code
                # if so, we need to alter the command line arguments
                if argv[0].startswith("-"):
                    ignore_ret = True
                    if argv[0] == "-":
                        del argv[0]
                    else:
                        argv[0] = argv[0].lstrip("-")
                else:
                    ignore_ret = False

                try:
                    self._pcall(
                        argv,
                        cwd=cwd,
                        action=action,
                        redirect=redirect,
                        ignore_ret=ignore_ret,
                        is_test_command=True,
                    )
                except tox.exception.InvocationError as err:
                    if ignore_outcome:
                        msg = "command failed but result from testenv is ignored\ncmd:"
                        reporter.warning("{} {}".format(msg, err))
                        self.status = "ignored failed command"
                        continue  # keep processing commands

                    reporter.error(str(err))
                    self.status = "commands failed"
                    if not ignore_errors:
                        break  # Don't process remaining commands
                except KeyboardInterrupt:
                    self.status = "keyboardinterrupt"
                    raise

    def _pcall(
        self,
        args,
        cwd,
        venv=True,
        is_test_command=False,
        action=None,
        redirect=True,
        ignore_ret=False,
        returnout=False,
        env=None,
    ):
        if env is None:
            env = self._get_os_environ(is_test_command=is_test_command)

        # construct environment variables
        env.pop("VIRTUALENV_PYTHON", None)
        bin_dir = str(self.envconfig.envbindir)
        path = self.envconfig.setenv.get("PATH") or os.environ["PATH"]
        env["PATH"] = os.pathsep.join([bin_dir, path])
        reporter.verbosity2("setting PATH={}".format(env["PATH"]))

        # get command
        args[0] = self.getcommandpath(args[0], venv, cwd)
        if sys.platform != "win32" and "TOX_LIMITED_SHEBANG" in os.environ:
            args = prepend_shebang_interpreter(args)

        cwd.ensure(dir=1)  # ensure the cwd exists
        return action.popen(
            args,
            cwd=cwd,
            env=env,
            redirect=redirect,
            ignore_ret=ignore_ret,
            returnout=returnout,
            report_fail=not is_test_command,
        )

    def setupenv(self):
        if self.envconfig._missing_subs:
            self.status = (
                "unresolvable substitution(s): {}. "
                "Environment variables are missing or defined recursively.".format(
                    ",".join(["'{}'".format(m) for m in self.envconfig._missing_subs]),
                )
            )
            return
        if not self.matching_platform():
            self.status = "platform mismatch"
            return  # we simply omit non-matching platforms
        with self.new_action("getenv", self.envconfig.envdir) as action:
            self.status = 0
            default_ret_code = 1
            envlog = self.env_log
            try:
                status = self.update(action=action)
            except IOError as e:
                if e.args[0] != 2:
                    raise
                status = (
                    "Error creating virtualenv. Note that spaces in paths are "
                    "not supported by virtualenv. Error details: {!r}".format(e)
                )
            except tox.exception.InvocationError as e:
                status = e
            except tox.exception.InterpreterNotFound as e:
                status = e
                if self.envconfig.config.option.skip_missing_interpreters == "true":
                    default_ret_code = 0
            if status:
                str_status = str(status)
                command_log = envlog.get_commandlog("setup")
                command_log.add_command(["setup virtualenv"], str_status, default_ret_code)
                self.status = status
                if default_ret_code == 0:
                    reporter.skip(str_status)
                else:
                    reporter.error(str_status)
                return False
            command_path = self.getcommandpath("python")
            envlog.set_python_info(command_path)
            return True

    def finishvenv(self):
        with self.new_action("finishvenv"):
            self.finish()
            return True


def getdigest(path):
    path = py.path.local(path)
    if not path.check(file=1):
        return "0" * 32
    return path.computehash("sha256")


def prepend_shebang_interpreter(args):
    # prepend interpreter directive (if any) to argument list
    #
    # When preparing virtual environments in a file container which has large
    # length, the system might not be able to invoke shebang scripts which
    # define interpreters beyond system limits (e.x. Linux as a limit of 128;
    # BINPRM_BUF_SIZE). This method can be used to check if the executable is
    # a script containing a shebang line. If so, extract the interpreter (and
    # possible optional argument) and prepend the values to the provided
    # argument list. tox will only attempt to read an interpreter directive of
    # a maximum size of 2048 bytes to limit excessive reading and support UNIX
    # systems which may support a longer interpret length.
    try:
        with open(args[0], "rb") as f:
            if f.read(1) == b"#" and f.read(1) == b"!":
                MAXINTERP = 2048
                interp = f.readline(MAXINTERP).rstrip().decode("UTF-8")
                interp_args = interp.split(None, 1)[:2]
                return interp_args + args
    except (UnicodeDecodeError, IOError):
        pass
    return args


_SKIP_VENV_CREATION = os.environ.get("_TOX_SKIP_ENV_CREATION_TEST", False) == "1"


@tox.hookimpl
def tox_testenv_create(venv, action):
    config_interpreter = venv.getsupportedinterpreter()
    args = [sys.executable, "-m", "virtualenv"]
    if venv.envconfig.sitepackages:
        args.append("--system-site-packages")
    if venv.envconfig.alwayscopy:
        args.append("--always-copy")
    if not venv.envconfig.download:
        args.append("--no-download")
    # add interpreter explicitly, to prevent using default (virtualenv.ini)
    args.extend(["--python", str(config_interpreter)])

    cleanup_for_venv(venv)

    base_path = venv.path.dirpath()
    base_path.ensure(dir=1)
    args.append(venv.path.basename)
    if not _SKIP_VENV_CREATION:
        try:
            venv._pcall(
                args,
                venv=False,
                action=action,
                cwd=base_path,
                redirect=reporter.verbosity() < reporter.Verbosity.DEBUG,
            )
        except KeyboardInterrupt:
            venv.status = "keyboardinterrupt"
            raise
    return True  # Return non-None to indicate plugin has completed


def cleanup_for_venv(venv):
    within_parallel = PARALLEL_ENV_VAR_KEY_PRIVATE in os.environ
    # if the directory exists and it doesn't look like a virtualenv, produce
    # an error
    if venv.path.exists():
        dir_items = set(os.listdir(str(venv.path))) - {".lock", "log"}
        dir_items = {p for p in dir_items if not p.startswith(".tox-") or p == ".tox-config1"}
    else:
        dir_items = set()

    if not (
        # doesn't exist => OK
        not venv.path.exists()
        # does exist, but it's empty => OK
        or not dir_items
        # tox has marked this as an environment it has created in the past
        or ".tox-config1" in dir_items
        # it exists and we're on windows with Lib and Scripts => OK
        or (INFO.IS_WIN and dir_items > {"Scripts", "Lib"})
        # non-windows, with lib and bin => OK
        or dir_items > {"bin", "lib"}
        # pypy has a different lib folder => OK
        or dir_items > {"bin", "lib_pypy"}
    ):
        venv.status = "error"
        reporter.error(
            "cowardly refusing to delete `envdir` (it does not look like a virtualenv): "
            "{}".format(venv.path),
        )
        raise SystemExit(2)

    if within_parallel:
        if venv.path.exists():
            # do not delete the log folder as that's used by parent
            for content in venv.path.listdir():
                if not content.basename == "log":
                    content.remove(rec=1, ignore_errors=True)
    else:
        ensure_empty_dir(venv.path)


@tox.hookimpl
def tox_testenv_install_deps(venv, action):
    deps = venv.get_resolved_dependencies()
    if deps:
        depinfo = ", ".join(map(str, deps))
        action.setactivity("installdeps", depinfo)
        venv._install(deps, action=action)
    return True  # Return non-None to indicate plugin has completed


@tox.hookimpl
def tox_runtest(venv, redirect):
    venv.test(redirect=redirect)
    return True  # Return non-None to indicate plugin has completed


@tox.hookimpl
def tox_runtest_pre(venv):
    venv.status = 0
    ensure_empty_dir(venv.envconfig.envtmpdir)
    venv.envconfig.envtmpdir.ensure(dir=1)
    venv.test(
        name="run-test-pre",
        commands=venv.envconfig.commands_pre,
        redirect=False,
        ignore_outcome=False,
        ignore_errors=False,
        display_hash_seed=True,
    )


@tox.hookimpl
def tox_runtest_post(venv):
    venv.test(
        name="run-test-post",
        commands=venv.envconfig.commands_post,
        redirect=False,
        ignore_outcome=False,
        ignore_errors=False,
    )


@tox.hookimpl
def tox_runenvreport(venv, action):
    # write out version dependency information
    args = venv.envconfig.list_dependencies_command
    output = venv._pcall(args, cwd=venv.envconfig.config.toxinidir, action=action, returnout=True)
    # the output contains a mime-header, skip it
    output = output.split("\n\n")[-1]
    packages = output.strip().split("\n")
    return packages  # Return non-None to indicate plugin has completed
