"""Hook specifications for tox - see https://pluggy.readthedocs.io/"""
import pluggy

hookspec = pluggy.HookspecMarker("tox")


@hookspec
def tox_addoption(parser):
    """ add command line options to the argparse-style parser object."""


@hookspec
def tox_configure(config):
    """Called after command line options are parsed and ini-file has been read.

    Please be aware that the config object layout may change between major tox versions.
    """


@hookspec(firstresult=True)
def tox_package(session, venv):
    """Return the package to be installed for the given venv.

    Called once for every environment."""


@hookspec(firstresult=True)
def tox_get_python_executable(envconfig):
    """Return a python executable for the given python base name.

    The first plugin/hook which returns an executable path will determine it.

    ``envconfig`` is the testenv configuration which contains
    per-testenv configuration, notably the ``.envname`` and ``.basepython``
    setting.
    """


@hookspec(firstresult=True)
def tox_testenv_create(venv, action):
    """Perform creation action for this venv.

    Some example usage:

    - To *add* behavior but still use tox's implementation to set up a
      virtualenv, implement this hook but do not return a value (or explicitly
      return ``None``).
    - To *override* tox's virtualenv creation, implement this hook and return
      a non-``None`` value.

    .. note:: This api is experimental due to the unstable api of
        :class:`tox.venv.VirtualEnv`.

    .. note:: This hook uses ``firstresult=True`` (see `pluggy first result only`_) -- hooks
        implementing this will be run until one returns non-``None``.

    .. _`pluggy first result only`: https://pluggy.readthedocs.io/en/latest/#first-result-only
    """


@hookspec(firstresult=True)
def tox_testenv_install_deps(venv, action):
    """Perform install dependencies action for this venv.

    Some example usage:

    - To *add* behavior but still use tox's implementation to install
      dependencies, implement this hook but do not return a value (or
      explicitly return ``None``).  One use-case may be to install (or ensure)
      non-python dependencies such as debian packages.
    - To *override* tox's installation of dependencies, implement this hook
      and return a non-``None`` value.  One use-case may be to install via
      a different installation tool such as `pip-accel`_ or `pip-faster`_.

    .. note:: This api is experimental due to the unstable api of
        :class:`tox.venv.VirtualEnv`.

    .. note:: This hook uses ``firstresult=True`` (see `pluggy first result only`_) -- hooks
        implementing this will be run until one returns non-``None``.

    .. _pip-accel: https://github.com/paylogic/pip-accel
    .. _pip-faster: https://github.com/Yelp/venv-update
    """


@hookspec
def tox_runtest_pre(venv):
    """Perform arbitrary action before running tests for this venv.

    This could be used to indicate that tests for a given venv have started, for instance.
    """


@hookspec(firstresult=True)
def tox_runtest(venv, redirect):
    """Run the tests for this venv.

    .. note:: This hook uses ``firstresult=True`` (see `pluggy first result only`_) -- hooks
        implementing this will be run until one returns non-``None``.
    """


@hookspec
def tox_runtest_post(venv):
    """Perform arbitrary action after running tests for this venv.

    This could be used to have per-venv test reporting of pass/fail status.
    """


@hookspec(firstresult=True)
def tox_runenvreport(venv, action):
    """Get the installed packages and versions in this venv.

    This could be used for alternative (ie non-pip) package managers, this
    plugin should return a ``list`` of type ``str``
    """


@hookspec
def tox_cleanup(session):
    """Called just before the session is destroyed, allowing any final cleanup operation"""
