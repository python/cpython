Pending removal in Python 3.20
------------------------------

* The ``__version__`` attribute has been deprecated in these standard library
  modules and will be removed in Python 3.20.
  Use :py:data:`sys.version_info` instead.

  - :mod:`argparse`
  - :mod:`csv`
  - :mod:`!ctypes.macholib`
  - :mod:`decimal` (use :data:`decimal.SPEC_VERSION` instead)
  - :mod:`imaplib`
  - :mod:`ipaddress`
  - :mod:`json`
  - :mod:`logging` (``__date__`` also deprecated)
  - :mod:`optparse`
  - :mod:`pickle`
  - :mod:`platform`
  - :mod:`re`
  - :mod:`socketserver`
  - :mod:`tabnanny`
  - :mod:`tkinter.font`
  - :mod:`tkinter.ttk`
  - :mod:`zlib`

  (Contributed by Hugo van Kemenade and Stan Ulbrych in :gh:`76007`.)
