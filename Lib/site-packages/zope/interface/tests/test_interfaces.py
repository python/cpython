import unittest


class _ConformsToIObjectEvent(object):

    def _makeOne(self, target=None):
        if target is None:
            target = object()
        return self._getTargetClass()(target)

    def test_class_conforms_to_IObjectEvent(self):
        from zope.interface.interfaces import IObjectEvent
        from zope.interface.verify import verifyClass
        verifyClass(IObjectEvent, self._getTargetClass())

    def test_instance_conforms_to_IObjectEvent(self):
        from zope.interface.interfaces import IObjectEvent
        from zope.interface.verify import verifyObject
        verifyObject(IObjectEvent, self._makeOne())


class _ConformsToIRegistrationEvent(_ConformsToIObjectEvent):

    def test_class_conforms_to_IRegistrationEvent(self):
        from zope.interface.interfaces import IRegistrationEvent
        from zope.interface.verify import verifyClass
        verifyClass(IRegistrationEvent, self._getTargetClass())

    def test_instance_conforms_to_IRegistrationEvent(self):
        from zope.interface.interfaces import IRegistrationEvent
        from zope.interface.verify import verifyObject
        verifyObject(IRegistrationEvent, self._makeOne())


class ObjectEventTests(unittest.TestCase, _ConformsToIObjectEvent):

    def _getTargetClass(self):
        from zope.interface.interfaces import ObjectEvent
        return ObjectEvent

    def test_ctor(self):
        target = object()
        event = self._makeOne(target)
        self.assertTrue(event.object is target)


class RegistrationEventTests(unittest.TestCase,
                             _ConformsToIRegistrationEvent):

    def _getTargetClass(self):
        from zope.interface.interfaces import RegistrationEvent
        return RegistrationEvent

    def test___repr__(self):
        target = object()
        event = self._makeOne(target)
        r = repr(event)
        self.assertEqual(r.splitlines(),
                         ['RegistrationEvent event:', repr(target)])


class RegisteredTests(unittest.TestCase,
                      _ConformsToIRegistrationEvent):

    def _getTargetClass(self):
        from zope.interface.interfaces import Registered
        return Registered

    def test_class_conforms_to_IRegistered(self):
        from zope.interface.interfaces import IRegistered
        from zope.interface.verify import verifyClass
        verifyClass(IRegistered, self._getTargetClass())

    def test_instance_conforms_to_IRegistered(self):
        from zope.interface.interfaces import IRegistered
        from zope.interface.verify import verifyObject
        verifyObject(IRegistered, self._makeOne())


class UnregisteredTests(unittest.TestCase,
                        _ConformsToIRegistrationEvent):

    def _getTargetClass(self):
        from zope.interface.interfaces import Unregistered
        return Unregistered

    def test_class_conforms_to_IUnregistered(self):
        from zope.interface.interfaces import IUnregistered
        from zope.interface.verify import verifyClass
        verifyClass(IUnregistered, self._getTargetClass())

    def test_instance_conforms_to_IUnregistered(self):
        from zope.interface.interfaces import IUnregistered
        from zope.interface.verify import verifyObject
        verifyObject(IUnregistered, self._makeOne())


class InterfaceClassTests(unittest.TestCase):

    def _getTargetClass(self):
        from zope.interface.interface import InterfaceClass
        return InterfaceClass

    def _getTargetInterface(self):
        from zope.interface.interfaces import IInterface
        return IInterface

    def _makeOne(self):
        from zope.interface.interface import Interface
        return Interface

    def test_class_conforms(self):
        from zope.interface.verify import verifyClass
        verifyClass(self._getTargetInterface(), self._getTargetClass())

    def test_instance_conforms(self):
        from zope.interface.verify import verifyObject
        verifyObject(self._getTargetInterface(), self._makeOne())

    def test_instance_consistent__iro__(self):
        from zope.interface import ro
        self.assertTrue(ro.is_consistent(self._getTargetInterface()))

    def test_class_consistent__iro__(self):
        from zope.interface import ro
        from zope.interface import implementedBy

        self.assertTrue(ro.is_consistent(implementedBy(self._getTargetClass())))
