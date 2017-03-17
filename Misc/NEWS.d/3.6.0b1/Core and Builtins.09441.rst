Issue #27782: Multi-phase extension module import now correctly allows the
``m_methods`` field to be used to add module level functions to instances
of non-module types returned from ``Py_create_mod``. Patch by Xiang Zhang.