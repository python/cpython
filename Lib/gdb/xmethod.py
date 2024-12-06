# Python side of the support for xmethods.
# Copyright (C) 2013-2024 Free Software Foundation, Inc.

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

"""Utilities for defining xmethods"""

import re

import gdb


class XMethod(object):
    """Base class (or a template) for an xmethod description.

    Currently, the description requires only the 'name' and 'enabled'
    attributes.  Description objects are managed by 'XMethodMatcher'
    objects (see below).  Note that this is only a template for the
    interface of the XMethodMatcher.methods objects.  One could use
    this class or choose to use an object which supports this exact same
    interface.  Also, an XMethodMatcher can choose not use it 'methods'
    attribute.  In such cases this class (or an equivalent) is not used.

    Attributes:
        name: The name of the xmethod.
        enabled: A boolean indicating if the xmethod is enabled.
    """

    def __init__(self, name):
        self.name = name
        self.enabled = True


class XMethodMatcher(object):
    """Abstract base class for matching an xmethod.

    When looking for xmethods, GDB invokes the `match' method of a
    registered xmethod matcher to match the object type and method name.
    The `match' method in concrete classes derived from this class should
    return an `XMethodWorker' object, or a list of `XMethodWorker'
    objects if there is a match (see below for 'XMethodWorker' class).

    Attributes:
        name: The name of the matcher.
        enabled: A boolean indicating if the matcher is enabled.
        methods: A sequence of objects of type 'XMethod', or objects
            which have at least the attributes of an 'XMethod' object.
            This list is used by the 'enable'/'disable'/'info' commands to
            enable/disable/list the xmethods registered with GDB.  See
            the 'match' method below to know how this sequence is used.
            This attribute is None if the matcher chooses not have any
            xmethods managed by it.
    """

    def __init__(self, name):
        """
        Args:
            name: An identifying name for the xmethod or the group of
                  xmethods returned by the `match' method.
        """
        self.name = name
        self.enabled = True
        self.methods = None

    def match(self, class_type, method_name):
        """Match class type and method name.

        In derived classes, it should return an XMethodWorker object, or a
        sequence of 'XMethodWorker' objects.  Only those xmethod workers
        whose corresponding 'XMethod' descriptor object is enabled should be
        returned.

        Args:
            class_type: The class type (gdb.Type object) to match.
            method_name: The name (string) of the method to match.
        """
        raise NotImplementedError("XMethodMatcher match")


class XMethodWorker(object):
    """Base class for all xmethod workers defined in Python.

    An xmethod worker is an object which matches the method arguments, and
    invokes the method when GDB wants it to.  Internally, GDB first invokes the
    'get_arg_types' method to perform overload resolution.  If GDB selects to
    invoke this Python xmethod, then it invokes it via the overridden
    '__call__' method.  The 'get_result_type' method is used to implement
    'ptype' on the xmethod.

    Derived classes should override the 'get_arg_types', 'get_result_type'
    and '__call__' methods.
    """

    def get_arg_types(self):
        """Return arguments types of an xmethod.

        A sequence of gdb.Type objects corresponding to the arguments of the
        xmethod are returned.  If the xmethod takes no arguments, then 'None'
        or an empty sequence is returned.  If the xmethod takes only a single
        argument, then a gdb.Type object or a sequence with a single gdb.Type
        element is returned.
        """
        raise NotImplementedError("XMethodWorker get_arg_types")

    def get_result_type(self, *args):
        """Return the type of the result of the xmethod.

        Args:
            args: Arguments to the method.  Each element of the tuple is a
                gdb.Value object.  The first element is the 'this' pointer
                value.  These are the same arguments passed to '__call__'.

        Returns:
            A gdb.Type object representing the type of the result of the
            xmethod.
        """
        raise NotImplementedError("XMethodWorker get_result_type")

    def __call__(self, *args):
        """Invoke the xmethod.

        Args:
            args: Arguments to the method.  Each element of the tuple is a
                gdb.Value object.  The first element is the 'this' pointer
                value.

        Returns:
            A gdb.Value corresponding to the value returned by the xmethod.
            Returns 'None' if the method does not return anything.
        """
        raise NotImplementedError("XMethodWorker __call__")


