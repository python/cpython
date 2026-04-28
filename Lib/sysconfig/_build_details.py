"""Helper module to generate and manipulate build-details.json data (see PEP 739)."""

from __future__ import annotations

import argparse
import dataclasses
import importlib.machinery
import json
import os
import pathlib
import sys
import sysconfig
import typing
import types

from typing import Any, Callable, ClassVar, Literal, NoDefault, Self, Type


# Types


class _NamespaceType(typing.Protocol):
    __dict__: dict[str, Any]


_PathType = str | os.PathLike[str]


# Helpers


class _FieldTransform[GetT, SetT]:
    """Descriptor that transforms values into the desired type.

    The dataclasses API does not provide a way to accept a different types in
    the constructor, and convert them to the field type. This is a very common
    functionality — eg. accepting str or os.PathLike in pathlib.Path fields.
    Such scenarios require us to disable the automatic __init__ generation,
    which is not ergonomic, requiring us to write a large amount of boilerplate.
    Alternatively, we can disable init inclusion for such fields, and define
    extra "init-only" fields to receive the value, allowing us to set the
    transformed field value in __post_init__. However, this requires the
    __init__ arguments to have a different name than the actual field.

    This descriptor provides a more ergonomic way to achieve this functionality.
    It allows defining a custom setter function, which can receive different
    types. It's similar to the functionality provided by "property".

    For better ergonomics when dealing with optional types, the transform
    function is not called when setting the value to None.
    """

    # XXX: Repurpose typing.NoDefault as our sentinel here.
    def __init__(self, fn: Callable[[SetT], GetT], default: GetT | NoDefault = NoDefault) -> None:
        self._fn = fn
        self._default_value = default
        self._value: dict[int, GetT] = {}

    def __set_name__(self, cls: Type[object], name: str) -> None:
        self.__name__ = name

    def __get__(self, obj: object | None, cls: Type[object]) -> GetT:
        if obj and id(obj) in self._value:
            return self._value[id(obj)]
        if self._default_value is not NoDefault:
            return self._default_value
        assert self.__name__, 'missing __set_name__'
        raise AttributeError(f'{cls.__name__!r} has no attribute {self.__name__!r}') from None

    def __set__(self, obj: object, value: SetT) -> None:
        self._value[id(obj)] = self._fn(value) if value else None


# Common data types.


class VersionInfo(typing.NamedTuple):
    major: int
    minor: int
    micro: int
    releaselevel: Literal['alpha', 'beta', 'candidate', 'final']
    serial: int

    @classmethod
    def from_object(cls, obj: object) -> VersionInfo:
        return cls(obj.major, obj.minor, obj.micro, obj.releaselevel, obj.serial)


@dataclasses.dataclass(kw_only=True)
class Implementation:
    name: str
    version: _FieldTransform[VersionInfo, object] = _FieldTransform(VersionInfo.from_object)
    hexversion: int
    cache_tag: str

    @classmethod
    def from_object(cls, obj: _NamespaceType) -> Implementation:
        """Creates a Implementation instance from a sys.implementation namespace object.

        The namespace object must define __dict__ (eg. SimpleNamespace instances).

        If the namespace object defines extra fields, a new dataclass type is
        created on-the-fly. This type subclasses Implementation, and defines the
        extra fields.
        """
        try:
            fields_dict = vars(obj)
        except TypeError:
            raise TypeError('object must define __dict__') from None

        obj_fields = set(fields_dict)
        cls_fields = {field.name for field in dataclasses.fields(cls)}

        if missing := (cls_fields - obj_fields):
            missing_list_repr = ', '.join(sorted(missing, key=cls._fields.index))
            raise ValueError(f'{obj} is missing the following fields: {missing_list_repr}')

        if obj_fields == cls_fields:
            return cls(**fields_dict)

        new_cls = dataclasses.make_dataclass(
            cls_name=f'Implementation{id(obj)}',
            fields=sorted(obj_fields - cls_fields, key=list(obj_fields).index),
            bases=(cls,),
        )
        return new_cls(**fields_dict)


# Base schema data structure definitions.


class SchemaVersion(typing.NamedTuple):
    major: int
    minor: int

    def __str__(self) -> str:
        return f'{self.major}.{self.minor}'

    def __repr__(self) -> str:
        return f'SchemaVersion({self.major}.{self.minor})'


@dataclasses.dataclass(kw_only=True)
class SchemaBase:
    """Base type for versioned data structures."""

    schema_version: ClassVar[SchemaVersion]

    def __init_subclass__(cls, schema: SchemaVersion):
        cls.schema_version = SchemaVersion(*schema)

    def _validate(self) -> Exception:
        # Check that all versioned objects in the dataclass fields are compatible
        for field in dataclasses.fields(self):
            value = getattr(self, field.name)
            if not isinstance(value, SchemaBase):
                continue
            if value.schema_version != self.schema_version:
                yield ValueError(
                    f'{self}.{key}: {value} has an incompatible schema version '
                    f'{str(value.schema_version)!r} (expected {str(self.schema_version)!r})'
                )

    def __post_init__(self) -> None:
        if validation_excs := list(self._validate()):
            raise ExceptionGroup('Compatibility validation failed', validation_excs)


