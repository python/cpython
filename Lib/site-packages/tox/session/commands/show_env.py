from __future__ import absolute_import, unicode_literals

from tox import reporter as report


def show_envs(config, all_envs=False, description=False):
    env_conf = config.envconfigs  # this contains all environments
    default = config.envlist_default  # this only the defaults
    ignore = {config.isolated_build_env, config.provision_tox_env}.union(default)
    extra = [e for e in env_conf if e not in ignore] if all_envs else []

    if description and default:
        report.line("default environments:")
    max_length = max(len(env) for env in (default + extra) or [""])

    def report_env(e):
        if description:
            text = env_conf[e].description or "[no description]"
            msg = "{} -> {}".format(e.ljust(max_length), text).strip()
        else:
            msg = e
        report.line(msg)

    for e in default:
        report_env(e)
    if all_envs and extra:
        if description:
            if default:
                report.line("")
            report.line("additional environments:")
        for e in extra:
            report_env(e)
