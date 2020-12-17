##############################################################################
#
# Copyright (c) 2003 Zope Foundation and Contributors.
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
"""
Compute a resolution order for an object and its bases.

.. versionchanged:: 5.0
   The resolution order is now based on the same C3 order that Python
   uses for classes. In complex instances of multiple inheritance, this
   may result in a different ordering.

   In older versions, the ordering wasn't required to be C3 compliant,
   and for backwards compatibility, it still isn't. If the ordering
   isn't C3 compliant (if it is *inconsistent*), zope.interface will
   make a best guess to try to produce a reasonable resolution order.
   Still (just as before), the results in such cases may be
   surprising.

.. rubric:: Environment Variables

Due to the change in 5.0, certain environment variables can be used to control errors
and warnings about inconsistent resolution orders. They are listed in priority order, with
variables at the bottom generally overriding variables above them.

ZOPE_INTERFACE_WARN_BAD_IRO
    If this is set to "1", then if there is at least one inconsistent resolution
    order discovered, a warning (:class:`InconsistentResolutionOrderWarning`) will
    be issued. Use the usual warning mechanisms to control this behaviour. The warning
    text will contain additional information on debugging.
ZOPE_INTERFACE_TRACK_BAD_IRO
    If this is set to "1", then zope.interface will log information about each
    inconsistent resolution order discovered, and keep those details in memory in this module
    for later inspection.
ZOPE_INTERFACE_STRICT_IRO
    If this is set to "1", any attempt to use :func:`ro` that would produce a non-C3
    ordering will fail by raising :class:`InconsistentResolutionOrderError`.

There are two environment variables that are independent.

ZOPE_INTERFACE_LOG_CHANGED_IRO
    If this is set to "1", then if the C3 resolution order is different from
    the legacy resolution order for any given object, a message explaining the differences
    will be logged. This is intended to be used for debugging complicated IROs.
ZOPE_INTERFACE_USE_LEGACY_IRO
    If this is set to "1", then the C3 resolution order will *not* be used. The
    legacy IRO will be used instead. This is a temporary measure and will be removed in the
    future. It is intended to help during the transition.
    It implies ``ZOPE_INTERFACE_LOG_CHANGED_IRO``.
"""
from __future__ import print_function
__docformat__ = 'restructuredtext'

__all__ = [
    'ro',
    'InconsistentResolutionOrderError',
    'InconsistentResolutionOrderWarning',
]

__logger = None

def _logger():
    global __logger # pylint:disable=global-statement
    if __logger is None:
        import logging
        __logger = logging.getLogger(__name__)
    return __logger

def _legacy_mergeOrderings(orderings):
    """Merge multiple orderings so that within-ordering order is preserved

    Orderings are constrained in such a way that if an object appears
    in two or more orderings, then the suffix that begins with the
    object must be in both orderings.

    For example:

    >>> _mergeOrderings([
    ... ['x', 'y', 'z'],
    ... ['q', 'z'],
    ... [1, 3, 5],
    ... ['z']
    ... ])
    ['x', 'y', 'q', 1, 3, 5, 'z']

    """

    seen = set()
    result = []
    for ordering in reversed(orderings):
        for o in reversed(ordering):
            if o not in seen:
                seen.add(o)
                result.insert(0, o)

    return result

def _legacy_flatten(begin):
    result = [begin]
    i = 0
    for ob in iter(result):
        i += 1
        # The recursive calls can be avoided by inserting the base classes
        # into the dynamically growing list directly after the currently
        # considered object;  the iterator makes sure this will keep working
        # in the future, since it cannot rely on the length of the list
        # by definition.
        result[i:i] = ob.__bases__
    return result

def _legacy_ro(ob):
    return _legacy_mergeOrderings([_legacy_flatten(ob)])

###
# Compare base objects using identity, not equality. This matches what
# the CPython MRO algorithm does, and is *much* faster to boot: that,
# plus some other small tweaks makes the difference between 25s and 6s
# in loading 446 plone/zope interface.py modules (1925 InterfaceClass,
# 1200 Implements, 1100 ClassProvides objects)
###


class InconsistentResolutionOrderWarning(PendingDeprecationWarning):
    """
    The warning issued when an invalid IRO is requested.
    """

