:mod:`!webbrowser` --- Convenient web-browser controller
========================================================

.. module:: webbrowser
   :synopsis: Easy-to-use controller for web browsers.

.. moduleauthor:: Fred L. Drake, Jr. <fdrake@acm.org>
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>

**Source code:** :source:`Lib/webbrowser.py`

--------------

The :mod:`webbrowser` module provides a high-level interface to allow displaying
web-based documents to users. Under most circumstances, simply calling the
:func:`.open` function from this module will do the right thing.

Under Unix, graphical browsers are preferred under X11, but text-mode browsers
will be used if graphical browsers are not available or an X11 display isn't
available.  If text-mode browsers are used, the calling process will block until
the user exits the browser.

If the environment variable :envvar:`BROWSER` exists, it is interpreted as the
:data:`os.pathsep`-separated list of browsers to try ahead of the platform
defaults.  When the value of a list part contains the string ``%s``, then it is
interpreted as a literal browser command line to be used with the argument URL
substituted for ``%s``; if the value is a single word that refers to one of the
already registered browsers this browser is added to the front of the search list;
if the part does not contain ``%s``, it is simply interpreted as the name of the
browser to launch. [1]_

.. versionchanged:: 3.14

   The :envvar:`BROWSER` variable can now also be used to reorder the list of
   platform defaults. This is particularly useful on macOS where the platform
   defaults do not refer to command-line tools on :envvar:`PATH`.


For non-Unix platforms, or when a remote browser is available on Unix, the
controlling process will not wait for the user to finish with the browser, but
allow the remote browser to maintain its own windows on the display.  If remote
browsers are not available on Unix, the controlling process will launch a new
browser and wait.

On iOS, the :envvar:`BROWSER` environment variable, as well as any arguments
controlling autoraise, browser preference, and new tab/window creation will be
ignored. Web pages will *always* be opened in the user's preferred browser, in
a new tab, with the browser being brought to the foreground. The use of the
:mod:`webbrowser` module on iOS requires the :mod:`ctypes` module. If
:mod:`ctypes` isn't available, calls to :func:`.open` will fail.

.. program:: webbrowser

The script :program:`webbrowser` can be used as a command-line interface for the
module. It accepts a URL as the argument. It accepts the following optional
parameters:

.. option:: -n, --new-window

   Opens the URL in a new browser window, if possible.

.. option:: -t, --new-tab

   Opens the URL in a new browser tab.

The options are, naturally, mutually exclusive.  Usage example:

.. code-block:: bash

   python -m webbrowser -t "https://www.python.org"

.. availability:: not WASI, not Android.

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

   Returns ``True`` if a browser was successfully launched, ``False`` otherwise.

   Note that on some platforms, trying to open a filename using this function,
   may work and start the operating system's associated program.  However, this
   is neither supported nor portable.

   .. audit-event:: webbrowser.open url webbrowser.open


.. function:: open_new(url)

   Open *url* in a new window of the default browser, if possible, otherwise, open
   *url* in the only browser window.

   Returns ``True`` if a browser was successfully launched, ``False`` otherwise.


.. function:: open_new_tab(url)

   Open *url* in a new page ("tab") of the default browser, if possible, otherwise
   equivalent to :func:`open_new`.

   Returns ``True`` if a browser was successfully launched, ``False`` otherwise.


.. function:: get(using=None)

   Return a controller object for the browser type *using*.  If *using* is
   ``None``, return a controller for a default browser appropriate to the
   caller's environment.


.. function:: register(name, constructor, instance=None, *, preferred=False)

   Register the browser type *name*.  Once a browser type is registered, the
   :func:`get` function can return a controller for that browser type.  If
   *instance* is not provided, or is ``None``, *constructor* will be called without
   parameters to create an instance when needed.  If *instance* is provided,
   *constructor* will never be called, and may be ``None``.

   Setting *preferred* to ``True`` makes this browser a preferred result for
   a :func:`get` call with no argument.  Otherwise, this entry point is only
   useful if you plan to either set the :envvar:`BROWSER` variable or call
   :func:`get` with a nonempty argument matching the name of a handler you
   declare.

   .. versionchanged:: 3.7
      *preferred* keyword-only parameter was added.

A number of browser types are predefined.  This table gives the type names that
may be passed to the :func:`get` function and the corresponding instantiations
for the controller classes, all defined in this module.

