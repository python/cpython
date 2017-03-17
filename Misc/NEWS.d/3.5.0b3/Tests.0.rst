- Issue #24373: _testmultiphase and xxlimited now use tp_traverse and
  tp_finalize to avoid reference leaks encountered when combining tp_dealloc
  with PyType_FromSpec (see issue #16690 for details)

