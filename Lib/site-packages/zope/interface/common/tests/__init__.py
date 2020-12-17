##############################################################################
# Copyright (c) 2020 Zope Foundation and Contributors.
# All Rights Reserved.
#
# This software is subject to the provisions of the Zope Public License,
# Version 2.1 (ZPL).  A copy of the ZPL should accompany this distribution.
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY AND ALL EXPRESS OR IMPLIED
# WARRANTIES ARE DISCLAIMED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF TITLE, MERCHANTABILITY, AGAINST INFRINGEMENT, AND FITNESS
# FOR A PARTICULAR PURPOSE.
##############################################################################

import unittest

from zope.interface.verify import verifyClass
from zope.interface.verify import verifyObject

from zope.interface.common import ABCInterface
from zope.interface.common import ABCInterfaceClass


def iter_abc_interfaces(predicate=lambda iface: True):
    # Iterate ``(iface, classes)``, where ``iface`` is a descendent of
    # the ABCInterfaceClass passing the *predicate* and ``classes`` is
    # an iterable of classes registered to conform to that interface.
    #
    # Note that some builtin classes are registered for two distinct
    # parts of the ABC/interface tree. For example, bytearray is both ByteString
    # and MutableSequence.
    seen = set()
    stack = list(ABCInterface.dependents) # subclasses, but also implementedBy objects
    while stack:
        iface = stack.pop(0)
        if iface in seen or not isinstance(iface, ABCInterfaceClass):
            continue
        seen.add(iface)
        stack.extend(list(iface.dependents))
        if not predicate(iface):
            continue

        registered = set(iface.getRegisteredConformers())
        registered -= set(iface._ABCInterfaceClass__ignored_classes)
        if registered:
            yield iface, registered


def add_abc_interface_tests(cls, module):
    def predicate(iface):
        return iface.__module__ == module
    add_verify_tests(cls, iter_abc_interfaces(predicate))


def add_verify_tests(cls, iface_classes_iter):
    cls.maxDiff = None
    for iface, registered_classes in iface_classes_iter:
        for stdlib_class in registered_classes:
            def test(self, stdlib_class=stdlib_class, iface=iface):
                if stdlib_class in self.UNVERIFIABLE or stdlib_class.__name__ in self.UNVERIFIABLE:
                    self.skipTest("Unable to verify %s" % stdlib_class)

                self.assertTrue(self.verify(iface, stdlib_class))

            suffix = "%s_%s_%s" % (
                stdlib_class.__name__,
                iface.__module__.replace('.', '_'),
                iface.__name__
            )
            name = 'test_auto_' + suffix
            test.__name__ = name
            assert not hasattr(cls, name), (name, list(cls.__dict__))
            setattr(cls, name, test)

            def test_ro(self, stdlib_class=stdlib_class, iface=iface):
                from zope.interface import ro
                from zope.interface import implementedBy
                from zope.interface import Interface
                self.assertEqual(
                    tuple(ro.ro(iface, strict=True)),
                    iface.__sro__)
                implements = implementedBy(stdlib_class)
                sro = implements.__sro__
                self.assertIs(sro[-1], Interface)

                # Check that we got the strict C3 resolution order, unless we
                # know we cannot. Note that 'Interface' is virtual base that doesn't
                # necessarily appear at the same place in the calculated SRO as in the
                # final SRO.
                strict = stdlib_class not in self.NON_STRICT_RO
                isro = ro.ro(implements, strict=strict)
                isro.remove(Interface)
                isro.append(Interface)

                self.assertEqual(tuple(isro), sro)

            name = 'test_auto_ro_' + suffix
            test_ro.__name__ = name
            assert not hasattr(cls, name)
            setattr(cls, name, test_ro)

class VerifyClassMixin(unittest.TestCase):
    verifier = staticmethod(verifyClass)
    UNVERIFIABLE = ()
    NON_STRICT_RO = ()

    def _adjust_object_before_verify(self, iface, x):
        return x

    def verify(self, iface, klass, **kwargs):
        return self.verifier(iface,
                             self._adjust_object_before_verify(iface, klass),
                             **kwargs)


class VerifyObjectMixin(VerifyClassMixin):
    verifier = staticmethod(verifyObject)
    CONSTRUCTORS = {
    }

    def _adjust_object_before_verify(self, iface, x):
        constructor = self.CONSTRUCTORS.get(x)
        if not constructor:
            constructor = self.CONSTRUCTORS.get(iface)
        if not constructor:
            constructor = self.CONSTRUCTORS.get(x.__name__)
        if not constructor:
            constructor = x
        if constructor is unittest.SkipTest:
            self.skipTest("Cannot create " + str(x))

        result = constructor()
        if hasattr(result, 'close'):
            self.addCleanup(result.close)
        return result
