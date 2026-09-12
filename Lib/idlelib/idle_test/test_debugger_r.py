"Test debugger_r, coverage 30%."

from idlelib import debugger_r
import types
import unittest

# Boilerplate likely to be needed for future test classes.
##from test.support import requires
##from tkinter import Tk
##class Test(unittest.TestCase):
##    @classmethod
##    def setUpClass(cls):
##        requires('gui')
##        cls.root = Tk()
##    @classmethod
##    def tearDownClass(cls):
##        cls.root.destroy()

# GUIProxy, IdbAdapter, FrameProxy, CodeProxy, DictProxy,
# GUIAdapter, IdbProxy, and 7 functions still need tests.

class IdbAdapterTest(unittest.TestCase):

    def test_dict_item_noattr(self):  # Issue 33065.

        class BinData:
            def __repr__(self):
                return self.length

        debugger_r.dicttable[0] = {'BinData': BinData()}
        idb = debugger_r.IdbAdapter(None)
        self.assertTrue(idb.dict_item(0, 'BinData'))
        debugger_r.dicttable.clear()


class FrameCodeMarshalTest(unittest.TestCase):
    "Transport a running frame's code object across the RPC via marshal."

    def test_round_trip(self):
        code = compile("def f(x):\n    return x + 1\n", "sample.py", "exec")
        # Subprocess side: the adapter marshals the registered frame's code.
        debugger_r.frametable[1] = types.SimpleNamespace(f_code=code)
        try:
            blob = debugger_r.IdbAdapter(None).frame_code_marshal(1)
        finally:
            debugger_r.frametable.clear()
        self.assertIsInstance(blob, bytes)

        # IDLE side: a FrameProxy loads the bytes back into a code object.
        class Conn:
            def remotecall(self, oid, meth, args, kwargs):
                self.call = (oid, meth, args)
                return blob
        conn = Conn()
        back = debugger_r.FrameProxy(conn, 1).code_object()
        self.assertEqual(conn.call,
                         ("idb_adapter", "frame_code_marshal", (1,)))
        self.assertEqual(back.co_qualname, code.co_qualname)
        self.assertEqual(back.co_code, code.co_code)
        nested = [c.co_qualname for c in back.co_consts if hasattr(c, "co_code")]
        self.assertIn("f", nested)


if __name__ == '__main__':
    unittest.main(verbosity=2)