class InconsistentResolutionOrderError(TypeError):
    """
    The error raised when an invalid IRO is requested in strict mode.
    """

    def __init__(self, c3, base_tree_remaining):
        self.C = c3.leaf
        base_tree = c3.base_tree
        self.base_ros = {
            base: base_tree[i + 1]
            for i, base in enumerate(self.C.__bases__)
        }
        # Unfortunately, this doesn't necessarily directly match
        # up to any transformation on C.__bases__, because
        # if any were fully used up, they were removed already.
        self.base_tree_remaining = base_tree_remaining

        TypeError.__init__(self)

    def __str__(self):
        import pprint
        return "%s: For object %r.\nBase ROs:\n%s\nConflict Location:\n%s" % (
            self.__class__.__name__,
            self.C,
            pprint.pformat(self.base_ros),
            pprint.pformat(self.base_tree_remaining),
        )


class _NamedBool(int): # cannot actually inherit bool

    def __new__(cls, val, name):
        inst = super(cls, _NamedBool).__new__(cls, val)
        inst.__name__ = name
        return inst


class _ClassBoolFromEnv(object):
    """
    Non-data descriptor that reads a transformed environment variable
    as a boolean, and caches the result in the class.
    """

    def __get__(self, inst, klass):
        import os
        for cls in klass.__mro__:
            my_name = None
            for k in dir(klass):
                if k in cls.__dict__ and cls.__dict__[k] is self:
                    my_name = k
                    break
            if my_name is not None:
                break
        else: # pragma: no cover
            raise RuntimeError("Unable to find self")

        env_name = 'ZOPE_INTERFACE_' + my_name
        val = os.environ.get(env_name, '') == '1'
        val = _NamedBool(val, my_name)
        setattr(klass, my_name, val)
        setattr(klass, 'ORIG_' + my_name, self)
        return val


class _StaticMRO(object):
    # A previously resolved MRO, supplied by the caller.
    # Used in place of calculating it.

    had_inconsistency = None # We don't know...

    def __init__(self, C, mro):
        self.leaf = C
        self.__mro = tuple(mro)

    def mro(self):
        return list(self.__mro)


