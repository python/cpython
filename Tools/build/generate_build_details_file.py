import argparse
import json
import os
import sys
import sysconfig


BasicConfigItem = str | int
ConfigItem = BasicConfigItem | list[BasicConfigItem]
SectionData = dict[str, 'ConfigItem | SectionData']
ConfigData = dict[str, SectionData]


def generic_info(executable: str) -> ConfigData:
    return {
        'python': {
            'version': sys.version.split(' ')[0],
            'version_parts': {
                field: getattr(sys.version_info, field)
                for field in ('major', 'minor', 'micro', 'releaselevel', 'serial')
            },
            'executable': executable,
            'stdlib': sysconfig.get_path('stdlib'),
        },
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('output_path', metavar='FILE')
    parser.add_argument('--executable', help='The executable path on the system.')

    args = parser.parse_args()

    config = generic_info(args.executable)

    with open(args.output_path, 'w') as f:
        json.dump(config, f)


if __name__ == '__main__':
    main()
