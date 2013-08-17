# A lot of failures in these tests on Mac OS X.
# Byte order related?

import unittest
from ctypes import *

import _ctypes_test

class CFunctions(unittest.TestCase):
    _dll = CDLL(_ctypes_test.__file__)

    def S(self):
        return c_longlong.in_dll(self._dll, "last_tf_arg_s").value
    def U(self):
        return c_ulonglong.in_dll(self._dll, "last_tf_arg_u").value

    def test_byte(self):
        self._dll.tf_b.restype = c_byte
        self._dll.tf_b.argtypes = (c_byte,)
        self.assertEqual(self._dll.tf_b(-126), -42)
        self.assertEqual(self.S(), -126)

    def test_byte_plus(self):
        self._dll.tf_bb.restype = c_byte
        self._dll.tf_bb.argtypes = (c_byte, c_byte)
        self.assertEqual(self._dll.tf_bb(0, -126), -42)
        self.assertEqual(self.S(), -126)

    def test_ubyte(self):
        self._dll.tf_B.restype = c_ubyte
        self._dll.tf_B.argtypes = (c_ubyte,)
        self.assertEqual(self._dll.tf_B(255), 85)
        self.assertEqual(self.U(), 255)

    def test_ubyte_plus(self):
        self._dll.tf_bB.restype = c_ubyte
        self._dll.tf_bB.argtypes = (c_byte, c_ubyte)
        self.assertEqual(self._dll.tf_bB(0, 255), 85)
        self.assertEqual(self.U(), 255)

    def test_short(self):
        self._dll.tf_h.restype = c_short
        self._dll.tf_h.argtypes = (c_short,)
        self.assertEqual(self._dll.tf_h(-32766), -10922)
        self.assertEqual(self.S(), -32766)

    def test_short_plus(self):
        self._dll.tf_bh.restype = c_short
        self._dll.tf_bh.argtypes = (c_byte, c_short)
        self.assertEqual(self._dll.tf_bh(0, -32766), -10922)
        self.assertEqual(self.S(), -32766)

    def test_ushort(self):
        self._dll.tf_H.restype = c_ushort
        self._dll.tf_H.argtypes = (c_ushort,)
        self.assertEqual(self._dll.tf_H(65535), 21845)
        self.assertEqual(self.U(), 65535)

    def test_ushort_plus(self):
        self._dll.tf_bH.restype = c_ushort
        self._dll.tf_bH.argtypes = (c_byte, c_ushort)
        self.assertEqual(self._dll.tf_bH(0, 65535), 21845)
        self.assertEqual(self.U(), 65535)

    def test_int(self):
        self._dll.tf_i.restype = c_int
        self._dll.tf_i.argtypes = (c_int,)
        self.assertEqual(self._dll.tf_i(-2147483646), -715827882)
        self.assertEqual(self.S(), -2147483646)

    def test_int_plus(self):
        self._dll.tf_bi.restype = c_int
        self._dll.tf_bi.argtypes = (c_byte, c_int)
        self.assertEqual(self._dll.tf_bi(0, -2147483646), -715827882)
        self.assertEqual(self.S(), -2147483646)

    def test_uint(self):
        self._dll.tf_I.restype = c_uint
        self._dll.tf_I.argtypes = (c_uint,)
        self.assertEqual(self._dll.tf_I(4294967295), 1431655765)
        self.assertEqual(self.U(), 4294967295)

    def test_uint_plus(self):
        self._dll.tf_bI.restype = c_uint
        self._dll.tf_bI.argtypes = (c_byte, c_uint)
        self.assertEqual(self._dll.tf_bI(0, 4294967295), 1431655765)
        self.assertEqual(self.U(), 4294967295)

    def test_long(self):
        self._dll.tf_l.restype = c_long
        self._dll.tf_l.argtypes = (c_long,)
        self.assertEqual(self._dll.tf_l(-2147483646), -715827882)
        self.assertEqual(self.S(), -2147483646)

    def test_long_plus(self):
        self._dll.tf_bl.restype = c_long
        self._dll.tf_bl.argtypes = (c_byte, c_long)
        self.assertEqual(self._dll.tf_bl(0, -2147483646), -715827882)
        self.assertEqual(self.S(), -2147483646)

    def test_ulong(self):
        self._dll.tf_L.restype = c_ulong
        self._dll.tf_L.argtypes = (c_ulong,)
        self.assertEqual(self._dll.tf_L(4294967295), 1431655765)
        self.assertEqual(self.U(), 4294967295)

    def test_ulong_plus(self):
        self._dll.tf_bL.restype = c_ulong
        self._dll.tf_bL.argtypes = (c_char, c_ulong)
        self.assertEqual(self._dll.tf_bL(b' ', 4294967295), 1431655765)
        self.assertEqual(self.U(), 4294967295)

    def test_longlong(self):
        self._dll.tf_q.restype = c_longlong
        self._dll.tf_q.argtypes = (c_longlong, )
        self.assertEqual(self._dll.tf_q(-9223372036854775806), -3074457345618258602)
        self.assertEqual(self.S(), -9223372036854775806)

    def test_longlong_plus(self):
        self._dll.tf_bq.restype = c_longlong
        self._dll.tf_bq.argtypes = (c_byte, c_longlong)
        self.assertEqual(self._dll.tf_bq(0, -9223372036854775806), -3074457345618258602)
        self.assertEqual(self.S(), -9223372036854775806)

    def test_ulonglong(self):
        self._dll.tf_Q.restype = c_ulonglong
        self._dll.tf_Q.argtypes = (c_ulonglong, )
        self.assertEqual(self._dll.tf_Q(18446744073709551615), 6148914691236517205)
        self.assertEqual(self.U(), 18446744073709551615)

    def test_ulonglong_plus(self):
        self._dll.tf_bQ.restype = c_ulonglong
        self._dll.tf_bQ.argtypes = (c_byte, c_ulonglong)
        self.assertEqual(self._dll.tf_bQ(0, 18446744073709551615), 6148914691236517205)
        self.assertEqual(self.U(), 18446744073709551615)

    def test_float(self):
        self._dll.tf_f.restype = c_float
        self._dll.tf_f.argtypes = (c_float,)
        self.assertEqual(self._dll.tf_f(-42.), -14.)
        self.assertEqual(self.S(), -42)

    def test_float_plus(self):
        self._dll.tf_bf.restype = c_float
        self._dll.tf_bf.argtypes = (c_byte, c_float)
        self.assertEqual(self._dll.tf_bf(0, -42.), -14.)
        self.assertEqual(self.S(), -42)

    def test_double(self):
        self._dll.tf_d.restype = c_double
        self._dll.tf_d.argtypes = (c_double,)
        self.assertEqual(self._dll.tf_d(42.), 14.)
        self.assertEqual(self.S(), 42)

    def test_double_plus(self):
        self._dll.tf_bd.restype = c_double
        self._dll.tf_bd.argtypes = (c_byte, c_double)
        self.assertEqual(self._dll.tf_bd(0, 42.), 14.)
        self.assertEqual(self.S(), 42)

    def test_longdouble(self):
        self._dll.tf_D.restype = c_longdouble
        self._dll.tf_D.argtypes = (c_longdouble,)
        self.assertEqual(self._dll.tf_D(42.), 14.)
        self.assertEqual(self.S(), 42)

    def test_longdouble_plus(self):
        self._dll.tf_bD.restype = c_longdouble
        self._dll.tf_bD.argtypes = (c_byte, c_longdouble)
        self.assertEqual(self._dll.tf_bD(0, 42.), 14.)
        self.assertEqual(self.S(), 42)

    def test_callwithresult(self):
        def process_result(result):
            return result * 2
        self._dll.tf_i.restype = process_result
        self._dll.tf_i.argtypes = (c_int,)
        self.assertEqual(self._dll.tf_i(42), 28)
        self.assertEqual(self.S(), 42)
        self.assertEqual(self._dll.tf_i(-42), -28)
        self.assertEqual(self.S(), -42)

    def test_void(self):
        self._dll.tv_i.restype = None
        self._dll.tv_i.argtypes = (c_int,)
        self.assertEqual(self._dll.tv_i(42), None)
        self.assertEqual(self.S(), 42)
        self.assertEqual(self._dll.tv_i(-42), None)
        self.assertEqual(self.S(), -42)

# The following repeats the above tests with stdcall functions (where
# they are available)
try:
    WinDLL
except NameError:
    pass
else:
    class stdcall_dll(WinDLL):
        def __getattr__(self, name):
            if name[:2] == '__' and name[-2:] == '__':
                raise AttributeError(name)
            func = self._FuncPtr(("s_" + name, self))
            setattr(self, name, func)
            return func

    class stdcallCFunctions(CFunctions):
        _dll = stdcall_dll(_ctypes_test.__file__)
        pass

if __name__ == '__main__':
    unittest.main()
