import py

import tox
from tox.exception import InvocationError


def run_sequential(config, venv_dict):
    for venv in venv_dict.values():
        if venv.setupenv():
            if venv.envconfig.skip_install:
                venv.finishvenv()
            else:
                if venv.envconfig.usedevelop:
                    develop_pkg(venv, config.setupdir)
                elif config.skipsdist:
                    venv.finishvenv()
                else:
                    installpkg(venv, venv.package)
            if venv.status == 0:
                runenvreport(venv, config)
        if venv.status == 0:
            runtestenv(venv, config)


def develop_pkg(venv, setupdir):
    with venv.new_action("developpkg", setupdir) as action:
        try:
            venv.developpkg(setupdir, action)
            return True
        except InvocationError as exception:
            venv.status = exception
            return False


def installpkg(venv, path):
    """Install package in the specified virtual environment.

    :param VenvConfig venv: Destination environment
    :param str path: Path to the distribution package.
    :return: True if package installed otherwise False.
    :rtype: bool
    """
    venv.env_log.set_header(installpkg=py.path.local(path))
    with venv.new_action("installpkg", path) as action:
        try:
            venv.installpkg(path, action)
            return True
        except tox.exception.InvocationError as exception:
            venv.status = exception
            return False


def runenvreport(venv, config):
    """
    Run an environment report to show which package
    versions are installed in the venv
    """
    try:
        with venv.new_action("envreport") as action:
            packages = config.pluginmanager.hook.tox_runenvreport(venv=venv, action=action)
        action.setactivity("installed", ",".join(packages))
        venv.env_log.set_installed(packages)
    except InvocationError as exception:
        venv.status = exception


def runtestenv(venv, config, redirect=False):
    if venv.status == 0 and config.option.notest:
        venv.status = "skipped tests"
    else:
        if venv.status:
            return
        config.pluginmanager.hook.tox_runtest_pre(venv=venv)
        if venv.status == 0:
            config.pluginmanager.hook.tox_runtest(venv=venv, redirect=redirect)
        config.pluginmanager.hook.tox_runtest_post(venv=venv)
