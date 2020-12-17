"""
Commonly useful validators.
"""

from __future__ import absolute_import, division, print_function

import re

from ._make import _AndValidator, and_, attrib, attrs
from .exceptions import NotCallableError


__all__ = [
    "and_",
    "deep_iterable",
    "deep_mapping",
    "in_",
    "instance_of",
    "is_callable",
    "matches_re",
    "optional",
    "provides",
]


@attrs(repr=False, slots=True, hash=True)
class _InstanceOfValidator(object):
    type = attrib()

    def __call__(self, inst, attr, value):
        """
        We use a callable class to be able to change the ``__repr__``.
        """
        if not isinstance(value, self.type):
            raise TypeError(
                "'{name}' must be {type!r} (got {value!r} that is a "
                "{actual!r}).".format(
                    name=attr.name,
                    type=self.type,
                    actual=value.__class__,
                    value=value,
                ),
                attr,
                self.type,
                value,
            )

    def __repr__(self):
        return "<instance_of validator for type {type!r}>".format(
            type=self.type
        )


def instance_of(type):
    """
    A validator that raises a `TypeError` if the initializer is called
    with a wrong type for this particular attribute (checks are performed using
    `isinstance` therefore it's also valid to pass a tuple of types).

    :param type: The type to check for.
    :type type: type or tuple of types

    :raises TypeError: With a human readable error message, the attribute
        (of type `attr.Attribute`), the expected type, and the value it
        got.
    """
    return _InstanceOfValidator(type)


@attrs(repr=False, frozen=True, slots=True)
class _MatchesReValidator(object):
    regex = attrib()
    flags = attrib()
    match_func = attrib()

    def __call__(self, inst, attr, value):
        """
        We use a callable class to be able to change the ``__repr__``.
        """
        if not self.match_func(value):
            raise ValueError(
                "'{name}' must match regex {regex!r}"
                " ({value!r} doesn't)".format(
                    name=attr.name, regex=self.regex.pattern, value=value
                ),
                attr,
                self.regex,
                value,
            )

    def __repr__(self):
        return "<matches_re validator for pattern {regex!r}>".format(
            regex=self.regex
        )


def matches_re(regex, flags=0, func=None):
    r"""
    A validator that raises `ValueError` if the initializer is called
    with a string that doesn't match *regex*.

    :param str regex: a regex string to match against
    :param int flags: flags that will be passed to the underlying re function
        (default 0)
    :param callable func: which underlying `re` function to call (options
        are `re.fullmatch`, `re.search`, `re.match`, default
        is ``None`` which means either `re.fullmatch` or an emulation of
        it on Python 2). For performance reasons, they won't be used directly
        but on a pre-`re.compile`\ ed pattern.

    .. versionadded:: 19.2.0
    """
    fullmatch = getattr(re, "fullmatch", None)
    valid_funcs = (fullmatch, None, re.search, re.match)
    if func not in valid_funcs:
        raise ValueError(
            "'func' must be one of %s."
            % (
                ", ".join(
                    sorted(
                        e and e.__name__ or "None" for e in set(valid_funcs)
                    )
                ),
            )
        )

    pattern = re.compile(regex, flags)
    if func is re.match:
        match_func = pattern.match
    elif func is re.search:
        match_func = pattern.search
    else:
        if fullmatch:
            match_func = pattern.fullmatch
        else:
            pattern = re.compile(r"(?:{})\Z".format(regex), flags)
            match_func = pattern.match

    return _MatchesReValidator(pattern, flags, match_func)


@attrs(repr=False, slots=True, hash=True)
class _ProvidesValidator(object):
    interface = attrib()

    def __call__(self, inst, attr, value):
        """
        We use a callable class to be able to change the ``__repr__``.
        """
        if not self.interface.providedBy(value):
            raise TypeError(
                "'{name}' must provide {interface!r} which {value!r} "
                "doesn't.".format(
                    name=attr.name, interface=self.interface, value=value
                ),
                attr,
                self.interface,
                value,
            )

    def __repr__(self):
        return "<provides validator for interface {interface!r}>".format(
            interface=self.interface
        )


def provides(interface):
    """
    A validator that raises a `TypeError` if the initializer is called
    with an object that does not provide the requested *interface* (checks are
    performed using ``interface.providedBy(value)`` (see `zope.interface
    <https://zopeinterface.readthedocs.io/en/latest/>`_).

    :param interface: The interface to check for.
    :type interface: ``zope.interface.Interface``

    :raises TypeError: With a human readable error message, the attribute
        (of type `attr.Attribute`), the expected interface, and the
        value it got.
    """
    return _ProvidesValidator(interface)


@attrs(repr=False, slots=True, hash=True)
class _OptionalValidator(object):
    validator = attrib()

    def __call__(self, inst, attr, value):
        if value is None:
            return

        self.validator(inst, attr, value)

    def __repr__(self):
        return "<optional validator for {what} or None>".format(
            what=repr(self.validator)
        )


