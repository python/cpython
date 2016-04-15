:mod:`webbrowser` --- Convenient Web-browser controller
=======================================================

.. module:: webbrowser
   :synopsis: Easy-to-use controller for Web browsers.
.. moduleauthor:: Fred L. Drake, Jr. <fdrake@acm.org>
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>

**Source code:** :source:`Lib/webbrowser.py`

--------------

The :mod:`webbrowser` module provides a high-level interface to allow displaying
Web-based documents to users. Under most circumstances, simply calling the
:func:`.open` function from this module will do the right thing.

Under Unix, graphical browsers are preferred under X11, but text-mode browsers
will be used if graphical browsers are not available or an X11 display isn't
available.  If text-mode browsers are used, the calling process will block until
the user exits the browser.

If the environment variable :envvar:`BROWSER` exists, it is interpreted to
override the platform default list of browsers, as a :data:`os.pathsep`-separated
list of browsers to try in order.  When the value of a list part contains the
string ``%s``, then it is  interpreted as a literal browser command line to be
used with the argument URL substituted for ``%s``; if the part does not contain
``%s``, it is simply interpreted as the name of the browser to launch. [1]_

For non-Unix platforms, or when a remote browser is available on Unix, the
controlling process will not wait for the user to finish with the browser, but
allow the remote browser to maintain its own windows on the display.  If remote
browsers are not available on Unix, the controlling process will launch a new
browser and wait.

The script :program:`webbrowser` can be used as a command-line interface for the
module. It accepts a URL as the argument. It accepts the following optional
parameters: ``-n`` opens the URL in a new browser window, if possible;
``-t`` opens the URL in a new browser page ("tab"). The options are,
naturally, mutually exclusive.  Usage example::

   python -m webbrowser -t "http://www.python.org"

The following exception is defined:


.. exception:: Error

   Exception raised when a browser control error occurs.

The following functions are defined:


.. function:: open(url, new=0, autoraise=True)

   Display *url* using the default browser. If *new* is 0, the *url* is opened
   in the same browser window if possible.  If *new* is 1, a new browser window
   is opened if possible.  If *new* is 2, a new browser page ("tab") is opened
   if possible.  If *autoraise* is ``True``, the window is raised if possible
   (note that under many window managers this will occur regardless of the
   setting of this variable).

   Note that on some platforms, trying to open a filename using this function,
   may work and start the operating system's associated program.  However, this
   is neither supported nor portable.

   .. versionchanged:: 2.5
      *new* can now be 2.


.. function:: open_new(url)

   Open *url* in a new window of the default browser, if possible, otherwise, open
   *url* in the only browser window.

.. function:: open_new_tab(url)

   Open *url* in a new page ("tab") of the default browser, if possible, otherwise
   equivalent to :func:`open_new`.

   .. versionadded:: 2.5


.. function:: get([name])

   Return a controller object for the browser type *name*.  If *name* is empty,
   return a controller for a default browser appropriate to the caller's
   environment.


.. function:: register(name, constructor[, instance])

   Register the browser type *name*.  Once a browser type is registered, the
   :func:`get` function can return a controller for that browser type.  If
   *instance* is not provided, or is ``None``, *constructor* will be called without
   parameters to create an instance when needed.  If *instance* is provided,
   *constructor* will never be called, and may be ``None``.

   This entry point is only useful if you plan to either set the :envvar:`BROWSER`
   variable or call :func:`get` with a nonempty argument matching the name of a
   handler you declare.

A number of browser types are predefined.  This table gives the type names that
may be passed to the :func:`get` function and the corresponding instantiations
for the controller classes, all defined in this module.

