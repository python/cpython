from tox import reporter


def show_help(config):
    reporter.line(config._parser._format_help())
    reporter.line("Environment variables", bold=True)
    reporter.line("TOXENV: comma separated list of environments (overridable by '-e')")
    reporter.line("TOX_SKIP_ENV: regular expression to filter down from running tox environments")
    reporter.line(
        "TOX_TESTENV_PASSENV: space-separated list of extra environment variables to be "
        "passed into test command environments",
    )
    reporter.line("PY_COLORS: 0 disable colorized output, 1 enable (default)")
    reporter.line("TOX_PARALLEL_NO_SPINNER: 1 disable spinner for CI, 0 enable (default)")
