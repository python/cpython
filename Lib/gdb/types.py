# Type utilities.
# Copyright (C) 2010-2024 Free Software Foundation, Inc.

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

"""Utilities for working with gdb.Types."""

import gdb


def get_basic_type(type_):
    """Return the "basic" type of a type.

    Arguments:
        type_: The type to reduce to its basic type.

    Returns:
        type_ with const/volatile is stripped away,
        and typedefs/references converted to the underlying type.
    """

    while (
        type_.code == gdb.TYPE_CODE_REF
        or type_.code == gdb.TYPE_CODE_RVALUE_REF
        or type_.code == gdb.TYPE_CODE_TYPEDEF
    ):
        if type_.code == gdb.TYPE_CODE_REF or type_.code == gdb.TYPE_CODE_RVALUE_REF:
            type_ = type_.target()
        else:
            type_ = type_.strip_typedefs()
    return type_.unqualified()


def has_field(type_, field):
    """Return True if a type has the specified field.

    Arguments:
        type_: The type to examine.
            It must be one of gdb.TYPE_CODE_STRUCT, gdb.TYPE_CODE_UNION.
        field: The name of the field to look up.

    Returns:
        True if the field is present either in type_ or any baseclass.

    Raises:
        TypeError: The type is not a struct or union.
    """

    type_ = get_basic_type(type_)
    if type_.code != gdb.TYPE_CODE_STRUCT and type_.code != gdb.TYPE_CODE_UNION:
        raise TypeError("not a struct or union")
    for f in type_.fields():
        if f.is_base_class:
            if has_field(f.type, field):
                return True
        else:
            # NOTE: f.name could be None
            if f.name == field:
                return True
    return False


def make_enum_dict(enum_type):
    """Return a dictionary from a program's enum type.

    Arguments:
        enum_type: The enum to compute the dictionary for.

    Returns:
        The dictionary of the enum.

    Raises:
        TypeError: The type is not an enum.
    """

    if enum_type.code != gdb.TYPE_CODE_ENUM:
        raise TypeError("not an enum type")
    enum_dict = {}
    for field in enum_type.fields():
        # The enum's value is stored in "enumval".
        enum_dict[field.name] = field.enumval
    return enum_dict


def deep_items(type_):
    """Return an iterator that recursively traverses anonymous fields.

    Arguments:
        type_: The type to traverse.  It should be one of
        gdb.TYPE_CODE_STRUCT or gdb.TYPE_CODE_UNION.

    Returns:
        an iterator similar to gdb.Type.iteritems(), i.e., it returns
        pairs of key, value, but for any anonymous struct or union
        field that field is traversed recursively, depth-first.
    """
    for k, v in type_.iteritems():
        if k:
            yield k, v
        else:
            for i in deep_items(v.type):
                yield i


class TypePrinter(object):
    """The base class for type printers.

    Instances of this type can be used to substitute type names during
    'ptype'.

    A type printer must have at least 'name' and 'enabled' attributes,
    and supply an 'instantiate' method.

    The 'instantiate' method must either return None, or return an
    object which has a 'recognize' method.  This method must accept a
    gdb.Type argument and either return None, meaning that the type
    was not recognized, or a string naming the type.
    """

    def __init__(self, name):
        self.name = name
        self.enabled = True

    def instantiate(self):
        return None


# Helper function for computing the list of type recognizers.
def _get_some_type_recognizers(result, plist):
    for printer in plist:
        if printer.enabled:
            inst = printer.instantiate()
            if inst is not None:
                result.append(inst)
    return None


def get_type_recognizers():
    "Return a list of the enabled type recognizers for the current context."
    result = []

    # First try the objfiles.
    for objfile in gdb.objfiles():
        _get_some_type_recognizers(result, objfile.type_printers)
    # Now try the program space.
    _get_some_type_recognizers(result, gdb.current_progspace().type_printers)
    # Finally, globals.
    _get_some_type_recognizers(result, gdb.type_printers)

    return result


def apply_type_recognizers(recognizers, type_obj):
    """Apply the given list of type recognizers to the type TYPE_OBJ.
    If any recognizer in the list recognizes TYPE_OBJ, returns the name
    given by the recognizer.  Otherwise, this returns None."""
    for r in recognizers:
        result = r.recognize(type_obj)
        if result is not None:
            return result
    return None


def register_type_printer(locus, printer):
    """Register a type printer.
    PRINTER is the type printer instance.
    LOCUS is either an objfile, a program space, or None, indicating
    global registration."""

    if locus is None:
        locus = gdb
    locus.type_printers.insert(0, printer)
