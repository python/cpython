# Test the windows specific win32reg module.
# Only win32reg functions not hit here: FlushKey, LoadKey and SaveKey

import os, sys, errno
import unittest
from test import support
threading = support.import_module("threading")
from platform import machine

# Do this first so test will be skipped if module doesn't exist
support.import_module('winreg')
# Now import everything
from winreg import *

try:
    REMOTE_NAME = sys.argv[sys.argv.index("--remote")+1]
except (IndexError, ValueError):
    REMOTE_NAME = None

# tuple of (major, minor)
WIN_VER = sys.getwindowsversion()[:2]
# Some tests should only run on 64-bit architectures where WOW64 will be.
WIN64_MACHINE = True if machine() == "AMD64" else False

# Starting with Windows 7 and Windows Server 2008 R2, WOW64 no longer uses
# registry reflection and formerly reflected keys are shared instead.
# Windows 7 and Windows Server 2008 R2 are version 6.1. Due to this, some
# tests are only valid up until 6.1
HAS_REFLECTION = True if WIN_VER < (6, 1) else False

# Use a per-process key to prevent concurrent test runs (buildbot!) from
# stomping on each other.
test_key_base = "Python Test Key [%d] - Delete Me" % (os.getpid(),)
test_key_name = "SOFTWARE\\" + test_key_base
# On OS'es that support reflection we should test with a reflected key
test_reflect_key_name = "SOFTWARE\\Classes\\" + test_key_base

test_data = [
    ("Int Value",     45,                                      REG_DWORD),
    ("String Val",    "A string value",                        REG_SZ),
    ("StringExpand",  "The path is %path%",                    REG_EXPAND_SZ),
    ("Multi-string",  ["Lots", "of", "string", "values"],      REG_MULTI_SZ),
    ("Raw Data",      b"binary\x00data",                       REG_BINARY),
    ("Big String",    "x"*(2**14-1),                           REG_SZ),
    ("Big Binary",    b"x"*(2**14),                            REG_BINARY),
    # Two and three kanjis, meaning: "Japan" and "Japanese")
    ("Japanese 日本", "日本語", REG_SZ),
]

