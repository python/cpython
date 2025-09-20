"""Generate build-details.json (see PEP 739)."""

# Script initially imported from:
# https://github.com/FFY00/python-instrospection/blob/main/python_introspection/scripts/generate-build-details.py

from __future__ import annotations

import argparse
import collections
import importlib.machinery
import json
import os
import sys
import sysconfig
from pathlib import Path

TYPE_CHECKING = False
if TYPE_CHECKING:
    from typing import Any, Literal, TypeAlias

    StrPath: TypeAlias = str | os.PathLike[str]
    ValidSchemaVersion: TypeAlias = Literal['1.0']


def write_build_details(
    *,
    schema_version: ValidSchemaVersion,
    base_path: StrPath | None,
    location: StrPath,
) -> None:
    data = generate_data(schema_version)
    if base_path is not None:
        make_paths_relative(data, base_path)

    json_output = json.dumps(data, indent=2)
    with open(location, 'w', encoding='utf-8') as f:
        f.write(json_output)
        f.write('\n')


def version_info_to_dict(obj: sys._version_info) -> dict[str, Any]:
    field_names = ('major', 'minor', 'micro', 'releaselevel', 'serial')
    return {field: getattr(obj, field) for field in field_names}


def get_dict_key(container: dict[str, Any], key: str) -> dict[str, Any]:
    for part in key.split('.'):
        container = container[part]
    return container


def generate_data(
    schema_version: ValidSchemaVersion
) -> collections.defaultdict[str, Any]:
    """Generate the build-details.json data (PEP 739).

    :param schema_version: The schema version of the data we want to generate.
    """

    if schema_version != '1.0':
        raise ValueError(f'Unsupported schema_version: {schema_version}')

    data: collections.defaultdict[str, Any] = collections.defaultdict(
        lambda: collections.defaultdict(dict),
    )

    data['schema_version'] = schema_version

    data['base_prefix'] = sysconfig.get_config_var('installed_base')
    #data['base_interpreter'] = sys._base_executable
    if os.name == 'nt':
        data['base_interpreter'] = os.path.join(
            data['base_prefix'],
            os.path.basename(sys._base_executable),  # type: ignore[attr-defined]
        )
    else:
        data['base_interpreter'] = os.path.join(
            sysconfig.get_path('scripts'),
            'python' + sysconfig.get_config_var('VERSION'),
        )
    data['platform'] = sysconfig.get_platform()

    data['language']['version'] = sysconfig.get_python_version()
    data['language']['version_info'] = version_info_to_dict(sys.version_info)

    data['implementation'] = vars(sys.implementation).copy()
    data['implementation']['version'] = version_info_to_dict(sys.implementation.version)
    # Fix cross-compilation
    if '_multiarch' in data['implementation']:
        data['implementation']['_multiarch'] = sysconfig.get_config_var('MULTIARCH')

    if os.name != 'nt':
        data['abi']['flags'] = list(sys.abiflags)
    else:
        data['abi']['flags'] = []

    data['suffixes']['source'] = importlib.machinery.SOURCE_SUFFIXES
    data['suffixes']['bytecode'] = importlib.machinery.BYTECODE_SUFFIXES
    #data['suffixes']['optimized_bytecode'] = importlib.machinery.OPTIMIZED_BYTECODE_SUFFIXES
    #data['suffixes']['debug_bytecode'] = importlib.machinery.DEBUG_BYTECODE_SUFFIXES
    data['suffixes']['extensions'] = importlib.machinery.EXTENSION_SUFFIXES

    if os.name == 'nt':
        LIBDIR = data['base_prefix']
    else:
        LIBDIR = sysconfig.get_config_var('LIBDIR')
    LDLIBRARY = sysconfig.get_config_var('LDLIBRARY')
    LIBRARY = sysconfig.get_config_var('LIBRARY')
    PY3LIBRARY = sysconfig.get_config_var('PY3LIBRARY')
    LIBPYTHON = sysconfig.get_config_var('LIBPYTHON')
    LIBPC = sysconfig.get_config_var('LIBPC')
    if os.name == 'nt':
        INCLUDEPY = os.path.join(data['base_prefix'], 'include')
    else:
        INCLUDEPY = sysconfig.get_config_var('INCLUDEPY')

    if os.name == 'posix':
        # On POSIX, LIBRARY is always the static library, while LDLIBRARY is the
        # dynamic library if enabled, otherwise it's the static library.
        # If LIBRARY != LDLIBRARY, support for the dynamic library is enabled.
        has_dynamic_library = LDLIBRARY != LIBRARY
        has_static_library = sysconfig.get_config_var('STATIC_LIBPYTHON')
    elif os.name == 'nt':
        # Windows can only use a dynamic library or a static library.
        # If it's using a dynamic library, sys.dllhandle will be set.
        # Static builds on Windows are not really well supported, though.
        # More context: https://github.com/python/cpython/issues/110234
        has_dynamic_library = hasattr(sys, 'dllhandle')
        has_static_library = not has_dynamic_library
    else:
        raise NotADirectoryError(f'Unknown platform: {os.name}')

    # On POSIX, EXT_SUFFIX is set regardless if extension modules are supported
    # or not, and on Windows older versions of CPython only set EXT_SUFFIX when
    # extension modules are supported, but newer versions of CPython set it
    # regardless.
    #
    # We only want to set abi.extension_suffix and stable_abi_suffix if
    # extension modules are supported.
    if has_dynamic_library:
        data['abi']['extension_suffix'] = sysconfig.get_config_var('EXT_SUFFIX')

        # EXTENSION_SUFFIXES has been constant for a long time, and currently we
        # don't have a better information source to find the stable ABI suffix.
        for suffix in importlib.machinery.EXTENSION_SUFFIXES:
            if suffix.startswith('.abi'):
                data['abi']['stable_abi_suffix'] = suffix
                break

        data['libpython']['dynamic'] = os.path.join(LIBDIR, LDLIBRARY)
        # FIXME: Not sure if windows has a different dll for the stable ABI, and
        #        even if it does, currently we don't have a way to get its name.
        if PY3LIBRARY:
            data['libpython']['dynamic_stableabi'] = os.path.join(LIBDIR, PY3LIBRARY)

        # Os POSIX, this is defined by the LIBPYTHON Makefile variable not being
        # empty. On Windows, don't link extensions — LIBPYTHON won't be defined,
        data['libpython']['link_extensions'] = bool(LIBPYTHON)

    if has_static_library:
        data['libpython']['static'] = os.path.join(LIBDIR, LIBRARY)

    data['c_api']['headers'] = INCLUDEPY
    if LIBPC:
        data['c_api']['pkgconfig_path'] = LIBPC

    return data