def optional(validator):
    """
    A validator that makes an attribute optional.  An optional attribute is one
    which can be set to ``None`` in addition to satisfying the requirements of
    the sub-validator.

    :param validator: A validator (or a list of validators) that is used for
        non-``None`` values.
    :type validator: callable or `list` of callables.

    .. versionadded:: 15.1.0
    .. versionchanged:: 17.1.0 *validator* can be a list of validators.
    """
    if isinstance(validator, list):
        return _OptionalValidator(_AndValidator(validator))
    return _OptionalValidator(validator)


@attrs(repr=False, slots=True, hash=True)
class _InValidator(object):
    options = attrib()

    def __call__(self, inst, attr, value):
        try:
            in_options = value in self.options
        except TypeError:  # e.g. `1 in "abc"`
            in_options = False

        if not in_options:
            raise ValueError(
                "'{name}' must be in {options!r} (got {value!r})".format(
                    name=attr.name, options=self.options, value=value
                )
            )

    def __repr__(self):
        return "<in_ validator with options {options!r}>".format(
            options=self.options
        )


def in_(options):
    """
    A validator that raises a `ValueError` if the initializer is called
    with a value that does not belong in the options provided.  The check is
    performed using ``value in options``.

    :param options: Allowed options.
    :type options: list, tuple, `enum.Enum`, ...

    :raises ValueError: With a human readable error message, the attribute (of
       type `attr.Attribute`), the expected options, and the value it
       got.

    .. versionadded:: 17.1.0
    """
    return _InValidator(options)


@attrs(repr=False, slots=False, hash=True)
class _IsCallableValidator(object):
    def __call__(self, inst, attr, value):
        """
        We use a callable class to be able to change the ``__repr__``.
        """
        if not callable(value):
            message = (
                "'{name}' must be callable "
                "(got {value!r} that is a {actual!r})."
            )
            raise NotCallableError(
                msg=message.format(
                    name=attr.name, value=value, actual=value.__class__
                ),
                value=value,
            )

    def __repr__(self):
        return "<is_callable validator>"


def is_callable():
    """
    A validator that raises a `attr.exceptions.NotCallableError` if the
    initializer is called with a value for this particular attribute
    that is not callable.

    .. versionadded:: 19.1.0

    :raises `attr.exceptions.NotCallableError`: With a human readable error
        message containing the attribute (`attr.Attribute`) name,
        and the value it got.
    """
    return _IsCallableValidator()


@attrs(repr=False, slots=True, hash=True)
class _DeepIterable(object):
    member_validator = attrib(validator=is_callable())
    iterable_validator = attrib(
        default=None, validator=optional(is_callable())
    )

    def __call__(self, inst, attr, value):
        """
        We use a callable class to be able to change the ``__repr__``.
        """
        if self.iterable_validator is not None:
            self.iterable_validator(inst, attr, value)

        for member in value:
            self.member_validator(inst, attr, member)

    def __repr__(self):
        iterable_identifier = (
            ""
            if self.iterable_validator is None
            else " {iterable!r}".format(iterable=self.iterable_validator)
        )
        return (
            "<deep_iterable validator for{iterable_identifier}"
            " iterables of {member!r}>"
        ).format(
            iterable_identifier=iterable_identifier,
            member=self.member_validator,
        )


def deep_iterable(member_validator, iterable_validator=None):
    """
    A validator that performs deep validation of an iterable.

    :param member_validator: Validator to apply to iterable members
    :param iterable_validator: Validator to apply to iterable itself
        (optional)

    .. versionadded:: 19.1.0

    :raises TypeError: if any sub-validators fail
    """
    return _DeepIterable(member_validator, iterable_validator)


@attrs(repr=False, slots=True, hash=True)
class _DeepMapping(object):
    key_validator = attrib(validator=is_callable())
    value_validator = attrib(validator=is_callable())
    mapping_validator = attrib(default=None, validator=optional(is_callable()))

    def __call__(self, inst, attr, value):
        """
        We use a callable class to be able to change the ``__repr__``.
        """
        if self.mapping_validator is not None:
            self.mapping_validator(inst, attr, value)

        for key in value:
            self.key_validator(inst, attr, key)
            self.value_validator(inst, attr, value[key])

    def __repr__(self):
        return (
            "<deep_mapping validator for objects mapping {key!r} to {value!r}>"
        ).format(key=self.key_validator, value=self.value_validator)


def deep_mapping(key_validator, value_validator, mapping_validator=None):
    """
    A validator that performs deep validation of a dictionary.

    :param key_validator: Validator to apply to dictionary keys
    :param value_validator: Validator to apply to dictionary values
    :param mapping_validator: Validator to apply to top-level mapping
        attribute (optional)

    .. versionadded:: 19.1.0

    :raises TypeError: if any sub-validators fail
    """
    return _DeepMapping(key_validator, value_validator, mapping_validator)
