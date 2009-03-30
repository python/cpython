Building a MacPython distribution
=================================

The ``build-install.py`` script creates MacPython distributions, including
sleepycat db4, sqlite3 and readline support.  It builds a complete 
framework-based Python out-of-tree, installs it in a funny place with 
$DESTROOT, massages that installation to remove .pyc files and such, creates 
an Installer package from the installation plus other files in ``resources`` 
and ``scripts`` and placed that on a ``.dmg`` disk image.

Prerequisites
-------------

* A MacOS X 10.4 (or later)

* XCode 2.2 (or later), with the universal SDK

* No Fink (in ``/sw``) or DarwinPorts (in ``/opt/local``), those could
  interfere with the build.

* The documentation for the release must be available on python.org
  because it is included in the installer.


The Recipe
----------

Here are the steps you need to follow to build a MacPython installer:

*  Run ``./build-installer.py``. Optionally you can pass a number of arguments
   to specify locations of various files. Please see the top of
  ``build-installer.py`` for its usage.

  Running this script takes some time, I will not only build Python itself
  but also some 3th-party libraries that are needed for extensions.

* When done the script will tell you where the DMG image is (by default
  somewhere in ``/tmp/_py``).

Building a 4-way universal installer
....................................

It is also possible to build a 4-way universal installer that runs on 
OSX Leopard or later::

  $ ./build-installer.py --dep-target=10.5 --universal-archs=all --sdk=/

This requires that the deployment target is 10.5 (or later), and hence
also that your building on at least OSX 10.5.

Testing
-------

The resulting binaries should work on MacOSX 10.3.9 or later. I usually run
the installer on a 10.3.9, a 10.4.x PPC and a 10.4.x Intel system and then
run the testsuite to make sure.


Announcements
-------------

(This is mostly of historic interest)

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