class SimpleXMethodMatcher(XMethodMatcher):
    """A utility class to implement simple xmethod mathers and workers.

    See the __init__ method below for information on how instances of this
    class can be used.

    For simple classes and methods, one can choose to use this class.  For
    complex xmethods, which need to replace/implement template methods on
    possibly template classes, one should implement their own xmethod
    matchers and workers.  See py-xmethods.py in testsuite/gdb.python
    directory of the GDB source tree for examples.
    """

    class SimpleXMethodWorker(XMethodWorker):
        def __init__(self, method_function, arg_types):
            self._arg_types = arg_types
            self._method_function = method_function

        def get_arg_types(self):
            return self._arg_types

        def __call__(self, *args):
            return self._method_function(*args)

    def __init__(
        self, name, class_matcher, method_matcher, method_function, *arg_types
    ):
        """
        Args:
            name: Name of the xmethod matcher.
            class_matcher: A regular expression used to match the name of the
                class whose method this xmethod is implementing/replacing.
            method_matcher: A regular expression used to match the name of the
                method this xmethod is implementing/replacing.
            method_function: A Python callable which would be called via the
                'invoke' method of the worker returned by the objects of this
                class.  This callable should accept the object (*this) as the
                first argument followed by the rest of the arguments to the
                method. All arguments to this function should be gdb.Value
                objects.
            arg_types: The gdb.Type objects corresponding to the arguments that
                this xmethod takes. It can be None, or an empty sequence,
                or a single gdb.Type object, or a sequence of gdb.Type objects.
        """
        XMethodMatcher.__init__(self, name)
        assert callable(method_function), (
            "The 'method_function' argument to 'SimpleXMethodMatcher' "
            "__init__ method should be a callable."
        )
        self._method_function = method_function
        self._class_matcher = class_matcher
        self._method_matcher = method_matcher
        self._arg_types = arg_types

    def match(self, class_type, method_name):
        cm = re.match(self._class_matcher, str(class_type.unqualified().tag))
        mm = re.match(self._method_matcher, method_name)
        if cm and mm:
            return SimpleXMethodMatcher.SimpleXMethodWorker(
                self._method_function, self._arg_types
            )


# A helper function for register_xmethod_matcher which returns an error
# object if MATCHER is not having the requisite attributes in the proper
# format.


def _validate_xmethod_matcher(matcher):
    if not hasattr(matcher, "match"):
        return TypeError("Xmethod matcher is missing method: match")
    if not hasattr(matcher, "name"):
        return TypeError("Xmethod matcher is missing attribute: name")
    if not hasattr(matcher, "enabled"):
        return TypeError("Xmethod matcher is missing attribute: enabled")
    if not isinstance(matcher.name, str):
        return TypeError("Attribute 'name' of xmethod matcher is not a " "string")
    if matcher.name.find(";") >= 0:
        return ValueError("Xmethod matcher name cannot contain ';' in it")


# A helper function for register_xmethod_matcher which looks up an
# xmethod matcher with NAME in LOCUS.  Returns the index of the xmethod
# matcher in 'xmethods' sequence attribute of the LOCUS.  If NAME is not
# found in LOCUS, then -1 is returned.


def _lookup_xmethod_matcher(locus, name):
    for i in range(0, len(locus.xmethods)):
        if locus.xmethods[i].name == name:
            return i
    return -1


def register_xmethod_matcher(locus, matcher, replace=False):
    """Registers a xmethod matcher MATCHER with a LOCUS.

    Arguments:
        locus: The locus in which the xmethods should be registered.
            It can be 'None' to indicate that the xmethods should be
            registered globally. Or, it could be a gdb.Objfile or a
            gdb.Progspace object in which the xmethods should be
            registered.
        matcher: The xmethod matcher to register with the LOCUS.  It
            should be an instance of 'XMethodMatcher' class.
        replace: If True, replace any existing xmethod matcher with the
            same name in the locus.  Otherwise, if a matcher with the same name
            exists in the locus, raise an exception.
    """
    err = _validate_xmethod_matcher(matcher)
    if err:
        raise err
    if not locus:
        locus = gdb
    if locus == gdb:
        locus_name = "global"
    else:
        locus_name = locus.filename
    index = _lookup_xmethod_matcher(locus, matcher.name)
    if index >= 0:
        if replace:
            del locus.xmethods[index]
        else:
            raise RuntimeError(
                "Xmethod matcher already registered with "
                "%s: %s" % (locus_name, matcher.name)
            )
    if gdb.parameter("verbose"):
        gdb.write("Registering xmethod matcher '%s' with %s' ...\n")
    locus.xmethods.insert(0, matcher)