# Schema definitions - V1


@dataclasses.dataclass(kw_only=True)
class LanguageV1(SchemaBase, schema=(1, 0)):
    """PEP 739 — "language" field (https://peps.python.org/pep-0739/#language)"""
    version: str
    version_info: VersionInfo | None = None


@dataclasses.dataclass(kw_only=True)
class ABIV1(SchemaBase, schema=(1, 0)):
    """PEP 739 — "abi" field (https://peps.python.org/pep-0739/#abi)"""
    flags: tuple[str, ...]
    extension_suffix: str | None = None
    stable_abi_suffix: str | None = None


@dataclasses.dataclass(kw_only=True)
class SuffixesV1(SchemaBase, schema=(1, 0)):
    """PEP 739 — "suffixes" field (https://peps.python.org/pep-0739/#suffixes)"""
    source: tuple[str, ...] | None = None
    bytecode: tuple[str, ...] | None = None
    optimized_bytecode: tuple[str, ...] | None = None
    debug_bytecode: tuple[str, ...] | None = None
    extensions: tuple[str, ...] | None = None


@dataclasses.dataclass(kw_only=True)
class LibpythonV1(SchemaBase, schema=(1, 0)):
    """PEP 739 — "libpython" field (https://peps.python.org/pep-0739/#libpython)"""
    dynamic: _FieldTransform[pathlib.Path | None, _PathType] = _FieldTransform(pathlib.Path, default=None)
    dynamic_stableabi: _FieldTransform[pathlib.Path | None, _PathType] = _FieldTransform(pathlib.Path, default=None)
    static: _FieldTransform[pathlib.Path | None, _PathType] = _FieldTransform(pathlib.Path, default=None)
    link_extensions: bool | None = None


@dataclasses.dataclass(kw_only=True)
class CAPIV1(SchemaBase, schema=(1, 0)):
    """PEP 739 — "capi" field (https://peps.python.org/pep-0739/#capi)"""
    headers: _FieldTransform[pathlib.Path, _PathType] = _FieldTransform(pathlib.Path)
    pkgconfig_path: _FieldTransform[pathlib.Path | None, _PathType] = _FieldTransform(pathlib.Path, default=None)


