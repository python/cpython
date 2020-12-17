import sys
from collections import OrderedDict

from packaging.requirements import Requirement
from packaging.utils import canonicalize_name
from six import StringIO
from six.moves import configparser

from tox import reporter
from tox.util.stdlib import importlib_metadata

DO_NOT_SHOW_CONFIG_ATTRIBUTES = (
    "interpreters",
    "envconfigs",
    "envlist",
    "pluginmanager",
    "envlist_explicit",
)


def show_config(config):
    parser = configparser.RawConfigParser()

    if not config.envlist_explicit or reporter.verbosity() >= reporter.Verbosity.INFO:
        tox_info(config, parser)
        version_info(parser)
    tox_envs_info(config, parser)

    content = StringIO()
    parser.write(content)
    value = content.getvalue().rstrip()
    reporter.verbosity0(value)


def tox_envs_info(config, parser):
    if config.envlist_explicit:
        env_list = config.envlist
    elif config.option.listenvs:
        env_list = config.envlist_default
    else:
        env_list = list(config.envconfigs.keys())
    for name in env_list:
        env_config = config.envconfigs[name]
        values = OrderedDict(
            (attr.name, str(getattr(env_config, attr.name)))
            for attr in config._parser._testenv_attr
        )
        section = "testenv:{}".format(name)
        set_section(parser, section, values)


def tox_info(config, parser):
    info = OrderedDict(
        (i, str(getattr(config, i)))
        for i in sorted(dir(config))
        if not i.startswith("_") and i not in DO_NOT_SHOW_CONFIG_ATTRIBUTES
    )
    info["host_python"] = sys.executable
    set_section(parser, "tox", info)


def version_info(parser):
    versions = OrderedDict()
    to_visit = {"tox"}
    while to_visit:
        current = to_visit.pop()
        current_dist = importlib_metadata.distribution(current)
        current_name = canonicalize_name(current_dist.metadata["name"])
        versions[current_name] = current_dist.version
        if current_dist.requires is not None:
            for require in current_dist.requires:
                pkg = Requirement(require)
                pkg_name = canonicalize_name(pkg.name)
                if (
                    pkg.marker is None or pkg.marker.evaluate({"extra": ""})
                ) and pkg_name not in versions:
                    to_visit.add(pkg_name)
    set_section(parser, "tox:versions", versions)


def set_section(parser, section, values):
    parser.add_section(section)
    for key, value in values.items():
        parser.set(section, key, value)
