Soft deprecations
-----------------

There are no plans to remove :term:`soft deprecated` APIs.

* :func:`re.match` and :meth:`re.Pattern.match` are now
  :term:`soft deprecated` in favor of the new :func:`re.prefixmatch` and
  :meth:`re.Pattern.prefixmatch` APIs, which have been added as alternate,
  more explicit names. These are intended to be used to alleviate confusion
  around what *match* means by following the Zen of Python's *"Explicit is
  better than implicit"* mantra. Most other language regular expression
  libraries use an API named *match* to mean what Python has always called
  *search*.

  We **do not** plan to remove the older :func:`!match` name, as it has been
  used in code for over 30 years. Code supporting older versions of Python
  should continue to use :func:`!match`, while new code should prefer
  :func:`!prefixmatch`. See :ref:`prefixmatch-vs-match`.

  (Contributed by Gregory P. Smith in :gh:`86519` and
  Hugo van Kemenade in :gh:`148100`.)