class C3(object):
    # Holds the shared state during computation of an MRO.

    @staticmethod
    def resolver(C, strict, base_mros):
        strict = strict if strict is not None else C3.STRICT_IRO
        factory = C3
        if strict:
            factory = _StrictC3
        elif C3.TRACK_BAD_IRO:
            factory = _TrackingC3

        memo = {}
        base_mros = base_mros or {}
        for base, mro in base_mros.items():
            assert base in C.__bases__
            memo[base] = _StaticMRO(base, mro)

        return factory(C, memo)

    __mro = None
    __legacy_ro = None
    direct_inconsistency = False

    def __init__(self, C, memo):
        self.leaf = C
        self.memo = memo
        kind = self.__class__

        base_resolvers = []
        for base in C.__bases__:
            if base not in memo:
                resolver = kind(base, memo)
                memo[base] = resolver
            base_resolvers.append(memo[base])

        self.base_tree = [
            [C]
        ] + [
            memo[base].mro() for base in C.__bases__
        ] + [
            list(C.__bases__)
        ]

        self.bases_had_inconsistency = any(base.had_inconsistency for base in base_resolvers)

        if len(C.__bases__) == 1:
            self.__mro = [C] + memo[C.__bases__[0]].mro()

    @property
    def had_inconsistency(self):
        return self.direct_inconsistency or self.bases_had_inconsistency

    @property
    def legacy_ro(self):
        if self.__legacy_ro is None:
            self.__legacy_ro = tuple(_legacy_ro(self.leaf))
        return list(self.__legacy_ro)

    TRACK_BAD_IRO = _ClassBoolFromEnv()
    STRICT_IRO = _ClassBoolFromEnv()
    WARN_BAD_IRO = _ClassBoolFromEnv()
    LOG_CHANGED_IRO = _ClassBoolFromEnv()
    USE_LEGACY_IRO = _ClassBoolFromEnv()
    BAD_IROS = ()

    def _warn_iro(self):
        if not self.WARN_BAD_IRO:
            # For the initial release, one must opt-in to see the warning.
            # In the future (2021?) seeing at least the first warning will
            # be the default
            return
        import warnings
        warnings.warn(
            "An inconsistent resolution order is being requested. "
            "(Interfaces should follow the Python class rules known as C3.) "
            "For backwards compatibility, zope.interface will allow this, "
            "making the best guess it can to produce as meaningful an order as possible. "
            "In the future this might be an error. Set the warning filter to error, or set "
            "the environment variable 'ZOPE_INTERFACE_TRACK_BAD_IRO' to '1' and examine "
            "ro.C3.BAD_IROS to debug, or set 'ZOPE_INTERFACE_STRICT_IRO' to raise exceptions.",
            InconsistentResolutionOrderWarning,
        )

    @staticmethod
    def _can_choose_base(base, base_tree_remaining):
        # From C3:
        # nothead = [s for s in nonemptyseqs if cand in s[1:]]
        for bases in base_tree_remaining:
            if not bases or bases[0] is base:
                continue

            for b in bases:
                if b is base:
                    return False
        return True

    @staticmethod
    def _nonempty_bases_ignoring(base_tree, ignoring):
        return list(filter(None, [
            [b for b in bases if b is not ignoring]
            for bases
            in base_tree
        ]))

    def _choose_next_base(self, base_tree_remaining):
        """
        Return the next base.

        The return value will either fit the C3 constraints or be our best
        guess about what to do. If we cannot guess, this may raise an exception.
        """
        base = self._find_next_C3_base(base_tree_remaining)
        if base is not None:
            return base
        return self._guess_next_base(base_tree_remaining)

    def _find_next_C3_base(self, base_tree_remaining):
        """
        Return the next base that fits the constraints, or ``None`` if there isn't one.
        """
        for bases in base_tree_remaining:
            base = bases[0]
            if self._can_choose_base(base, base_tree_remaining):
                return base
        return None

    class _UseLegacyRO(Exception):
        pass

    def _guess_next_base(self, base_tree_remaining):
        # Narf. We may have an inconsistent order (we won't know for
        # sure until we check all the bases). Python cannot create
        # classes like this:
        #
        # class B1:
        #   pass
        # class B2(B1):
        #   pass
        # class C(B1, B2): # -> TypeError; this is like saying C(B1, B2, B1).
        #  pass
        #
        # However, older versions of zope.interface were fine with this order.
        # A good example is ``providedBy(IOError())``. Because of the way
        # ``classImplements`` works, it winds up with ``__bases__`` ==
        # ``[IEnvironmentError, IIOError, IOSError, <implementedBy Exception>]``
        # (on Python 3). But ``IEnvironmentError`` is a base of both ``IIOError``
        # and ``IOSError``. Previously, we would get a resolution order of
        # ``[IIOError, IOSError, IEnvironmentError, IStandardError, IException, Interface]``
        # but the standard Python algorithm would forbid creating that order entirely.

        # Unlike Python's MRO, we attempt to resolve the issue. A few
        # heuristics have been tried. One was:
        #
        # Strip off the first (highest priority) base of each direct
        # base one at a time and seeing if we can come to an agreement
        # with the other bases. (We're trying for a partial ordering
        # here.) This often resolves cases (such as the IOSError case
        # above), and frequently produces the same ordering as the
        # legacy MRO did. If we looked at all the highest priority
        # bases and couldn't find any partial ordering, then we strip
        # them *all* out and begin the C3 step again. We take care not
        # to promote a common root over all others.
        #
        # If we only did the first part, stripped off the first
        # element of the first item, we could resolve simple cases.
        # But it tended to fail badly. If we did the whole thing, it
        # could be extremely painful from a performance perspective
        # for deep/wide things like Zope's OFS.SimpleItem.Item. Plus,
        # anytime you get ExtensionClass.Base into the mix, you're
        # likely to wind up in trouble, because it messes with the MRO
        # of classes. Sigh.
        #
        # So now, we fall back to the old linearization (fast to compute).
        self._warn_iro()
        self.direct_inconsistency = InconsistentResolutionOrderError(self, base_tree_remaining)
        raise self._UseLegacyRO

    def _merge(self):
        # Returns a merged *list*.
        result = self.__mro = []
        base_tree_remaining = self.base_tree
        base = None
        while 1:
            # Take last picked base out of the base tree wherever it is.
            # This differs slightly from the standard Python MRO and is needed
            # because we have no other step that prevents duplicates
            # from coming in (e.g., in the inconsistent fallback path)
            base_tree_remaining = self._nonempty_bases_ignoring(base_tree_remaining, base)

            if not base_tree_remaining:
                return result
            try:
                base = self._choose_next_base(base_tree_remaining)
            except self._UseLegacyRO:
                self.__mro = self.legacy_ro
                return self.legacy_ro

            result.append(base)

    def mro(self):
        if self.__mro is None:
            self.__mro = tuple(self._merge())
        return list(self.__mro)


