The IDLE icons are from https://bugs.python.org/issue1490384

Created by Andrew Clover.

The original sources are available from Andrew's website:
https://www.doxdesk.com/software/py/pyicons.html

Various different formats and sizes are available at this GitHub Pull Request:
https://github.com/python/cpython/pull/17473

The idle.ico file was created with ImageMagick:

    $ convert idle_16.png idle_32.png idle_48.png idle_256.png idle.ico

The idle.icns file was created by (no 48 pixel icon is supported):

    $ png2icns idle.icns idle_16.png idle_32.png idle_256.png

png2icns from http://icns.sourceforge.net/
