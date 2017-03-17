- Issue #19548: Update the codecs module documentation to better cover the
  distinction between text encodings and other codecs, together with other
  clarifications. Patch by Martin Panter.

- Issue #22394: Doc/Makefile now supports ``make venv PYTHON=../python`` to
  create a venv for generating the documentation, e.g.,
  ``make html PYTHON=venv/bin/python3``.

- Issue #21514: The documentation of the json module now refers to new JSON RFC
  7159 instead of obsoleted RFC 4627.

- Issue #21777: The binary sequence methods on bytes and bytearray are now
  documented explicitly, rather than assuming users will be able to derive
  the expected behaviour from the behaviour of the corresponding str methods.

- Issue #6916: undocument deprecated asynchat.fifo class.

- Issue #17386: Expanded functionality of the ``Doc/make.bat`` script to make
  it much more comparable to ``Doc/Makefile``.

- Issue #21312: Update the thread_foobar.h template file to include newer
  threading APIs.  Patch by Jack McCracken.

- Issue #21043: Remove the recommendation for specific CA organizations and to
  mention the ability to load the OS certificates.

- Issue #20765: Add missing documentation for PurePath.with_name() and
  PurePath.with_suffix().

- Issue #19407: New package installation and distribution guides based on
  the Python Packaging Authority tools. Existing guides have been retained
  as legacy links from the distutils docs, as they still contain some
  required reference material for tool developers that isn't recorded
  anywhere else.

- Issue #19697: Document cases where __main__.__spec__ is None.

