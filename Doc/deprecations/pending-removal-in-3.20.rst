Pending removal in Python 3.20
------------------------------

* The ``__version__``, ``version`` and ``VERSION`` attributes have been
  deprecated in these standard library modules and will be removed in
  Python 3.20. Use :py:data:`sys.version_info` instead.

  - :mod:`argparse`
  - :mod:`csv`
  - :mod:`ctypes`
  - :mod:`!ctypes.macholib`
  - :mod:`decimal` (use :data:`decimal.SPEC_VERSION` instead)
  - :mod:`http.server`
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
  - :mod:`wsgiref.simple_server`
  - :mod:`xml.etree.ElementTree`
  - :mod:`!xml.sax.expatreader`
  - :mod:`xml.sax.handler`
  - :mod:`zlib`

  (Contributed by Hugo van Kemenade and Stan Ulbrych in :gh:`76007`.)
