# Copyright 2023-2024 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# A simple runtime type checker.

import collections.abc
import functools
import typing


# 'isinstance' won't work in general for type variables, so we
# implement the subset that is needed by DAP.
def _check_instance(value, typevar):
    base = typing.get_origin(typevar)
    if base is None:
        return isinstance(value, typevar)
    arg_types = typing.get_args(typevar)
    if base == collections.abc.Mapping or base == typing.Mapping:
        if not isinstance(value, collections.abc.Mapping):
            return False
        assert len(arg_types) == 2
        (keytype, valuetype) = arg_types
        return all(
            _check_instance(k, keytype) and _check_instance(v, valuetype)
            for k, v in value.items()
        )
    elif base == collections.abc.Sequence or base == typing.Sequence:
        # In some places we simply use 'Sequence' without arguments.
        if not isinstance(value, base):
            return False
        if len(arg_types) == 0:
            return True
        assert len(arg_types) == 1
        arg_type = arg_types[0]
        return all(_check_instance(item, arg_type) for item in value)
    elif base == typing.Union:
        return any(_check_instance(value, arg_type) for arg_type in arg_types)
    raise TypeError("unsupported type variable '" + str(typevar) + "'")


def type_check(func):
    """A decorator that checks FUNC's argument types at runtime."""

    # The type checker relies on 'typing.get_origin', which was added
    # in Python 3.8.  (It also relies on 'typing.get_args', but that
    # was added at the same time.)
    if not hasattr(typing, "get_origin"):
        return func

    hints = typing.get_type_hints(func)
    # We don't check the return type, but we allow it in case someone
    # wants to use it on a function definition.
    if "return" in hints:
        del hints["return"]

    # Note that keyword-only is fine for our purposes, because this is
    # only used for DAP requests, and those are always called this
    # way.
    @functools.wraps(func)
    def check_arguments(**kwargs):
        for key in hints:
            # The argument might not be passed in; we could type-check
            # any default value here, but it seems fine to just rely
            # on the code being correct -- the main goal of this
            # checking is to verify JSON coming from the client.
            if key in kwargs and not _check_instance(kwargs[key], hints[key]):
                raise TypeError(
                    "value for '"
                    + key
                    + "' does not have expected type '"
                    + str(hints[key])
                    + "'"
                )
        return func(**kwargs)

    return check_arguments
