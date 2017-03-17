- Issue #21750: mock_open.read_data can now be read from each instance, as it
  could in Python 3.3.

- Issue #24552: Fix use after free in an error case of the _pickle module.

- Issue #24514: tarfile now tolerates number fields consisting of only
  whitespace.

- Issue #19176: Fixed doctype() related bugs in C implementation of ElementTree.
  A deprecation warning no longer issued by XMLParser subclass with default
  doctype() method.  Direct call of doctype() now issues a warning.  Parser's
  doctype() now is not called if target's doctype() is called.  Based on patch
  by Martin Panter.

- Issue #20387: Restore semantic round-trip correctness in tokenize/untokenize
  for tab-indented blocks.

- Issue #24456: Fixed possible buffer over-read in adpcm2lin() and lin2adpcm()
  functions of the audioop module.

- Issue #24336: The contextmanager decorator now works with functions with
  keyword arguments called "func" and "self".  Patch by Martin Panter.

- Issue #24522: Fix possible integer overflow in json accelerator module.

- Issue #24489: ensure a previously set C errno doesn't disturb cmath.polar().

- Issue #24408: Fixed AttributeError in measure() and metrics() methods of
  tkinter.Font.

- Issue #14373: C implementation of functools.lru_cache() now can be used with
  methods.

- Issue #24347: Set KeyError if PyDict_GetItemWithError returns NULL.

- Issue #24348: Drop superfluous incref/decref.

- Issue #24359: Check for changed OrderedDict size during iteration.

- Issue #24368: Support keyword arguments in OrderedDict methods.

- Issue #24362: Simplify the C OrderedDict fast nodes resize logic.

- Issue #24377: Fix a ref leak in OrderedDict.__repr__.

- Issue #24369: Defend against key-changes during iteration.

