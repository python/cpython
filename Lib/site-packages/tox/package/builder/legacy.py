import sys

import py

from tox import reporter
from tox.util.path import ensure_empty_dir


def make_sdist(config, session):
    setup = config.setupdir.join("setup.py")
    pyproject = config.setupdir.join("pyproject.toml")
    setup_check = setup.check()
    if not setup_check and not pyproject.check():
        reporter.error(
            "No pyproject.toml or setup.py file found. The expected locations are:\n"
            "  {pyproject} or {setup}\n"
            "You can\n"
            "  1. Create one:\n"
            "     https://tox.readthedocs.io/en/latest/example/package.html\n"
            "  2. Configure tox to avoid running sdist:\n"
            "     https://tox.readthedocs.io/en/latest/example/general.html\n"
            "  3. Configure tox to use an isolated_build".format(pyproject=pyproject, setup=setup),
        )
        raise SystemExit(1)
    if not setup_check:
        reporter.error(
            "pyproject.toml file found.\n"
            "To use a PEP 517 build-backend you are required to "
            "configure tox to use an isolated_build:\n"
            "https://tox.readthedocs.io/en/latest/example/package.html\n",
        )
        raise SystemExit(1)
    with session.newaction("GLOB", "packaging") as action:
        action.setactivity("sdist-make", setup)
        ensure_empty_dir(config.distdir)
        build_log = action.popen(
            [sys.executable, setup, "sdist", "--formats=zip", "--dist-dir", config.distdir],
            cwd=config.setupdir,
            returnout=True,
        )
        reporter.verbosity2(build_log)
        try:
            return config.distdir.listdir()[0]
        except py.error.ENOENT:
            # check if empty or comment only
            data = []
            with open(str(setup)) as fp:
                for line in fp:
                    if line and line[0] == "#":
                        continue
                    data.append(line)
            if not "".join(data).strip():
                reporter.error("setup.py is empty")
                raise SystemExit(1)
            reporter.error(
                "No dist directory found. Please check setup.py, e.g with:\n"
                "     python setup.py sdist",
            )
            raise SystemExit(1)
