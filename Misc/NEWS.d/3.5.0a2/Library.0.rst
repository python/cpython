- Issue #22524: New os.scandir() function, part of the PEP 471: "os.scandir()
  function -- a better and faster directory iterator". Patch written by Ben
  Hoyt.

- Issue #23103: Reduced the memory consumption of IPv4Address and IPv6Address.

- Issue #21793: BaseHTTPRequestHandler again logs response code as numeric,
  not as stringified enum.  Patch by Demian Brecht.

- Issue #23476: In the ssl module, enable OpenSSL's X509_V_FLAG_TRUSTED_FIRST
  flag on certificate stores when it is available.

- Issue #23576: Avoid stalling in SSL reads when EOF has been reached in the
  SSL layer but the underlying connection hasn't been closed.

- Issue #23504: Added an __all__ to the types module.

- Issue #23563: Optimized utility functions in urllib.parse.

- Issue #7830: Flatten nested functools.partial.

- Issue #20204: Added the __module__ attribute to _tkinter classes.

- Issue #19980: Improved help() for non-recognized strings.  help('') now
  shows the help on str.  help('help') now shows the help on help().
  Original patch by Mark Lawrence.

- Issue #23521: Corrected pure python implementation of timedelta division.
  
  Eliminated OverflowError from timedelta * float for some floats;
  Corrected rounding in timedlta true division.

- Issue #21619: Popen objects no longer leave a zombie after exit in the with
  statement if the pipe was broken.  Patch by Martin Panter.

- Issue #22936: Make it possible to show local variables in tracebacks for
  both the traceback module and unittest.

- Issue #15955: Add an option to limit the output size in bz2.decompress().
  Patch by Nikolaus Rath.

- Issue #6639: Module-level turtle functions no longer raise TclError after
  closing the window.

- Issues #814253, #9179: Group references and conditional group references now
  work in lookbehind assertions in regular expressions.

- Issue #23215: Multibyte codecs with custom error handlers that ignores errors
  consumed too much memory and raised SystemError or MemoryError.
  Original patch by Aleksi Torhamo.

- Issue #5700: io.FileIO() called flush() after closing the file.
  flush() was not called in close() if closefd=False.

- Issue #23374: Fixed pydoc failure with non-ASCII files when stdout encoding
  differs from file system encoding (e.g. on Mac OS).

- Issue #23481: Remove RC4 from the SSL module's default cipher list.

- Issue #21548: Fix pydoc.synopsis() and pydoc.apropos() on modules with empty
  docstrings.

- Issue #22885: Fixed arbitrary code execution vulnerability in the dbm.dumb
  module.  Original patch by Claudiu Popa.

- Issue #23239: ssl.match_hostname() now supports matching of IP addresses.

- Issue #23146: Fix mishandling of absolute Windows paths with forward
  slashes in pathlib.

- Issue #23096: Pickle representation of floats with protocol 0 now is the same
  for both Python and C implementations.

- Issue #19105: pprint now more efficiently uses free space at the right.

- Issue #14910: Add allow_abbrev parameter to argparse.ArgumentParser. Patch by
  Jonathan Paugh, Steven Bethard, paul j3 and Daniel Eriksson.

- Issue #21717: tarfile.open() now supports 'x' (exclusive creation) mode.

- Issue #23344: marshal.dumps() is now 20-25% faster on average.

- Issue #20416: marshal.dumps() with protocols 3 and 4 is now 40-50% faster on
  average.

- Issue #23421: Fixed compression in tarfile CLI.  Patch by wdv4758h.

- Issue #23367: Fix possible overflows in the unicodedata module.

- Issue #23361: Fix possible overflow in Windows subprocess creation code.

- logging.handlers.QueueListener now takes a respect_handler_level keyword
  argument which, if set to True, will pass messages to handlers taking handler
  levels into account.

- Issue #19705: turtledemo now has a visual sorting algorithm demo.  Original
  patch from Jason Yeo.

- Issue #23801: Fix issue where cgi.FieldStorage did not always ignore the
  entire preamble to a multipart body.

