import os
import sys
import unittest
from test import test_support
from Tkinter import Tcl, TclError

test_support.requires('gui')

class TkLoadTest(unittest.TestCase):

    @unittest.skipIf('DISPLAY' not in os.environ, 'No $DISPLAY set.')
    def testLoadTk(self):
        tcl = Tcl()
        self.assertRaises(TclError,tcl.winfo_geometry)
        tcl.loadtk()
        self.assertEqual('1x1+0+0', tcl.winfo_geometry())
        tcl.destroy()

    def testLoadTkFailure(self):
        old_display = None
        if sys.platform.startswith(('win', 'darwin', 'cygwin')):
            # no failure possible on windows?

            # XXX Maybe on tk older than 8.4.13 it would be possible,
            # see tkinter.h.
            return
        with test_support.EnvironmentVarGuard() as env:
            if 'DISPLAY' in os.environ:
                del env['DISPLAY']
                # on some platforms, deleting environment variables
                # doesn't actually carry through to the process level
                # because they don't support unsetenv
                # If that's the case, abort.
                display = os.popen('echo $DISPLAY').read().strip()
                if display:
                    return

            tcl = Tcl()
            self.assertRaises(TclError, tcl.winfo_geometry)
            self.assertRaises(TclError, tcl.loadtk)

tests_gui = (TkLoadTest, )

if __name__ == "__main__":
    test_support.run_unittest(*tests_gui)
