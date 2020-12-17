from __future__ import absolute_import, unicode_literals

from .base import PluginLoader


class Discovery(PluginLoader):
    """"""


def get_discover(parser, args):
    discover_types = Discovery.entry_points_for("virtualenv.discovery")
    discovery_parser = parser.add_argument_group(
        title="discovery",
        description="discover and provide a target interpreter",
    )
    discovery_parser.add_argument(
        "--discovery",
        choices=_get_default_discovery(discover_types),
        default=next(i for i in discover_types.keys()),
        required=False,
        help="interpreter discovery method",
    )
    options, _ = parser.parse_known_args(args)
    discover_class = discover_types[options.discovery]
    discover_class.add_parser_arguments(discovery_parser)
    options, _ = parser.parse_known_args(args, namespace=options)
    discover = discover_class(options)
    return discover


def _get_default_discovery(discover_types):
    return list(discover_types.keys())
