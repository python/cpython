from tox.config import Parser, get_plugin_manager


def cli_parser():
    parser = Parser()
    pm = get_plugin_manager(tuple())
    pm.hook.tox_addoption(parser=parser)
    return parser.argparser


cli = cli_parser()
