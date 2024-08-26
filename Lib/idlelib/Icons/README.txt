2006?: Andrew Clover made the 16-, 32-, and 48-bit icons for win23.
https://www.doxdesk.com/software/py/pyicons.html
(TJR: Not sure if this originally included 256-bit image.)

2006: They were copied to CPython as Python application icons.
https://github.com/python/cpython/issues/43372
(TJR: I do not remember where put in CPython source tree.)

2014: They were copied (perhaps a bit revised) to idlelib/Icons.
https://github.com/python/cpython/issues/64605

2020: Add Clover's 256-bit image.
https://github.com/python/cpython/issues/82620

The idle.ico file used for Windows was created with ImageMagick:
  $ convert idle_16.png idle_32.png idle_48.png idle_256.png idle.ico
** This needs redoing whenever files are changed.
?? Do Start, Desktop, and Taskbar use idlelib/Icons files?

Issue added Windows Store PC/icons/idlex44.png and .../idlex150.png.
https://github.com/python/cpython/pull/22817
** These should also be updated with major changes.

2022: Optimize .png images in CPython repository.
https://github.com/python/cpython/pull/21348
idle.ico (and idlex##) were not updated.

The idlexx.gif files are only needed for *nix running tcl/tk 8.5.
As of 2022, this was known true for 1 'major' Linux distribution.
(Same would be true for any non-Aqua macOS with 8.5, but must be none.)

The other .gifs are used by browsers using idlelib.tree.  At least some
will not be used when tree is replaced by ttk.Treeview.


Edited 2024 August 25 by TJR.