class _StrictC3(C3):
    __slots__ = ()
    def _guess_next_base(self, base_tree_remaining):
        raise InconsistentResolutionOrderError(self, base_tree_remaining)


class _TrackingC3(C3):
    __slots__ = ()
    def _guess_next_base(self, base_tree_remaining):
        import traceback
        bad_iros = C3.BAD_IROS
        if self.leaf not in bad_iros:
            if bad_iros == ():
                import weakref
                # This is a race condition, but it doesn't matter much.
                bad_iros = C3.BAD_IROS = weakref.WeakKeyDictionary()
            bad_iros[self.leaf] = t = (
                InconsistentResolutionOrderError(self, base_tree_remaining),
                traceback.format_stack()
            )
            _logger().warning("Tracking inconsistent IRO: %s", t[0])
        return C3._guess_next_base(self, base_tree_remaining)


class _ROComparison(object):
    # Exists to compute and print a pretty string comparison
    # for differing ROs.
    # Since we're used in a logging context, and may actually never be printed,
    # this is a class so we can defer computing the diff until asked.

    # Components we use to build up the comparison report
    class Item(object):
        prefix = '  '
        def __init__(self, item):
            self.item = item
        def __str__(self):
            return "%s%s" % (
                self.prefix,
                self.item,
            )

    class Deleted(Item):
        prefix = '- '

    class Inserted(Item):
        prefix = '+ '

    Empty = str

    class ReplacedBy(object): # pragma: no cover
        prefix = '- '
        suffix = ''
        def __init__(self, chunk, total_count):
            self.chunk = chunk
            self.total_count = total_count

        def __iter__(self):
            lines = [
                self.prefix + str(item) + self.suffix
                for item in self.chunk
            ]
            while len(lines) < self.total_count:
                lines.append('')

            return iter(lines)

    class Replacing(ReplacedBy):
        prefix = "+ "
        suffix = ''


    _c3_report = None
    _legacy_report = None

    def __init__(self, c3, c3_ro, legacy_ro):
        self.c3 = c3
        self.c3_ro = c3_ro
        self.legacy_ro = legacy_ro

    def __move(self, from_, to_, chunk, operation):
        for x in chunk:
            to_.append(operation(x))
            from_.append(self.Empty())

    def _generate_report(self):
        if self._c3_report is None:
            import difflib
            # The opcodes we get describe how to turn 'a' into 'b'. So
            # the old one (legacy) needs to be first ('a')
            matcher = difflib.SequenceMatcher(None, self.legacy_ro, self.c3_ro)
            # The reports are equal length sequences. We're going for a
            # side-by-side diff.
            self._c3_report = c3_report = []
            self._legacy_report = legacy_report = []
            for opcode, leg1, leg2, c31, c32 in matcher.get_opcodes():
                c3_chunk = self.c3_ro[c31:c32]
                legacy_chunk = self.legacy_ro[leg1:leg2]

                if opcode == 'equal':
                    # Guaranteed same length
                    c3_report.extend((self.Item(x) for x in c3_chunk))
                    legacy_report.extend(self.Item(x) for x in legacy_chunk)
                if opcode == 'delete':
                    # Guaranteed same length
                    assert not c3_chunk
                    self.__move(c3_report, legacy_report, legacy_chunk, self.Deleted)
                if opcode == 'insert':
                    # Guaranteed same length
                    assert not legacy_chunk
                    self.__move(legacy_report, c3_report, c3_chunk, self.Inserted)
                if opcode == 'replace': # pragma: no cover (How do you make it output this?)
                    # Either side could be longer.
                    chunk_size = max(len(c3_chunk), len(legacy_chunk))
                    c3_report.extend(self.Replacing(c3_chunk, chunk_size))
                    legacy_report.extend(self.ReplacedBy(legacy_chunk, chunk_size))

        return self._c3_report, self._legacy_report

    @property
    def _inconsistent_label(self):
        inconsistent = []
        if self.c3.direct_inconsistency:
            inconsistent.append('direct')
        if self.c3.bases_had_inconsistency:
            inconsistent.append('bases')
        return '+'.join(inconsistent) if inconsistent else 'no'

    def __str__(self):
        c3_report, legacy_report = self._generate_report()
        assert len(c3_report) == len(legacy_report)

        left_lines = [str(x) for x in legacy_report]
        right_lines = [str(x) for x in c3_report]

        # We have the same number of lines in the report; this is not
        # necessarily the same as the number of items in either RO.
        assert len(left_lines) == len(right_lines)

        padding = ' ' * 2
        max_left = max(len(x) for x in left_lines)
        max_right = max(len(x) for x in right_lines)

        left_title = 'Legacy RO (len=%s)' % (len(self.legacy_ro),)

        right_title = 'C3 RO (len=%s; inconsistent=%s)' % (
            len(self.c3_ro),
            self._inconsistent_label,
        )
        lines = [
            (padding + left_title.ljust(max_left) + padding + right_title.ljust(max_right)),
            padding + '=' * (max_left + len(padding) + max_right)
        ]
        lines += [
            padding + left.ljust(max_left) + padding + right
            for left, right in zip(left_lines, right_lines)
        ]

        return '\n'.join(lines)


