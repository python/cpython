import argparse
import json
import os
import sys
import sysconfig


BasicConfigItem = str | int
ConfigItem = BasicConfigItem | list[BasicConfigItem]
SectionData = dict[str, 'ConfigItem | SectionData']
ConfigData = dict[str, SectionData]


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


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('output_path', metavar='FILE')

    args = parser.parse_args()

    config = generic_info()

    with open(args.output_path, 'w') as f:
        json.dump(config, f)


if __name__ == '__main__':
    main()