class BaseWinregTests(unittest.TestCase):

    def setUp(self):
        # Make sure that the test key is absent when the test
        # starts.
        self.delete_tree(HKEY_CURRENT_USER, test_key_name)

    def delete_tree(self, root, subkey):
        try:
            hkey = OpenKey(root, subkey, KEY_ALL_ACCESS)
        except WindowsError:
            # subkey does not exist
            return
        while True:
            try:
                subsubkey = EnumKey(hkey, 0)
            except WindowsError:
                # no more subkeys
                break
            self.delete_tree(hkey, subsubkey)
        CloseKey(hkey)
        DeleteKey(root, subkey)

    def _write_test_data(self, root_key, subkeystr="sub_key",
                         CreateKey=CreateKey):
        # Set the default value for this key.
        SetValue(root_key, test_key_name, REG_SZ, "Default value")
        key = CreateKey(root_key, test_key_name)
        self.assertTrue(key.handle != 0)
        # Create a sub-key
        sub_key = CreateKey(key, subkeystr)
        # Give the sub-key some named values

        for value_name, value_data, value_type in test_data:
            SetValueEx(sub_key, value_name, 0, value_type, value_data)

        # Check we wrote as many items as we thought.
        nkeys, nvalues, since_mod = QueryInfoKey(key)
        self.assertEqual(nkeys, 1, "Not the correct number of sub keys")
        self.assertEqual(nvalues, 1, "Not the correct number of values")
        nkeys, nvalues, since_mod = QueryInfoKey(sub_key)
        self.assertEqual(nkeys, 0, "Not the correct number of sub keys")
        self.assertEqual(nvalues, len(test_data),
                         "Not the correct number of values")
        # Close this key this way...
        # (but before we do, copy the key as an integer - this allows
        # us to test that the key really gets closed).
        int_sub_key = int(sub_key)
        CloseKey(sub_key)
        try:
            QueryInfoKey(int_sub_key)
            self.fail("It appears the CloseKey() function does "
                      "not close the actual key!")
        except EnvironmentError:
            pass
        # ... and close that key that way :-)
        int_key = int(key)
        key.Close()
        try:
            QueryInfoKey(int_key)
            self.fail("It appears the key.Close() function "
                      "does not close the actual key!")
        except EnvironmentError:
            pass

    def _read_test_data(self, root_key, subkeystr="sub_key", OpenKey=OpenKey):
        # Check we can get default value for this key.
        val = QueryValue(root_key, test_key_name)
        self.assertEqual(val, "Default value",
                         "Registry didn't give back the correct value")

        key = OpenKey(root_key, test_key_name)
        # Read the sub-keys
        with OpenKey(key, subkeystr) as sub_key:
            # Check I can enumerate over the values.
            index = 0
            while 1:
                try:
                    data = EnumValue(sub_key, index)
                except EnvironmentError:
                    break
                self.assertEqual(data in test_data, True,
                                 "Didn't read back the correct test data")
                index = index + 1
            self.assertEqual(index, len(test_data),
                             "Didn't read the correct number of items")
            # Check I can directly access each item
            for value_name, value_data, value_type in test_data:
                read_val, read_typ = QueryValueEx(sub_key, value_name)
                self.assertEqual(read_val, value_data,
                                 "Could not directly read the value")
                self.assertEqual(read_typ, value_type,
                                 "Could not directly read the value")
        sub_key.Close()
        # Enumerate our main key.
        read_val = EnumKey(key, 0)
        self.assertEqual(read_val, subkeystr, "Read subkey value wrong")
        try:
            EnumKey(key, 1)
            self.fail("Was able to get a second key when I only have one!")
        except EnvironmentError:
            pass

        key.Close()

    def _delete_test_data(self, root_key, subkeystr="sub_key"):
        key = OpenKey(root_key, test_key_name, 0, KEY_ALL_ACCESS)
        sub_key = OpenKey(key, subkeystr, 0, KEY_ALL_ACCESS)
        # It is not necessary to delete the values before deleting
        # the key (although subkeys must not exist).  We delete them
        # manually just to prove we can :-)
        for value_name, value_data, value_type in test_data:
            DeleteValue(sub_key, value_name)

        nkeys, nvalues, since_mod = QueryInfoKey(sub_key)
        self.assertEqual(nkeys, 0, "subkey not empty before delete")
        self.assertEqual(nvalues, 0, "subkey not empty before delete")
        sub_key.Close()
        DeleteKey(key, subkeystr)

        try:
            # Shouldnt be able to delete it twice!
            DeleteKey(key, subkeystr)
            self.fail("Deleting the key twice succeeded")
        except EnvironmentError:
            pass
        key.Close()
        DeleteKey(root_key, test_key_name)
        # Opening should now fail!
        try:
            key = OpenKey(root_key, test_key_name)
            self.fail("Could open the non-existent key")
        except WindowsError: # Use this error name this time
            pass

    def _test_all(self, root_key, subkeystr="sub_key"):
        self._write_test_data(root_key, subkeystr)
        self._read_test_data(root_key, subkeystr)
        self._delete_test_data(root_key, subkeystr)

    def _test_named_args(self, key, sub_key):
        with CreateKeyEx(key=key, sub_key=sub_key, reserved=0,
                         access=KEY_ALL_ACCESS) as ckey:
            self.assertTrue(ckey.handle != 0)

        with OpenKeyEx(key=key, sub_key=sub_key, reserved=0,
                       access=KEY_ALL_ACCESS) as okey:
            self.assertTrue(okey.handle != 0)


