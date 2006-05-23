Building a MacPython distribution
=================================

The ``build-install.py`` script creates MacPython distributions, including
sleepycat db4, sqlite3 and readline support.  It builds a complete 
framework-based Python out-of-tree, installs it in a funny place with 
$DESTROOT, massages that installation to remove .pyc files and such, creates 
an Installer package from the installation plus other files in ``resources`` 
and ``scripts`` and placed that on a ``.dmg`` disk image.

Here are the steps you ned to follow to build a MacPython installer:

- Run ``./build-installer.py``. Optionally you can pass a number of arguments
  to specify locations of various files. Please see the top of
  ``build-installer.py`` for its usage.
- When done the script will tell you where the DMG image is.

The script needs to be run on Mac OS X 10.4 with Xcode 2.2 or later and
the 10.4u SDK.

When all is done, announcements can be posted to at least the following
places:
-   pythonmac-sig@python.org
-   python-dev@python.org
-   python-announce@python.org
-   archivist@info-mac.org
-   adcnews@apple.com
-   news@macnn.com
-   http://www.macupdate.com
-   http://guide.apple.com/usindex.lasso
-   http://www.apple.com/downloads/macosx/submit
-   http://www.versiontracker.com/ (userid Jack.Jansen@oratrix.com)
-   http://www.macshareware.net (userid jackjansen)

Also, check out Stephan Deibels http://pythonology.org/market contact list
