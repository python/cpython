Pending removal in Python 3.13
------------------------------

Modules (see :pep:`594`):

* :mod:`!aifc`
* :mod:`!audioop`
* :mod:`!cgi`
* :mod:`!cgitb`
* :mod:`!chunk`
* :mod:`!crypt`
* :mod:`!imghdr`
* :mod:`!mailcap`
* :mod:`!msilib`
* :mod:`!nis`
* :mod:`!nntplib`
* :mod:`!ossaudiodev`
* :mod:`!pipes`
* :mod:`!sndhdr`
* :mod:`!spwd`
* :mod:`!sunau`
* :mod:`!telnetlib`
* :mod:`!uu`
* :mod:`!xdrlib`

Other modules:

* :mod:`!lib2to3`, and the :program:`2to3` program (:gh:`84540`)

APIs:

* :class:`!configparser.LegacyInterpolation` (:gh:`90765`)
* ``locale.resetlocale()`` (:gh:`90817`)
* :meth:`!turtle.RawTurtle.settiltangle` (:gh:`50096`)
* :func:`!unittest.findTestCases` (:gh:`50096`)
* :func:`!unittest.getTestCaseNames` (:gh:`50096`)
* :func:`!unittest.makeSuite` (:gh:`50096`)
* :meth:`!unittest.TestProgram.usageExit` (:gh:`67048`)
* :class:`!webbrowser.MacOSX` (:gh:`86421`)
* :class:`classmethod` descriptor chaining (:gh:`89519`)