class LocalWinregTests(BaseWinregTests):

    def test_registry_works(self):
        self._test_all(HKEY_CURRENT_USER)
        self._test_all(HKEY_CURRENT_USER, "日本-subkey")

    def test_registry_works_extended_functions(self):
        # Substitute the regular CreateKey and OpenKey calls with their
        # extended counterparts.
        # Note: DeleteKeyEx is not used here because it is platform dependent
        cke = lambda key, sub_key: CreateKeyEx(key, sub_key, 0, KEY_ALL_ACCESS)
        self._write_test_data(HKEY_CURRENT_USER, CreateKey=cke)

        oke = lambda key, sub_key: OpenKeyEx(key, sub_key, 0, KEY_READ)
        self._read_test_data(HKEY_CURRENT_USER, OpenKey=oke)

        self._delete_test_data(HKEY_CURRENT_USER)

    def test_named_arguments(self):
        self._test_named_args(HKEY_CURRENT_USER, test_key_name)
        # Use the regular DeleteKey to clean up
        # DeleteKeyEx takes named args and is tested separately
        DeleteKey(HKEY_CURRENT_USER, test_key_name)

    def test_connect_registry_to_local_machine_works(self):
        # perform minimal ConnectRegistry test which just invokes it
        h = ConnectRegistry(None, HKEY_LOCAL_MACHINE)
        self.assertNotEqual(h.handle, 0)
        h.Close()
        self.assertEqual(h.handle, 0)

    def test_inexistant_remote_registry(self):
        connect = lambda: ConnectRegistry("abcdefghijkl", HKEY_CURRENT_USER)
        self.assertRaises(WindowsError, connect)

    def testExpandEnvironmentStrings(self):
        r = ExpandEnvironmentStrings("%windir%\\test")
        self.assertEqual(type(r), str)
        self.assertEqual(r, os.environ["windir"] + "\\test")

    def test_context_manager(self):
        # ensure that the handle is closed if an exception occurs
        try:
            with ConnectRegistry(None, HKEY_LOCAL_MACHINE) as h:
                self.assertNotEqual(h.handle, 0)
                raise WindowsError
        except WindowsError:
            self.assertEqual(h.handle, 0)

    def test_changing_value(self):
        # Issue2810: A race condition in 2.6 and 3.1 may cause
        # EnumValue or QueryValue to raise "WindowsError: More data is
        # available"
        done = False

        class VeryActiveThread(threading.Thread):
            def run(self):
                with CreateKey(HKEY_CURRENT_USER, test_key_name) as key:
                    use_short = True
                    long_string = 'x'*2000
                    while not done:
                        s = 'x' if use_short else long_string
                        use_short = not use_short
                        SetValue(key, 'changing_value', REG_SZ, s)

        thread = VeryActiveThread()
        thread.start()
        try:
            with CreateKey(HKEY_CURRENT_USER,
                           test_key_name+'\\changing_value') as key:
                for _ in range(1000):
                    num_subkeys, num_values, t = QueryInfoKey(key)
                    for i in range(num_values):
                        name = EnumValue(key, i)
                        QueryValue(key, name[0])
        finally:
            done = True
            thread.join()
            DeleteKey(HKEY_CURRENT_USER, test_key_name+'\\changing_value')
            DeleteKey(HKEY_CURRENT_USER, test_key_name)

    def test_long_key(self):
        # Issue2810, in 2.6 and 3.1 when the key name was exactly 256
        # characters, EnumKey raised "WindowsError: More data is
        # available"
        name = 'x'*256
        try:
            with CreateKey(HKEY_CURRENT_USER, test_key_name) as key:
                SetValue(key, name, REG_SZ, 'x')
                num_subkeys, num_values, t = QueryInfoKey(key)
                EnumKey(key, 0)
        finally:
            DeleteKey(HKEY_CURRENT_USER, '\\'.join((test_key_name, name)))
            DeleteKey(HKEY_CURRENT_USER, test_key_name)

    def test_dynamic_key(self):
        # Issue2810, when the value is dynamically generated, these
        # raise "WindowsError: More data is available" in 2.6 and 3.1
        try:
            EnumValue(HKEY_PERFORMANCE_DATA, 0)
        except OSError as e:
            if e.errno in (errno.EPERM, errno.EACCES):
                self.skipTest("access denied to registry key "
                              "(are you running in a non-interactive session?)")
            raise
        QueryValueEx(HKEY_PERFORMANCE_DATA, "")

    # Reflection requires XP x64/Vista at a minimum. XP doesn't have this stuff
    # or DeleteKeyEx so make sure their use raises NotImplementedError
    @unittest.skipUnless(WIN_VER < (5, 2), "Requires Windows XP")
    def test_reflection_unsupported(self):
        try:
            with CreateKey(HKEY_CURRENT_USER, test_key_name) as ck:
                self.assertNotEqual(ck.handle, 0)

            key = OpenKey(HKEY_CURRENT_USER, test_key_name)
            self.assertNotEqual(key.handle, 0)

            with self.assertRaises(NotImplementedError):
                DisableReflectionKey(key)
            with self.assertRaises(NotImplementedError):
                EnableReflectionKey(key)
            with self.assertRaises(NotImplementedError):
                QueryReflectionKey(key)
            with self.assertRaises(NotImplementedError):
                DeleteKeyEx(HKEY_CURRENT_USER, test_key_name)
        finally:
            DeleteKey(HKEY_CURRENT_USER, test_key_name)

    def test_setvalueex_value_range(self):
        # Test for Issue #14420, accept proper ranges for SetValueEx.
        # Py2Reg, which gets called by SetValueEx, was using PyLong_AsLong,
        # thus raising OverflowError. The implementation now uses
        # PyLong_AsUnsignedLong to match DWORD's size.
        try:
            with CreateKey(HKEY_CURRENT_USER, test_key_name) as ck:
                self.assertNotEqual(ck.handle, 0)
                SetValueEx(ck, "test_name", None, REG_DWORD, 0x80000000)
        finally:
            DeleteKey(HKEY_CURRENT_USER, test_key_name)

    def test_queryvalueex_return_value(self):
        # Test for Issue #16759, return unsigned int from QueryValueEx.
        # Reg2Py, which gets called by QueryValueEx, was returning a value
        # generated by PyLong_FromLong. The implmentation now uses
        # PyLong_FromUnsignedLong to match DWORD's size.
        try:
            with CreateKey(HKEY_CURRENT_USER, test_key_name) as ck:
                self.assertNotEqual(ck.handle, 0)
                test_val = 0x80000000
                SetValueEx(ck, "test_name", None, REG_DWORD, test_val)
                ret_val, ret_type = QueryValueEx(ck, "test_name")
                self.assertEqual(ret_type, REG_DWORD)
                self.assertEqual(ret_val, test_val)
        finally:
            DeleteKey(HKEY_CURRENT_USER, test_key_name)



