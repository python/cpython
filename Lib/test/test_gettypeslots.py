import imp
import unittest


class TestGetTypeSlots(unittest.TestCase):

    def test_gettypeslots(self):
        module = '_testgettypeslots'
        fileobj, pathname, description = imp.find_module(module)
        fileobj.close()
        mod = imp.load_dynamic(module, pathname)
        print(mod.__name__)