def make_paths_relative(data: dict[str, Any], base_path: StrPath | None = None) -> None:
    base_prefix = data['base_prefix']

    # Update path values to make them relative to base_prefix
    PATH_KEYS = (
        'base_interpreter',
        'libpython.dynamic',
        'libpython.dynamic_stableabi',
        'libpython.static',
        'c_api.headers',
        'c_api.pkgconfig_path',
    )
    for entry in PATH_KEYS:
        *parents, child = entry.split('.')
        # Get the key container object
        try:
            container = data
            for part in parents:
                container = container[part]
            if child not in container:
                raise KeyError(child)
            current_path = container[child]
        except KeyError:
            continue
        # Get the relative path
        new_path = relative_path(current_path, base_prefix)
        # Join '.' so that the path is formated as './path' instead of 'path'
        new_path = os.path.join('.', new_path)
        container[child] = new_path

    if base_path:
        # Make base_prefix relative to the base_path directory
        config_dir = Path(base_path).resolve().parent
        data['base_prefix'] = relative_path(base_prefix, config_dir)

def relative_path(path: StrPath, base: StrPath) -> str:
    if os.name != 'nt':
        return os.path.relpath(path, base)

    # There are no relative paths between drives on Windows.
    path_drv, _ = os.path.splitdrive(path)
    base_drv, _ = os.path.splitdrive(base)
    if path_drv.lower() == base_drv.lower():
        return os.path.relpath(path, base)

    return os.fspath(path)


def main() -> None:
    parser = argparse.ArgumentParser(exit_on_error=False)
    parser.add_argument('location')
    parser.add_argument(
        '--schema-version',
        default='1.0',
        help='Schema version of the build-details.json file to generate.',
    )
    parser.add_argument(
        '--relative-paths',
        action='store_true',
        help='Whether to specify paths as absolute, or as relative paths to ``base_prefix``.',
    )
    parser.add_argument(
        '--config-file-path',
        default=None,
        help='If specified, ``base_prefix`` will be set as a relative path to the given config file path.',
    )

    args = parser.parse_args()
    if os.name == 'nt':
        # Windows builds are relocatable; always make paths relative.
        base_path = args.config_file_path or args.location
    elif args.relative_paths:
        base_path = args.config_file_path
    else:
        base_path = None
    write_build_details(
        schema_version=args.schema_version,
        base_path=base_path,
        location=args.location,
    )


if __name__ == '__main__':
    main()
