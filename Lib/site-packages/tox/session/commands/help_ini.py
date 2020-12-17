from tox import reporter


def show_help_ini(config):
    reporter.separator("-", "per-testenv attributes", reporter.Verbosity.INFO)
    for env_attr in config._testenv_attr:
        reporter.line(
            "{:<15} {:<8} default: {}".format(
                env_attr.name,
                "<{}>".format(env_attr.type),
                env_attr.default,
            ),
            bold=True,
        )
        reporter.line(env_attr.help)
        reporter.line("")
