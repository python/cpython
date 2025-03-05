import unittest
import tkinter
import _tkinter
from test.support import requires, run_unittest
from tkinter.test.support import AbstractTkTest

requires('gui')

class EventTest(AbstractTkTest, unittest.TestCase):

    called = False
    test_data = None

    # pump_events function 'borrowed' from ivan_pozdeev on stackoverflow
    def pump_events(self):
        while self.root.dooneevent(_tkinter.ALL_EVENTS | _tkinter.DONT_WAIT):
            pass

    def test_virtual_event(self):
        def receive(e):
            EventTest.called = True
            self.assertIsInstance(e, tkinter.Event)
            self.assertEqual(e.type, tkinter.EventType.VirtualEvent)
            self.assertIsInstance(e.user_data, str)
            if EventTest.test_data is not None:
                self.assertEqual(e.user_data, EventTest.test_data)
            else:
                self.assertEqual(e.user_data, '')
        self.pump_events()
        EventTest.called = False
        b = self.root.bind('<<TEST>>', lambda e:receive(e))
        self.root.event_generate('<<TEST>>')
        self.pump_events()
        self.assertTrue(EventTest.called)
        EventTest.called = False
        EventTest.test_data = 'test'
        self.root.event_generate('<<TEST>>', data='test')
        self.pump_events()
        self.assertTrue(EventTest.called)
        self.root.unbind('<<TEST>>', b)
        EventTest.called = False
        self.root.event_generate('<<TEST>>')
        self.pump_events()
        self.assertFalse(EventTest.called)

tests_gui = (EventTest, )

if __name__ == "__main__":
    run_unittest(*tests_gui)