# Set to `Interface` once it is defined. This is used to
# avoid logging false positives about changed ROs.
_ROOT = None

def ro(C, strict=None, base_mros=None, log_changed_ro=None, use_legacy_ro=None):
    """
    ro(C) -> list

    Compute the precedence list (mro) according to C3.

    :return: A fresh `list` object.

    .. versionchanged:: 5.0.0
       Add the *strict*, *log_changed_ro* and *use_legacy_ro*
       keyword arguments. These are provisional and likely to be
       removed in the future. They are most useful for testing.
    """
    # The ``base_mros`` argument is for internal optimization and
    # not documented.
    resolver = C3.resolver(C, strict, base_mros)
    mro = resolver.mro()

    log_changed = log_changed_ro if log_changed_ro is not None else resolver.LOG_CHANGED_IRO
    use_legacy = use_legacy_ro if use_legacy_ro is not None else resolver.USE_LEGACY_IRO

    if log_changed or use_legacy:
        legacy_ro = resolver.legacy_ro
        assert isinstance(legacy_ro, list)
        assert isinstance(mro, list)
        changed = legacy_ro != mro
        if changed:
            # Did only Interface move? The fix for issue #8 made that
            # somewhat common. It's almost certainly not a problem, though,
            # so allow ignoring it.
            legacy_without_root = [x for x in legacy_ro if x is not _ROOT]
            mro_without_root = [x for x in mro if x is not _ROOT]
            changed = legacy_without_root != mro_without_root

        if changed:
            comparison = _ROComparison(resolver, mro, legacy_ro)
            _logger().warning(
                "Object %r has different legacy and C3 MROs:\n%s",
                C, comparison
            )
        if resolver.had_inconsistency and legacy_ro == mro:
            comparison = _ROComparison(resolver, mro, legacy_ro)
            _logger().warning(
                "Object %r had inconsistent IRO and used the legacy RO:\n%s"
                "\nInconsistency entered at:\n%s",
                C, comparison, resolver.direct_inconsistency
            )
        if use_legacy:
            return legacy_ro

    return mro


def is_consistent(C):
    """
    Check if the resolution order for *C*, as computed by :func:`ro`, is consistent
    according to C3.
    """
    return not C3.resolver(C, False, None).had_inconsistency