@dataclasses.dataclass(kw_only=True)
class BuildDetailsV1(SchemaBase, schema=(1, 0)):
    """PEP 739 — root object (https://peps.python.org/pep-0739/#format)"""
    schema_version: SchemaVersion

    base_prefix: _FieldTransform[pathlib.Path, _PathType] = _FieldTransform(pathlib.Path)
    base_interpreter: _FieldTransform[pathlib.Path | None, _PathType] = _FieldTransform(pathlib.Path, default=None)
    platform: str

    language: LanguageV1
    implementation: Implementation
    abi: ABIV1 | None = None
    suffixes: SuffixesV1 | None = None
    libpython: LibpythonV1 | None = None
    c_api: CAPIV1 | None = None

    arbitrary_data: SchemaBase | None = None

    def _relocate(self, obj: SchemaBase, origin: pathlib.Path) -> None:
        changes = {}
        for field in dataclasses.fields(obj):
            if not field.init or field.name == 'base_prefix':
                continue
            value = getattr(obj, field.name)
            if isinstance(value, pathlib.Path):
                changes[field.name] = value.relative_to(origin)
            elif isinstance(value, SchemaBase):
                changes[field.name] = self._relocate(value, origin)
        return dataclasses.replace(obj, **changes)

    def as_relocatable(self) -> Self:
        """Returns a copy of the object where all the paths are relative to base_prefix."""
        return self._relocate(self, origin=self.base_prefix)

    def _dataclass_asdict(self, items: list[tuple[str, Any]]) -> dict[str, Any]:
        data = dict(items)
        for key, value in items:
            # Filter out empty sections.
            if not value:
                data.pop(key, None)
            # Convert schema_version to string.
            elif isinstance(value, SchemaVersion):
                data[key] = str(value)
            # Convert VersionInfo namedtuple to dict.
            elif isinstance(value, VersionInfo):
                data[key] = value._asdict()
            # Convert paths to string.
            elif isinstance(value, pathlib.Path):
                data[key] = os.fspath(value)
                # Join '.' so that the it is formated as './path' instead of 'path'.
                if not value.is_absolute():
                    data[key] = os.path.join('.', data[key])
        return data

    def as_json(self) -> str:
        """Serialize data as JSON."""
        data = dataclasses.asdict(self, dict_factory=self._dataclass_asdict)
        return json.dumps(data, indent=2)

    @classmethod
    def from_interpreter(cls) -> BuildDetailsV1:
        """Generate the build-details.json data for the running interpreter."""
        # TODO: Abstract data sources, in order to be able to re-use this code for other environments.
        default_scheme_paths = types.SimpleNamespace({
            key: pathlib.Path(value)
            for key, value in sysconfig.get_paths().items()
        })

        LDLIBRARY = sysconfig.get_config_var('LDLIBRARY')
        LIBRARY = sysconfig.get_config_var('LIBRARY')
        PY3LIBRARY = sysconfig.get_config_var('PY3LIBRARY')
        LIBPYTHON = sysconfig.get_config_var('LIBPYTHON')

        LIBDIR = pathlib.Path(sysconfig.get_config_var('LIBDIR'))
        LIBPC = pathlib.Path(sysconfig.get_config_var('LIBPC'))
        INCLUDEPY = pathlib.Path(sysconfig.get_config_var('INCLUDEPY'))

        interpreter_name = 'python' + sysconfig.get_config_var('LDVERSION') + sysconfig.get_config_var('EXE')

        data = types.SimpleNamespace()

        data.base_prefix = sysconfig.get_config_var('installed_base')
        data.base_interpreter = default_scheme_paths.scripts / interpreter_name
        data.platform = sysconfig.get_platform()
        data.language = LanguageV1(
            version=sysconfig.get_python_version(),
            version_info=VersionInfo.from_object(sys.version_info),
        )
        data.implementation = Implementation.from_object(sys.implementation)
        data.abi = ABIV1(
            flags=tuple(sys.abiflags),
        )
        data.libpython = LibpythonV1()
        data.suffixes = SuffixesV1(
            source=importlib.machinery.SOURCE_SUFFIXES,
            bytecode=importlib.machinery.BYTECODE_SUFFIXES,
            #optimized_bytecode=importlib.machinery.OPTIMIZED_BYTECODE_SUFFIXES,
            #debug_bytecode=importlib.machinery.DEBUG_BYTECODE_SUFFIXES,
            extensions=importlib.machinery.EXTENSION_SUFFIXES,
        )
        data.c_api = CAPIV1(
            headers=INCLUDEPY,
        )

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
            data.abi.extension_suffix = sysconfig.get_config_var('EXT_SUFFIX')

            # EXTENSION_SUFFIXES has been constant for a long time, and currently we
            # don't have a better information source to find the stable ABI suffix.
            for suffix in importlib.machinery.EXTENSION_SUFFIXES:
                if suffix.startswith('.abi'):
                    data.abi.stable_abi_suffix = suffix
                    break

            data.libpython.dynamic = LIBDIR / LDLIBRARY
            # FIXME: Not sure if windows has a different dll for the stable ABI, and
            #        even if it does, currently we don't have a way to get its name.
            if PY3LIBRARY:
                data.libpython.dynamic_stableabi = LIBDIR / PY3LIBRARY

            # Os POSIX, this is defined by the LIBPYTHON Makefile variable not being
            # empty. On Windows, don't link extensions — LIBPYTHON won't be defined,
            data.libpython.link_extensions = bool(LIBPYTHON)

        if has_static_library:
            data.libpython.static = LIBDIR / LIBRARY

        if LIBPC:
            data.c_api.pkgconfig_path = LIBPC

        return BuildDetailsV1(**vars(data))


# Default alias.


class _BuildDetailsProtocol(typing.Protocol):
    def as_relocatable(self) -> Self:
        """Returns a copy of the object where all the paths are relative to base_prefix."""
        return self._relocate(self, origin=self.base_prefix)

    def as_json(self) -> str:
        return json.dumps(self, indent=2, object_hook=self._json_object_serializer)

    @classmethod
    def from_interpreter(cls) -> SchemaBase: ...


BuildDetails: _BuildDetailsProtocol = BuildDetailsV1


# Generation/CLI helpers.


def generate(path: _PathType, relative_paths=False) -> None:
    """Generates build-details.json for the current interpreter."""
    data = BuildDetails.from_interpreter()

    if relative_paths:
        data = data.as_relocatable()
        # If relative_paths is enabled, we also make base_prefix relative.
        data.base_prefix = data.base_prefix.relative_to(path)

    with open(path, 'w', encoding='utf-8') as f:
        f.write(data.as_json() + '\n')


def main() -> None:
    parser = argparse.ArgumentParser(exit_on_error=False)
    parser.add_argument('location')
    parser.add_argument(
        '--relative-paths',
        action='store_true',
        help='Whether to specify paths as absolute, or as relative paths to ``base_prefix``.',
    )

    args = parser.parse_args()

    generate(args.location, args.relative_paths)


if __name__ == '__main__':
    main()
