##############################################################################
#
# Copyright (c) 2002 Zope Foundation and Contributors.
# All Rights Reserved.
#
# This software is subject to the provisions of the Zope Public License,
# Version 2.1 (ZPL).  A copy of the ZPL should accompany this distribution.
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY AND ALL EXPRESS OR IMPLIED
# WARRANTIES ARE DISCLAIMED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF TITLE, MERCHANTABILITY, AGAINST INFRINGEMENT, AND FITNESS
# FOR A PARTICULAR PURPOSE.
#
##############################################################################
"""Interface-specific exceptions
"""

__all__ = [
    # Invalid tree
    'Invalid',
    'DoesNotImplement',
    'BrokenImplementation',
    'BrokenMethodImplementation',
    'MultipleInvalid',
    # Other
    'BadImplements',
    'InvalidInterface',
]

class Invalid(Exception):
    """A specification is violated
    """


class _TargetInvalid(Invalid):
    # Internal use. Subclass this when you're describing
    # a particular target object that's invalid according
    # to a specific interface.
    #
    # For backwards compatibility, the *target* and *interface* are
    # optional, and the signatures are inconsistent in their ordering.
    #
    # We deal with the inconsistency in ordering by defining the index
    # of the two values in ``self.args``. *target* uses a marker object to
    # distinguish "not given" from "given, but None", because the latter
    # can be a value that gets passed to validation. For this reason, it must
    # always be the last argument (we detect absense by the ``IndexError``).

    _IX_INTERFACE = 0
    _IX_TARGET = 1
    # The exception to catch when indexing self.args indicating that
    # an argument was not given. If all arguments are expected,
    # a subclass should set this to ().
    _NOT_GIVEN_CATCH = IndexError
    _NOT_GIVEN = '<Not Given>'

    def _get_arg_or_default(self, ix, default=None):
        try:
            return self.args[ix] # pylint:disable=unsubscriptable-object
        except self._NOT_GIVEN_CATCH:
            return default

    @property
    def interface(self):
        return self._get_arg_or_default(self._IX_INTERFACE)

    @property
    def target(self):
        return self._get_arg_or_default(self._IX_TARGET, self._NOT_GIVEN)

    ###
    # str
    #
    # The ``__str__`` of self is implemented by concatenating (%s), in order,
    # these properties (none of which should have leading or trailing
    # whitespace):
    #
    # - self._str_subject
    #   Begin the message, including a description of the target.
    # - self._str_description
    #   Provide a general description of the type of error, including
    #   the interface name if possible and relevant.
    # - self._str_conjunction
    #   Join the description to the details. Defaults to ": ".
    # - self._str_details
    #   Provide details about how this particular instance of the error.
    # - self._str_trailer
    #   End the message. Usually just a period.
    ###

    @property
    def _str_subject(self):
        target = self.target
        if target is self._NOT_GIVEN:
            return "An object"
        return "The object %r" % (target,)

    @property
    def _str_description(self):
        return "has failed to implement interface %s" % (
            self.interface or '<Unknown>'
        )

    _str_conjunction = ": "
    _str_details = "<unknown>"
    _str_trailer = '.'

    def __str__(self):
        return "%s %s%s%s%s" % (
            self._str_subject,
            self._str_description,
            self._str_conjunction,
            self._str_details,
            self._str_trailer
        )


class DoesNotImplement(_TargetInvalid):
    """
    DoesNotImplement(interface[, target])

    The *target* (optional) does not implement the *interface*.

    .. versionchanged:: 5.0.0
       Add the *target* argument and attribute, and change the resulting
       string value of this object accordingly.
    """

    _str_details = "Does not declaratively implement the interface"


