Building a MacPython distribution
=================================

The ``build`` shell script here creates MacPython distributions.
It builds a complete framework-based Python out-of-tree, installs
it in a funny place with $DESTROOT, massages that installation to remove
.pyc files and such, creates an Installer package from the installation
plus the stuff in ``resources`` and compresses that installer as a
``.dmg`` disk image.

Here are the steps you ned to follow to build a MacPython installer:

- There are various version numbers that need to be updated. Weed through
  ``Mac/OSXResources``, ``Mac/scripts`` and ``Mac/Tools`` and inspect the
  various ``.plist`` and ``.strings`` files. Note that the latter are
  UTF-16 files.
- Edit ``resource/ReadMe.txt`` and ``resources/Welcome.rtf`` to reflect
  version number and such.
- Edit ``build`` to change ``PYVERSION``, ``PYVER`` and ``BUILDNUM``.
- Run ``./build``. Optionally you can pass the name of the directory
  where Python will be built, so you don't have to wait for the complete
  build when you're debugging the process. For the final distribution use
  a clean build.
- When done the script will tell you where the DMG image is.

Currently (November 2003) there is still a bug in the build procedure
for $DESTROOT builds: building some of the applets will fail (in
``Mac/OSX/Makefile``) if you don't have the same version of Python installed
normally. So before doing the distribution you should build and install
a framework Python in the normal way.

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

After all this is done you may also need to update the Package Manager
database for the new distribution. A description of this remains TBD.
