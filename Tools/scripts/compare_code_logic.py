#!/usr/bin/env python3
"""Compares Python code *logic*, ignoring line number and filename changes.

Intended to be used as an equivalence test for sources before and after a
semantic preserving code transformation tool such as an auto-formatter has
been run on them.

Usage:

   compare_code_logic.py before_formatting.py after_formatting.py

As a library, see :func:`compare_bytecode_ignoring_line_number_changes`.

Ideally this should be run under the Python interpreter that will later be
used to execute the code in question as py_compile failures could be version
specific and count as a bad transformation.
"""

import marshal
import py_compile
import sys
import tempfile
from types import CodeType as _Code
from typing import Iterable, Mapping, Sequence


class ComparisonError(Exception):
    """Raised when a code comparison fails with details where it happened."""

    def __init__(self, *args):
        super().__init__(*args)
        self.message = args[0]

    def __str__(self):
        return f'{type(self).__name__}: {self.message}'

    def __repr__(self):
        return f'{type(self).__name__}({self.args!r})'


_MARSHAL_OFFSET = 16 if sys.version_info[:2] >= (3, 7) else 12  # PEP-552


def _codeobj_from_pyc_data(pyc_data: bytes) -> _Code:
    return marshal.loads(pyc_data[_MARSHAL_OFFSET:])


def _compile_and_get_pyc_data(py_filename: str) -> bytes:
    with tempfile.NamedTemporaryFile(suffix='.pyc', delete=True) as temp_pyc:
        py_compile.compile(py_filename, cfile=temp_pyc.name, doraise=True)
        with open(temp_pyc.name, mode='rb') as pyc_file:
            return pyc_file.read()


def _py_file_to_code(py_filename: str) -> _Code:
    pyc_data = _compile_and_get_pyc_data(py_filename)
    return _codeobj_from_pyc_data(pyc_data)


_ATTRS_OF_CODE_TO_IGNORE = frozenset(
    {'co_lnotab', 'co_firstlineno', 'co_filename'})


def _compare_code_obj_values(value_a, value_b, error_location: str) -> bool:
    """Many code object attribute values are containers; recursively compare."""
    type_a = type(value_a)
    type_b = type(value_b)
    if type_a != type_b:
        raise ComparisonError(
            f'Type mismatch in {error_location}: {type_a} vs {type_b}.')
    if isinstance(value_a, (int, str, bytes)):
        if value_a != value_b:
            raise ComparisonError(
                f'inequality in {error_location}: {value_a} != {value_b}')
    if isinstance(value_a, Mapping) and value_a != value_b:
        # unexpected... code objects should not have dict attributes.
        raise ComparisonError(
            f'Unexpected dict in {error_location}:\n{value_a}\n !=\n{value_b}')
    elif isinstance(value_a, Iterable):  # usually a tuple
        if len(value_a) != len(value_b):
            raise ComparisonError(f'Lengths differ in {error_location}:\n'
                                  f'{value_a}\n !=\n{value_b}')
        for a, b in zip(value_a, value_b):
            if isinstance(a, _Code) and isinstance(b, _Code):
                _compare_code_obj_ignoring_line_numbers(
                    a, b, error_location=f'{error_location}.{type(a)}')
            else:
                if isinstance(a, (int, str, bytes)):
                    if a != b:
                        raise ComparisonError(
                            f'inequality in {error_location}.{type(value_a)}:'
                            f' {a} != {b}')
                elif a != b:
                    _compare_code_obj_values(
                        a, b, error_location=f'{error_location}.{type(a)}')


def _compare_code_obj_ignoring_line_numbers(a: _Code,
                                            b: _Code,
                                            *,
                                            error_location='code') -> None:
    """Compare two code objects recursively, ignoring line number information.

    Args:
        a: Code object to compare.
        b: Code object to compare.
        error_location: Dotted "path" the object being compared.  Do not supply
          this yourself, it is used by our recursion to be able to indicate
          where we found the problem in the exception message.

    Raises:
        ComparisonError: This exception contains the details of the first
            miscomparison encountered.  There may be others; we fail fast.
    """
    if a == b:  # Shortcut for a potential common case.
        return

    # More in depth comparison required to find important differences.
    attr_names = {name for name in dir(a) + dir(b) if name.startswith('co_')}
    attr_names -= _ATTRS_OF_CODE_TO_IGNORE

    # Sorting keeps the error message consistent run after run.Putting `co_code`
    # last more human friendly errors such as `co_consts` or `co_cellvars`
    # changes in the error message when present rather than the co_code binary
    # difference that accompanies those.  Doesn't matter, but it feels nice.
    attr_names.remove('co_code')  # we put it back below...
    attr_names = sorted(attr_names)
    attr_names.append('co_code')

    for name in attr_names:
        value_a = getattr(a, name)
        value_b = getattr(b, name)
        if isinstance(value_a, (Mapping, Iterable, _Code, int, str, bytes)):
            _compare_code_obj_values(
                value_a, value_b, error_location=f'{error_location}.{name}')
        else:
            raise ComparisonError(
                'unexpected attr type on code object for '
                f'{error_location}.{name}: type(value_a) {value_a}.')


def compare_bytecode_ignoring_line_number_changes(
        before_py_filename: str, after_py_filename: str) -> None:
    """Compares the bytecode generated for two Python source files.

    Line number changes and the different filenames are ignored.

    Because we are comparing at the bytecode level, non-logical changes such as
    insignificant whitespace, comments, or reflowed potentially multi-line or
    implicitly joined string literals will pass the test as those do not impact
    the compiled Python bytecode.

    Args:
        before_py_filename: Pathname to a Python source file.
        after_py_filename: Pathname to a Python source file.

    Raises:
        ComparisonError: This exception contains the details of the first
            miscomparison encountered.  There may be others; we fail fast.
    """
    before_code = _py_file_to_code(before_py_filename)
    after_code = _py_file_to_code(after_py_filename)
    _compare_code_obj_ignoring_line_numbers(before_code, after_code)


def main(argv: Sequence[str]):
    before_filename, after_filename = argv[:2]

    try:
        compare_bytecode_ignoring_line_number_changes(before_filename,
                                                      after_filename)
    except ComparisonError as err:
        print(err)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