class BrokenImplementation(_TargetInvalid):
    """
    BrokenImplementation(interface, name[, target])

    The *target* (optional) is missing the attribute *name*.

    .. versionchanged:: 5.0.0
       Add the *target* argument and attribute, and change the resulting
       string value of this object accordingly.

       The *name* can either be a simple string or a ``Attribute`` object.
    """

    _IX_NAME = _TargetInvalid._IX_INTERFACE + 1
    _IX_TARGET = _IX_NAME + 1

    @property
    def name(self):
        return self.args[1] # pylint:disable=unsubscriptable-object

    @property
    def _str_details(self):
        return "The %s attribute was not provided" % (
            repr(self.name) if isinstance(self.name, str) else self.name
        )


class BrokenMethodImplementation(_TargetInvalid):
    """
    BrokenMethodImplementation(method, message[, implementation, interface, target])

    The *target* (optional) has a *method* in *implementation* that violates
    its contract in a way described by *mess*.

    .. versionchanged:: 5.0.0
       Add the *interface* and *target* argument and attribute,
       and change the resulting string value of this object accordingly.

       The *method* can either be a simple string or a ``Method`` object.

    .. versionchanged:: 5.0.0
       If *implementation* is given, then the *message* will have the
       string "implementation" replaced with an short but informative
       representation of *implementation*.

    """

    _IX_IMPL = 2
    _IX_INTERFACE = _IX_IMPL + 1
    _IX_TARGET = _IX_INTERFACE + 1

    @property
    def method(self):
        return self.args[0] # pylint:disable=unsubscriptable-object

    @property
    def mess(self):
        return self.args[1] # pylint:disable=unsubscriptable-object

    @staticmethod
    def __implementation_str(impl):
        # It could be a callable or some arbitrary object, we don't
        # know yet.
        import inspect # Inspect is a heavy-weight dependency, lots of imports
        try:
            sig = inspect.signature
            formatsig = str
        except AttributeError:
            sig = inspect.getargspec
            f = inspect.formatargspec
            formatsig = lambda sig: f(*sig) # pylint:disable=deprecated-method

        try:
            sig = sig(impl)
        except (ValueError, TypeError):
            # Unable to introspect. Darn.
            # This could be a non-callable, or a particular builtin,
            # or a bound method that doesn't even accept 'self', e.g.,
            # ``Class.method = lambda: None; Class().method``
            return repr(impl)

        try:
            name = impl.__qualname__
        except AttributeError:
            name = impl.__name__

        return name + formatsig(sig)

    @property
    def _str_details(self):
        impl = self._get_arg_or_default(self._IX_IMPL, self._NOT_GIVEN)
        message = self.mess
        if impl is not self._NOT_GIVEN and 'implementation' in message:
            message = message.replace("implementation", '%r')
            message = message % (self.__implementation_str(impl),)

        return 'The contract of %s is violated because %s' % (
            repr(self.method) if isinstance(self.method, str) else self.method,
            message,
        )


class MultipleInvalid(_TargetInvalid):
    """
    The *target* has failed to implement the *interface* in
    multiple ways.

    The failures are described by *exceptions*, a collection of
    other `Invalid` instances.

    .. versionadded:: 5.0
    """

    _NOT_GIVEN_CATCH = ()

    def __init__(self, interface, target, exceptions):
        super(MultipleInvalid, self).__init__(interface, target, tuple(exceptions))

    @property
    def exceptions(self):
        return self.args[2] # pylint:disable=unsubscriptable-object

    @property
    def _str_details(self):
        # It would be nice to use tabs here, but that
        # is hard to represent in doctests.
        return '\n    ' + '\n    '.join(
            x._str_details.strip() if isinstance(x, _TargetInvalid) else str(x)
            for x in self.exceptions
        )

    _str_conjunction = ':' # We don't want a trailing space, messes up doctests
    _str_trailer = ''


class InvalidInterface(Exception):
    """The interface has invalid contents
    """

class BadImplements(TypeError):
    """An implementation assertion is invalid

    because it doesn't contain an interface or a sequence of valid
    implementation assertions.
    """