+------------------------+-----------------------------------------+-------+
| Type Name              | Class Name                              | Notes |
+========================+=========================================+=======+
| ``'mozilla'``          | ``Mozilla('mozilla')``                  |       |
+------------------------+-----------------------------------------+-------+
| ``'firefox'``          | ``Mozilla('mozilla')``                  |       |
+------------------------+-----------------------------------------+-------+
| ``'epiphany'``         | ``Epiphany('epiphany')``                |       |
+------------------------+-----------------------------------------+-------+
| ``'kfmclient'``        | ``Konqueror()``                         | \(1)  |
+------------------------+-----------------------------------------+-------+
| ``'konqueror'``        | ``Konqueror()``                         | \(1)  |
+------------------------+-----------------------------------------+-------+
| ``'kfm'``              | ``Konqueror()``                         | \(1)  |
+------------------------+-----------------------------------------+-------+
| ``'opera'``            | ``Opera()``                             |       |
+------------------------+-----------------------------------------+-------+
| ``'links'``            | ``GenericBrowser('links')``             |       |
+------------------------+-----------------------------------------+-------+
| ``'elinks'``           | ``Elinks('elinks')``                    |       |
+------------------------+-----------------------------------------+-------+
| ``'lynx'``             | ``GenericBrowser('lynx')``              |       |
+------------------------+-----------------------------------------+-------+
| ``'w3m'``              | ``GenericBrowser('w3m')``               |       |
+------------------------+-----------------------------------------+-------+
| ``'windows-default'``  | ``WindowsDefault``                      | \(2)  |
+------------------------+-----------------------------------------+-------+
| ``'macosx'``           | ``MacOSXOSAScript('default')``          | \(3)  |
+------------------------+-----------------------------------------+-------+
| ``'safari'``           | ``MacOSXOSAScript('safari')``           | \(3)  |
+------------------------+-----------------------------------------+-------+
| ``'google-chrome'``    | ``Chrome('google-chrome')``             |       |
+------------------------+-----------------------------------------+-------+
| ``'chrome'``           | ``Chrome('chrome')``                    |       |
+------------------------+-----------------------------------------+-------+
| ``'chromium'``         | ``Chromium('chromium')``                |       |
+------------------------+-----------------------------------------+-------+
| ``'chromium-browser'`` | ``Chromium('chromium-browser')``        |       |
+------------------------+-----------------------------------------+-------+
| ``'iosbrowser'``       | ``IOSBrowser``                          | \(4)  |
+------------------------+-----------------------------------------+-------+

Notes:

(1)
   "Konqueror" is the file manager for the KDE desktop environment for Unix, and
   only makes sense to use if KDE is running.  Some way of reliably detecting KDE
   would be nice; the :envvar:`!KDEDIR` variable is not sufficient.  Note also that
   the name "kfm" is used even when using the :program:`konqueror` command with KDE
   2 --- the implementation selects the best strategy for running Konqueror.

(2)
   Only on Windows platforms.

(3)
   Only on macOS.

(4)
   Only on iOS.

.. versionadded:: 3.2
   A new :class:`!MacOSXOSAScript` class has been added
   and is used on Mac instead of the previous :class:`!MacOSX` class.
   This adds support for opening browsers not currently set as the OS default.

.. versionadded:: 3.3
   Support for Chrome/Chromium has been added.

.. versionchanged:: 3.12
   Support for several obsolete browsers has been removed.
   Removed browsers include Grail, Mosaic, Netscape, Galeon,
   Skipstone, Iceape, and Firefox versions 35 and below.

.. versionchanged:: 3.13
   Support for iOS has been added.

Here are some simple examples::

   url = 'https://docs.python.org/'

   # Open URL in a new tab, if a browser window is already open.
   webbrowser.open_new_tab(url)

   # Open URL in new window, raising the window if possible.
   webbrowser.open_new(url)


.. _browser-controllers:

Browser Controller Objects
--------------------------

Browser controllers provide the :attr:`~controller.name` attribute,
and the following three methods which parallel module-level convenience functions:


.. attribute:: controller.name

   System-dependent name for the browser.


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


.. rubric:: Footnotes

.. [1] Executables named here without a full path will be searched in the
       directories given in the :envvar:`PATH` environment variable.
