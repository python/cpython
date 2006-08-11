# Test the windows specific win32reg module.
# Only win32reg functions not hit here: FlushKey, LoadKey and SaveKey

from _winreg import *
import os, sys

from test.test_support import verify, have_unicode

test_key_name = "SOFTWARE\\Python Registry Test Key - Delete Me"

test_data = [
    ("Int Value",     45,                                      REG_DWORD),
    ("String Val",    "A string value",                        REG_SZ),
    ("StringExpand",  "The path is %path%",                    REG_EXPAND_SZ),
    ("Multi-string",  ["Lots", "of", "string", "values"],      REG_MULTI_SZ),
    ("Raw Data",      ("binary"+chr(0)+"data"),                REG_BINARY),
    ("Big String",    "x"*(2**14-1),                           REG_SZ),
    ("Big Binary",    "x"*(2**14),                             REG_BINARY),
]
if have_unicode:
    test_data+=[
    (unicode("Unicode Val"),  unicode("A Unicode value"),                      REG_SZ,),
    ("UnicodeExpand", unicode("The path is %path%"),                   REG_EXPAND_SZ),
    ("Multi-unicode", [unicode("Lots"), unicode("of"), unicode("unicode"), unicode("values")], REG_MULTI_SZ),
    ("Multi-mixed",   [unicode("Unicode"), unicode("and"), "string", "values"],REG_MULTI_SZ),
    ]

def WriteTestData(root_key):
    # Set the default value for this key.
    SetValue(root_key, test_key_name, REG_SZ, "Default value")
    key = CreateKey(root_key, test_key_name)
    # Create a sub-key
    sub_key = CreateKey(key, "sub_key")
    # Give the sub-key some named values

    for value_name, value_data, value_type in test_data:
        SetValueEx(sub_key, value_name, 0, value_type, value_data)

    # Check we wrote as many items as we thought.
    nkeys, nvalues, since_mod = QueryInfoKey(key)
    verify(nkeys==1, "Not the correct number of sub keys")
    verify(nvalues==1, "Not the correct number of values")
    nkeys, nvalues, since_mod = QueryInfoKey(sub_key)
    verify(nkeys==0, "Not the correct number of sub keys")
    verify(nvalues==len(test_data), "Not the correct number of values")
    # Close this key this way...
    # (but before we do, copy the key as an integer - this allows
    # us to test that the key really gets closed).
    int_sub_key = int(sub_key)
    CloseKey(sub_key)
    try:
        QueryInfoKey(int_sub_key)
        raise RuntimeError, "It appears the CloseKey() function does not close the actual key!"
    except EnvironmentError:
        pass
    # ... and close that key that way :-)
    int_key = int(key)
    key.Close()
    try:
        QueryInfoKey(int_key)
        raise RuntimeError, "It appears the key.Close() function does not close the actual key!"
    except EnvironmentError:
        pass

def ReadTestData(root_key):
    # Check we can get default value for this key.
    val = QueryValue(root_key, test_key_name)
    verify(val=="Default value", "Registry didn't give back the correct value")

    key = OpenKey(root_key, test_key_name)
    # Read the sub-keys
    sub_key = OpenKey(key, "sub_key")
    # Check I can enumerate over the values.
    index = 0
    while 1:
        try:
            data = EnumValue(sub_key, index)
        except EnvironmentError:
            break
        verify(data in test_data, "Didn't read back the correct test data")
        index = index + 1
    verify(index==len(test_data), "Didn't read the correct number of items")
    # Check I can directly access each item
    for value_name, value_data, value_type in test_data:
        read_val, read_typ = QueryValueEx(sub_key, value_name)
        verify(read_val==value_data and read_typ == value_type, \
               "Could not directly read the value" )
    sub_key.Close()
    # Enumerate our main key.
    read_val = EnumKey(key, 0)
    verify(read_val == "sub_key", "Read subkey value wrong")
    try:
        EnumKey(key, 1)
        verify(0, "Was able to get a second key when I only have one!")
    except EnvironmentError:
        pass

    key.Close()

def DeleteTestData(root_key):
    key = OpenKey(root_key, test_key_name, 0, KEY_ALL_ACCESS)
    sub_key = OpenKey(key, "sub_key", 0, KEY_ALL_ACCESS)
    # It is not necessary to delete the values before deleting
    # the key (although subkeys must not exist).  We delete them
    # manually just to prove we can :-)
    for value_name, value_data, value_type in test_data:
        DeleteValue(sub_key, value_name)

    nkeys, nvalues, since_mod = QueryInfoKey(sub_key)
    verify(nkeys==0 and nvalues==0, "subkey not empty before delete")
    sub_key.Close()
    DeleteKey(key, "sub_key")

    try:
        # Shouldnt be able to delete it twice!
        DeleteKey(key, "sub_key")
        verify(0, "Deleting the key twice succeeded")
    except EnvironmentError:
        pass
    key.Close()
    DeleteKey(root_key, test_key_name)
    # Opening should now fail!
    try:
        key = OpenKey(root_key, test_key_name)
        verify(0, "Could open the non-existent key")
    except WindowsError: # Use this error name this time
        pass

def TestAll(root_key):
    WriteTestData(root_key)
    ReadTestData(root_key)
    DeleteTestData(root_key)

# Test on my local machine.
TestAll(HKEY_CURRENT_USER)
print "Local registry tests worked"
try:
    remote_name = sys.argv[sys.argv.index("--remote")+1]
except (IndexError, ValueError):
    remote_name = None

if remote_name is not None:
    try:
        remote_key = ConnectRegistry(remote_name, HKEY_CURRENT_USER)
    except EnvironmentError, exc:
        print "Could not connect to the remote machine -", exc.strerror
        remote_key = None
    if remote_key is not None:
        TestAll(remote_key)
        print "Remote registry tests worked"
else:
    print "Remote registry calls can be tested using",
    print "'test_winreg.py --remote \\\\machine_name'"
    # perform minimal ConnectRegistry test which just invokes it
    h = ConnectRegistry(None, HKEY_LOCAL_MACHINE)
    h.Close()