+-----------------------+-----------------------------------------+-------+
| Type Name             | Class Name                              | Notes |
+=======================+=========================================+=======+
| ``'mozilla'``         | :class:`Mozilla('mozilla')`             |       |
+-----------------------+-----------------------------------------+-------+
| ``'firefox'``         | :class:`Mozilla('mozilla')`             |       |
+-----------------------+-----------------------------------------+-------+
| ``'netscape'``        | :class:`Mozilla('netscape')`            |       |
+-----------------------+-----------------------------------------+-------+
| ``'galeon'``          | :class:`Galeon('galeon')`               |       |
+-----------------------+-----------------------------------------+-------+
| ``'epiphany'``        | :class:`Galeon('epiphany')`             |       |
+-----------------------+-----------------------------------------+-------+
| ``'skipstone'``       | :class:`BackgroundBrowser('skipstone')` |       |
+-----------------------+-----------------------------------------+-------+
| ``'kfmclient'``       | :class:`Konqueror()`                    | \(1)  |
+-----------------------+-----------------------------------------+-------+
| ``'konqueror'``       | :class:`Konqueror()`                    | \(1)  |
+-----------------------+-----------------------------------------+-------+
| ``'kfm'``             | :class:`Konqueror()`                    | \(1)  |
+-----------------------+-----------------------------------------+-------+
| ``'mosaic'``          | :class:`BackgroundBrowser('mosaic')`    |       |
+-----------------------+-----------------------------------------+-------+
| ``'opera'``           | :class:`Opera()`                        |       |
+-----------------------+-----------------------------------------+-------+
| ``'grail'``           | :class:`Grail()`                        |       |
+-----------------------+-----------------------------------------+-------+
| ``'links'``           | :class:`GenericBrowser('links')`        |       |
+-----------------------+-----------------------------------------+-------+
| ``'elinks'``          | :class:`Elinks('elinks')`               |       |
+-----------------------+-----------------------------------------+-------+
| ``'lynx'``            | :class:`GenericBrowser('lynx')`         |       |
+-----------------------+-----------------------------------------+-------+
| ``'w3m'``             | :class:`GenericBrowser('w3m')`          |       |
+-----------------------+-----------------------------------------+-------+
| ``'windows-default'`` | :class:`WindowsDefault`                 | \(2)  |
+-----------------------+-----------------------------------------+-------+
| ``'macosx'``          | :class:`MacOSX('default')`              | \(3)  |
+-----------------------+-----------------------------------------+-------+
| ``'safari'``          | :class:`MacOSX('safari')`               | \(3)  |
+-----------------------+-----------------------------------------+-------+
| ``'google-chrome'``   | :class:`Chrome('google-chrome')`        | \(4)  |
+-----------------------+-----------------------------------------+-------+
| ``'chrome'``          | :class:`Chrome('chrome')`               | \(4)  |
+-----------------------+-----------------------------------------+-------+
| ``'chromium'``        | :class:`Chromium('chromium')`           | \(4)  |
+-----------------------+-----------------------------------------+-------+
| ``'chromium-browser'``| :class:`Chromium('chromium-browser')`   | \(4)  |
+-----------------------+-----------------------------------------+-------+

Notes:

(1)
   "Konqueror" is the file manager for the KDE desktop environment for Unix, and
   only makes sense to use if KDE is running.  Some way of reliably detecting KDE
   would be nice; the :envvar:`KDEDIR` variable is not sufficient.  Note also that
   the name "kfm" is used even when using the :program:`konqueror` command with KDE
   2 --- the implementation selects the best strategy for running Konqueror.

(2)
   Only on Windows platforms.

(3)
   Only on Mac OS X platform.

(4)
   Support for Chrome/Chromium has been added in version 2.7.5.

Here are some simple examples::

   url = 'http://www.python.org/'

   # Open URL in a new tab, if a browser window is already open.
   webbrowser.open_new_tab(url + 'doc/')

   # Open URL in new window, raising the window if possible.
   webbrowser.open_new(url)


.. _browser-controllers:

Browser Controller Objects
--------------------------

Browser controllers provide these methods which parallel three of the
module-level convenience functions:


.. method:: controller.open(url, new=0, autoraise=True)

   Display *url* using the browser handled by this controller. If *new* is 1, a new
   browser window is opened if possible. If *new* is 2, a new browser page ("tab")
   is opened if possible.


.. method:: controller.open_new(url)

   Open *url* in a new window of the browser handled by this controller, if
   possible, otherwise, open *url* in the only browser window.  Alias
   :func:`open_new`.


.. method:: controller.open_new_tab(url)

   Open *url* in a new page ("tab") of the browser handled by this controller, if
   possible, otherwise equivalent to :func:`open_new`.

   .. versionadded:: 2.5


.. rubric:: Footnotes

.. [1] Executables named here without a full path will be searched in the
       directories given in the :envvar:`PATH` environment variable.
