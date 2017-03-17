Issue #27662: Fix an overflow check in ``List_New``: the original code was
checking against ``Py_SIZE_MAX`` instead of the correct upper bound of
``Py_SSIZE_T_MAX``. Patch by Xiang Zhang.