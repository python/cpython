import argparse
import io
import os
import sys
import sysconfig
import tomllib

from typing import Any, Callable


ConfigItem = str | int | list[str | int]
ConfigData = dict[str, dict[str, ConfigItem]]


def generic_info() -> ConfigData:
    return {
        'python': {
            'version': sys.version.split(' ')[0],
            'version_parts': {
                field: getattr(sys.version_info, field)
                for field in ('major', 'minor', 'micro', 'releaselevel', 'serial')
            },
            'executable': os.path.join(
                sysconfig.get_path('scripts'),
                os.path.basename(sys.executable),
            ),
            'stdlib': sysconfig.get_path('stdlib'),
        },
    }


def toml_dump(fd: io.TextIOBase, data: ConfigData) -> None:
    """**Very** basic TOML writer. It only implements what is necessary for this use-case."""
    def toml_repr(obj: object) -> str:
        if isinstance(obj, dict):
            return '{ ' + ', '.join(f'{k} = {toml_repr(v)}' for k, v in obj.items()) + ' }'
        else:
            return repr(obj)

    for section, entries in data.items():
        print(f'[{section}]', file=fd)
        for name, value in entries.items():
            print(f'{name} = {toml_repr(value)}', file=fd)
        fd.write('\n')


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('output_path', metavar='FILE')

    args = parser.parse_args()

    config = generic_info()

    # write TOML with our very basic writer
    with open(args.output_path, 'w') as f:
        toml_dump(f, config)

    # try to load the data as a sanity check to verify our writer outputed, at least, valid TOML
    with open(args.output_path, 'rb') as f:
        parsed = tomllib.load(f)


if __name__ == '__main__':
    main()
