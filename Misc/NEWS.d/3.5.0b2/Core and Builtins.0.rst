- Issue #24284: The startswith and endswith methods of the str class no longer
  return True when finding the empty string and the indexes are completely out
  of range.

- Issue #24115: Update uses of PyObject_IsTrue(), PyObject_Not(),
  PyObject_IsInstance(), PyObject_RichCompareBool() and _PyDict_Contains()
  to check for and handle errors correctly.

- Issue #24328: Fix importing one character extension modules.

- Issue #11205: In dictionary displays, evaluate the key before the value.

- Issue #24285: Fixed regression that prevented importing extension modules
  from inside packages. Patch by Petr Viktorin.

