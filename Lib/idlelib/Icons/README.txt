IDLE-PYTHON LOGOS

These are sent to tk on Windows, *NIX, and non-Aqua macOS
in pyshell following "# set application icon".


2006?: Andrew Clover made variously sized python icons for win23.
https://www.doxdesk.com/software/py/pyicons.html

2006: 16, 32, and 48 bit .png versions were copied to CPython
as Python application icons, maybe in PC/icons/py.ico.
https://github.com/python/cpython/issues/43372

2014: They were copied (perhaps a bit revised) to idlelib/Icons.
https://github.com/python/cpython/issues/64605
.gif versions were also added.

2020: Add Clover's 256-bit image.
https://github.com/python/cpython/issues/82620
Other fixups were done.

The idle.ico file used for Windows was created with ImageMagick:
  $ convert idle_16.png idle_32.png idle_48.png idle_256.png idle.ico
** This needs redoing whenever files are changed.
?? Do Start, Desktop, and Taskbar use idlelib/Icons files?

Issue added Windows Store PC/icons/idlex44.png and .../idlex150.png.
https://github.com/python/cpython/pull/22817
?? Should these be updated with major changes?

2022: Optimize .png images in CPython repository with external program.
https://github.com/python/cpython/pull/21348
idle.ico (and idlex##) were not updated.

The idlexx.gif files are only needed for *nix running tcl/tk 8.5.
As of 2022, this was known true for 1 'major' Linux distribution.
(Same would be true for any non-Aqua macOS with 8.5, but now none?)
Can be deleted when we require 8.6 or it is known always used.

Future: Derivatives of Python logo should be submitted for approval.
PSF Trademark Working Group / Committee psf-trademarks@python.org
https://www.python.org/community/logos/  # Original files
https://www.python.org/psf/trademarks-faq/
https://www.python.org/psf/trademarks/  # Usage.


OTHER GIFS: These are used by browsers using idlelib.tree.
At least some will not be used when tree is replaced by ttk.Treeview.


Edited 2024 August 26 by TJR.