@unittest.skipUnless(REMOTE_NAME, "Skipping remote registry tests")
class RemoteWinregTests(BaseWinregTests):

    def test_remote_registry_works(self):
        remote_key = ConnectRegistry(REMOTE_NAME, HKEY_CURRENT_USER)
        self._test_all(remote_key)


@unittest.skipUnless(WIN64_MACHINE, "x64 specific registry tests")
class Win64WinregTests(BaseWinregTests):

    def test_named_arguments(self):
        self._test_named_args(HKEY_CURRENT_USER, test_key_name)
        # Clean up and also exercise the named arguments
        DeleteKeyEx(key=HKEY_CURRENT_USER, sub_key=test_key_name,
                    access=KEY_ALL_ACCESS, reserved=0)

    def test_reflection_functions(self):
        # Test that we can call the query, enable, and disable functions
        # on a key which isn't on the reflection list with no consequences.
        with OpenKey(HKEY_LOCAL_MACHINE, "Software") as key:
            # HKLM\Software is redirected but not reflected in all OSes
            self.assertTrue(QueryReflectionKey(key))
            self.assertIsNone(EnableReflectionKey(key))
            self.assertIsNone(DisableReflectionKey(key))
            self.assertTrue(QueryReflectionKey(key))

    @unittest.skipUnless(HAS_REFLECTION, "OS doesn't support reflection")
    def test_reflection(self):
        # Test that we can create, open, and delete keys in the 32-bit
        # area. Because we are doing this in a key which gets reflected,
        # test the differences of 32 and 64-bit keys before and after the
        # reflection occurs (ie. when the created key is closed).
        try:
            with CreateKeyEx(HKEY_CURRENT_USER, test_reflect_key_name, 0,
                             KEY_ALL_ACCESS | KEY_WOW64_32KEY) as created_key:
                self.assertNotEqual(created_key.handle, 0)

                # The key should now be available in the 32-bit area
                with OpenKey(HKEY_CURRENT_USER, test_reflect_key_name, 0,
                             KEY_ALL_ACCESS | KEY_WOW64_32KEY) as key:
                    self.assertNotEqual(key.handle, 0)

                # Write a value to what currently is only in the 32-bit area
                SetValueEx(created_key, "", 0, REG_SZ, "32KEY")

                # The key is not reflected until created_key is closed.
                # The 64-bit version of the key should not be available yet.
                open_fail = lambda: OpenKey(HKEY_CURRENT_USER,
                                            test_reflect_key_name, 0,
                                            KEY_READ | KEY_WOW64_64KEY)
                self.assertRaises(WindowsError, open_fail)

            # Now explicitly open the 64-bit version of the key
            with OpenKey(HKEY_CURRENT_USER, test_reflect_key_name, 0,
                         KEY_ALL_ACCESS | KEY_WOW64_64KEY) as key:
                self.assertNotEqual(key.handle, 0)
                # Make sure the original value we set is there
                self.assertEqual("32KEY", QueryValue(key, ""))
                # Set a new value, which will get reflected to 32-bit
                SetValueEx(key, "", 0, REG_SZ, "64KEY")

            # Reflection uses a "last-writer wins policy, so the value we set
            # on the 64-bit key should be the same on 32-bit
            with OpenKey(HKEY_CURRENT_USER, test_reflect_key_name, 0,
                         KEY_READ | KEY_WOW64_32KEY) as key:
                self.assertEqual("64KEY", QueryValue(key, ""))
        finally:
            DeleteKeyEx(HKEY_CURRENT_USER, test_reflect_key_name,
                        KEY_WOW64_32KEY, 0)

    @unittest.skipUnless(HAS_REFLECTION, "OS doesn't support reflection")
    def test_disable_reflection(self):
        # Make use of a key which gets redirected and reflected
        try:
            with CreateKeyEx(HKEY_CURRENT_USER, test_reflect_key_name, 0,
                             KEY_ALL_ACCESS | KEY_WOW64_32KEY) as created_key:
                # QueryReflectionKey returns whether or not the key is disabled
                disabled = QueryReflectionKey(created_key)
                self.assertEqual(type(disabled), bool)
                # HKCU\Software\Classes is reflected by default
                self.assertFalse(disabled)

                DisableReflectionKey(created_key)
                self.assertTrue(QueryReflectionKey(created_key))

            # The key is now closed and would normally be reflected to the
            # 64-bit area, but let's make sure that didn't happen.
            open_fail = lambda: OpenKeyEx(HKEY_CURRENT_USER,
                                          test_reflect_key_name, 0,
                                          KEY_READ | KEY_WOW64_64KEY)
            self.assertRaises(WindowsError, open_fail)

            # Make sure the 32-bit key is actually there
            with OpenKeyEx(HKEY_CURRENT_USER, test_reflect_key_name, 0,
                           KEY_READ | KEY_WOW64_32KEY) as key:
                self.assertNotEqual(key.handle, 0)
        finally:
            DeleteKeyEx(HKEY_CURRENT_USER, test_reflect_key_name,
                        KEY_WOW64_32KEY, 0)

    def test_exception_numbers(self):
        with self.assertRaises(FileNotFoundError) as ctx:
            QueryValue(HKEY_CLASSES_ROOT, 'some_value_that_does_not_exist')

def test_main():
    support.run_unittest(LocalWinregTests, RemoteWinregTests,
                         Win64WinregTests)

if __name__ == "__main__":
    if not REMOTE_NAME:
        print("Remote registry calls can be tested using",
              "'test_winreg.py --remote \\\\machine_name'")
    test_main()
