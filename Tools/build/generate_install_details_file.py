import argparse
import io
import os
import sys
import sysconfig
import tomllib


BasicConfigItem = str | int
ConfigItem = BasicConfigItem | list[BasicConfigItem]
SectionData = dict[str, 'ConfigItem | SectionData']
ConfigData = dict[str, SectionData]


# From https://toml.io/en/v1.0.0#string:
# Any Unicode character may be used except those that must be escaped:
# quotation mark, backslash, and the control characters other than tab
# (U+0000 to U+0008, U+000A to U+001F, U+007F).
_needs_escaping = {*range(0x0008 + 1), *range(0x000A, 0x001F + 1), 0x007F}
toml_string_escape_table = {i: f'\\u{i:04}' for i in _needs_escaping}
toml_string_escape_table.update({
    # characters with a custom escape sequence
    0x0008: r'\b',
    0x0009: r'\t',
    0x000A: r'\n',
    0x000C: r'\f',
    0x000D: r'\r',
    0x0022: r'\"',
    0x005C: r'\\\\',
})


def toml_dump(fd: io.TextIOBase, data: ConfigData, outer_section: str | None = None) -> None:
    """**Very** basic TOML writer. It only implements what is necessary for our use-case."""

    def toml_repr(obj: object) -> str:
        if isinstance(obj, str):
            return f'"{obj.translate(toml_string_escape_table)}"'
        else:
            return repr(obj)

    def dump_section(name: str, data: SectionData) -> None:
        subsections: dict[str, SectionData] = {}

        # don't emit a newline before the first section
        if fd.tell() != 0:
            fd.write('\n')

        # write section
        print(f'[{name}]', file=fd)
        for key, value in data.items():
            if isinstance(value, dict):
                subsections[f'{name}.{key}'] = value
            else:
                print(f'{key} = {toml_repr(value)}', file=fd)

        # write subsections
        for subsection_name, subsection_data in subsections.items():
            dump_section(subsection_name, subsection_data)

    for name, section_data in data.items():
        dump_section(name, section_data)


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

    # write TOML with our very basic writer
    with open(args.output_path, 'w') as f:
        toml_dump(f, config)

    # try to load the data as a sanity check to verify our writer outputed, at least, valid TOML
    with open(args.output_path, 'rb') as f:
        parsed = tomllib.load(f)


if __name__ == '__main__':
    main()
