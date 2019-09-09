"Test posixcap functions"

from test import support

# Skip these tests if there is no posixcap module.
posix = support.import_module('posixcap')

import os
import unittest


class PosixCapTester(unittest.TestCase):

    def test_cap(self):
        if hasattr(posix, 'cap_init'):
            a = posix.cap_init()
            b = posix.cap_init()
            self.assert_(a)
            self.assert_(a.cap_get_proc())
            self.assert_(a.cap_to_text())
            self.assert_(b.cap_from_text(a.cap_to_text()))
            self.assert_(a.cap_copy_ext())
            a.cap_set_proc()
            self.assert_(a.cap_clear())
            self.assert_(a.cap_get_flag(posix.CAP_CHOWN, posix.CAP_EFFECTIVE) == posix.CAP_CLEAR)
            self.assert_(a.cap_set_flag(posix.CAP_CHOWN, posix.CAP_EFFECTIVE, posix.CAP_SET))
            self.assert_(a.cap_get_flag(posix.CAP_CHOWN, posix.CAP_EFFECTIVE) == posix.CAP_SET)
            self.assert_(a.cap_set_flag(posix.CAP_CHOWN, posix.CAP_EFFECTIVE, posix.CAP_CLEAR))
            self.assert_(a.cap_get_flag(posix.CAP_CHOWN, posix.CAP_EFFECTIVE) == posix.CAP_CLEAR)
            self.assert_(a.cap_get_flag(posix.CAP_CHOWN, posix.CAP_INHERITABLE) == posix.CAP_CLEAR)
            self.assert_(a.cap_set_flag(posix.CAP_CHOWN, posix.CAP_INHERITABLE, posix.CAP_SET))
            self.assert_(a.cap_get_flag(posix.CAP_CHOWN, posix.CAP_INHERITABLE) == posix.CAP_SET)
            self.assert_(a.cap_set_flag(posix.CAP_CHOWN, posix.CAP_INHERITABLE, posix.CAP_CLEAR))
            self.assert_(a.cap_get_flag(posix.CAP_CHOWN, posix.CAP_INHERITABLE) == posix.CAP_CLEAR)
            self.assert_(a.cap_get_flag(posix.CAP_NET_RAW, posix.CAP_EFFECTIVE) == posix.CAP_CLEAR)
            self.assert_(a.cap_set_flag(posix.CAP_NET_RAW, posix.CAP_EFFECTIVE, posix.CAP_SET))
            self.assert_(a.cap_get_flag(posix.CAP_NET_RAW, posix.CAP_EFFECTIVE) == posix.CAP_SET)
            self.assert_(a.cap_set_flag(posix.CAP_NET_RAW, posix.CAP_EFFECTIVE, posix.CAP_CLEAR))
            self.assert_(a.cap_get_flag(posix.CAP_NET_RAW, posix.CAP_EFFECTIVE) == posix.CAP_CLEAR)

    def test_cap_copy_int(self):
        if hasattr(posix, 'cap_init'):
            a = posix.cap_init()
            b = posix.cap_init()
            self.assert_(b.cap_copy_int(a.cap_copy_ext()))

def test_main():
    try:
        support.run_unittest(
            PosixCapTester,
        )
    finally:
        support.reap_children()

if __name__ == '__main__':
    test_main()
