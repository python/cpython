# -*- coding: utf-8 -*-
"""
    tox._quickstart
    ~~~~~~~~~~~~~~~~~

    Command-line script to quickly setup a configuration for a Python project

    This file was heavily inspired by and uses code from ``sphinx-quickstart``
    in the BSD-licensed `Sphinx project`_.

    .. Sphinx project_: http://sphinx.pocoo.org/

    License for Sphinx
    ==================

    Copyright (c) 2007-2011 by the Sphinx team (see AUTHORS file).
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""
import argparse
import codecs
import os
import sys
import textwrap

import six

import tox

ALTERNATIVE_CONFIG_NAME = "tox-generated.ini"
QUICKSTART_CONF = """\
# tox (https://tox.readthedocs.io/) is a tool for running tests
# in multiple virtualenvs. This configuration file will run the
# test suite on all supported python versions. To use it, "pip install tox"
# and then run "tox" from this directory.

[tox]
envlist = {envlist}

[testenv]
deps =
    {deps}
commands =
    {commands}
"""


class ValidationError(Exception):
    """Raised for validation errors."""


def nonempty(x):
    if not x:
        raise ValidationError("Please enter some text.")
    return x


def choice(*line):
    def val(x):
        if x not in line:
            raise ValidationError("Please enter one of {}.".format(", ".join(line)))
        return x

    return val


def boolean(x):
    if x.upper() not in ("Y", "YES", "N", "NO"):
        raise ValidationError("Please enter either 'y' or 'n'.")
    return x.upper() in ("Y", "YES")


def list_modificator(answer, existing=None):
    if not existing:
        existing = []
    if not isinstance(existing, list):
        existing = [existing]
    if not answer:
        return existing
    existing.extend([t.strip() for t in answer.split(",") if t.strip()])
    return existing


def do_prompt(map_, key, text, default=None, validator=nonempty, modificator=None):
    while True:
        prompt = "> {} [{}]: ".format(text, default) if default else "> {}: ".format(text)
        answer = six.moves.input(prompt)
        if default and not answer:
            answer = default
        # FIXME use six instead of self baked solution
        # noinspection PyUnresolvedReferences
        if sys.version_info < (3,) and not isinstance(answer, unicode):  # noqa
            # for Python 2.x, try to get a Unicode string out of it
            if answer.decode("ascii", "replace").encode("ascii", "replace") != answer:
                term_encoding = getattr(sys.stdin, "encoding", None)
                if term_encoding:
                    answer = answer.decode(term_encoding)
                else:
                    print(
                        "* Note: non-ASCII characters entered but terminal encoding unknown"
                        " -> assuming UTF-8 or Latin-1.",
                    )
                    try:
                        answer = answer.decode("utf-8")
                    except UnicodeDecodeError:
                        answer = answer.decode("latin1")
        if validator:
            try:
                answer = validator(answer)
            except ValidationError as exception:
                print("* {}".format(exception))
                continue
        break
    map_[key] = modificator(answer, map_.get(key)) if modificator else answer


def ask_user(map_):
    """modify *map_* in place by getting info from the user."""
    print("Welcome to the tox {} quickstart utility.".format(tox.__version__))
    print(
        "This utility will ask you a few questions and then generate a simple configuration "
        "file to help get you started using tox.\n"
        "Please enter values for the following settings (just press Enter to accept a "
        "default value, if one is given in brackets).\n",
    )
    print(
        textwrap.dedent(
            """What Python versions do you want to test against?
            [1] {}
            [2] py27, {}
            [3] (All versions) {}
            [4] Choose each one-by-one""",
        ).format(
            tox.PYTHON.CURRENT_RELEASE_ENV,
            tox.PYTHON.CURRENT_RELEASE_ENV,
            ", ".join(tox.PYTHON.QUICKSTART_PY_ENVS),
        ),
    )
    do_prompt(
        map_,
        "canned_pyenvs",
        "Enter the number of your choice",
        default="3",
        validator=choice("1", "2", "3", "4"),
    )
    if map_["canned_pyenvs"] == "1":
        map_[tox.PYTHON.CURRENT_RELEASE_ENV] = True
    elif map_["canned_pyenvs"] == "2":
        for pyenv in ("py27", tox.PYTHON.CURRENT_RELEASE_ENV):
            map_[pyenv] = True
    elif map_["canned_pyenvs"] == "3":
        for pyenv in tox.PYTHON.QUICKSTART_PY_ENVS:
            map_[pyenv] = True
    elif map_["canned_pyenvs"] == "4":
        for pyenv in tox.PYTHON.QUICKSTART_PY_ENVS:
            if pyenv not in map_:
                do_prompt(
                    map_,
                    pyenv,
                    "Test your project with {} (Y/n)".format(pyenv),
                    "Y",
                    validator=boolean,
                )
    print(
        textwrap.dedent(
            """What command should be used to test your project? Examples:\
            - pytest\n"
            - python -m unittest discover
            - python setup.py test
            - trial package.module""",
        ),
    )
    do_prompt(
        map_,
        "commands",
        "Type the command to run your tests",
        default="pytest",
        modificator=list_modificator,
    )
    print("What extra dependencies do your tests have?")
    map_["deps"] = get_default_deps(map_["commands"])
    if map_["deps"]:
        print("default dependencies are: {}".format(map_["deps"]))
    do_prompt(
        map_,
        "deps",
        "Comma-separated list of dependencies",
        validator=None,
        modificator=list_modificator,
    )


def get_default_deps(commands):
    if commands and any(c in str(commands) for c in ["pytest", "py.test"]):
        return ["pytest"]
    if "trial" in commands:
        return ["twisted"]
    return []


def post_process_input(map_):
    envlist = [env for env in tox.PYTHON.QUICKSTART_PY_ENVS if map_.get(env) is True]
    map_["envlist"] = ", ".join(envlist)
    map_["commands"] = "\n    ".join([cmd.strip() for cmd in map_["commands"]])
    map_["deps"] = "\n    ".join([dep.strip() for dep in set(map_["deps"])])


def generate(map_):
    """Generate project based on values in *d*."""
    dpath = map_.get("path", os.getcwd())
    altpath = os.path.join(dpath, ALTERNATIVE_CONFIG_NAME)
    while True:
        name = map_.get("name", tox.INFO.DEFAULT_CONFIG_NAME)
        targetpath = os.path.join(dpath, name)
        if not os.path.isfile(targetpath):
            break
        do_prompt(map_, "name", "{} exists - choose an alternative".format(targetpath), altpath)
    with codecs.open(targetpath, "w", encoding="utf-8") as f:
        f.write(prepare_content(QUICKSTART_CONF.format(**map_)))
        print(
            "Finished: {} has been created. For information on this file, "
            "see https://tox.readthedocs.io/en/latest/config.html\n"
            "Execute `tox` to test your project.".format(targetpath),
        )


def prepare_content(content):
    return "\n".join([line.rstrip() for line in content.split("\n")])


def parse_args():
    parser = argparse.ArgumentParser(
        description="Command-line script to quickly tox config file for a Python project.",
    )
    parser.add_argument(
        "root",
        type=str,
        nargs="?",
        default=".",
        help="Custom root directory to write config to. Defaults to current directory.",
    )
    parser.add_argument(
        "--version",
        action="version",
        version="%(prog)s {}".format(tox.__version__),
    )
    return parser.parse_args()


def main():
    args = parse_args()
    map_ = {"path": args.root}
    try:
        ask_user(map_)
    except (KeyboardInterrupt, EOFError):
        print("\n[Interrupted.]")
        return 1
    post_process_input(map_)
    generate(map_)


if __name__ == "__main__":
    sys.exit(main())
