--- README.macosx ---

[1] You need to build Python for macosx. The latest release (2.1b2)
   builds out of the box for macosx -- use:
	configure --with-dyld --with-suffix=.x
   ( get the latest version -- you'll have to fix the build process
     to get earlier versions to build.)


[2] You need a copy of Apple's Universal Interfaces CIncludes.

  The Carbon.framework include files on macosx are not as backwards
  compatible as the ones in Universal Interfaces. For example 
  UI has a Windows.h file that includes MacWindows.h, while 
  Carbon.framework only has MacWindows.h 
  ( actually, it's: HIToolbox.framework/Headers/MacWindows.h )

  Until macpython sources are converted to be more Carbon compliant
  you need to get a copy of Universal Interfaces (downloadable from
  Apple's web site if you don't have a copy. If you have Metrowerks
  Compiler, there is probably a copy on your CD. ) 


[3] If your are building from patches, you need to download the cvs
    distribution and apply the patches. ( The "If" is there because I
    may eventually package this up in a separate patched distribution.)

 cvs -d:pserver:ropython@pythoncvs.oratrix.nl:/hosts/mm/CVSREMOTE login
 Password: geheim

 cvs -d:pserver:ropython@pythoncvs.oratrix.nl:/hosts/mm/CVSREMOTE co python/Mac

    The normal macpython distributions have classic-mac line endings.
    macosx gnu tools want unix line endings. Getting sources from
    the cvs server avoids having to do a bunch of conversions on 
    the source files.


    New files (go in python/Mac/Modules/ directory)
	README.macosx (this file)
	Makefile  (makefile also creates a dummy .h file)
	Carbonmodule.c

    Patched files:
	python/Mac/Python/macglue.c
	python/Mac/Modules/Win/Winmodule.c


[4]  There are two lines you have to edit in the Makefile to point to
     Universal Interfaces CIncludes  and the Python source distribution.
     (eventually, disutils will be used to find the Python sources.
      eventually, macpython sources will use the more modern headers.)


# Point this to the directory with the Universal Interfaces CIncludes:
UINCLUDE=	/Local/Carbon/Universal.Interfaces/CIncludes/

# Point this to your python source directory:
PYTHONHOME=	/Users/sdm7g/Src/Python-2.1a2/

	After changing those lines, you can type "make" .

    "-undefined warning" is used rather than "-undefined supress" --
    and there are always some undefined symbols that are resolved 
    in the main python module. If there is something *REALLY* 
    unresolved (or often the problem is multiple definitions) 
    you will get an error on importing Carbonmodule.so. 

[5] Installation is manual for now.
   If you want to run it out of the python/Mac/Modules directory, you
    need to copy it to /usr/local/lib/python2.1/site-packages/ 
    ( or whatever is the appropriate directory on your machine )


[6] All of the toolbox modules are built into a single container module
   named Carbon. All of the other modules are in the Carbon namespace:
    i.e. Carbon.Win. You can do a "from Carbon import *", however all
    modules get put into sys.modules under their own name, so once 
    Carbon has been imported, statements like "import Win" will work --
     there is no Winmodule.so file, but import will find 'Win' in the
    sys.modules cache before it tries to match a filename. 


These are the modules currently linked:

Python 2.1a2 (#1, 02/12/01, 19:49:54) 
[GCC Apple DevKit-based CPP 5.0] on Darwin1.2
Type "copyright", "credits" or "license" for more information.
>>> import Carbon
>>> dir(Carbon)
['AE', 'App', 'Cm', 'ColorPicker', 'Ctl', 'Dlg', 'Drag', 'Evt', 'Fm', 'HtmlRender', 'Icn', 'List', 'Menu', 'Qd', 'Qdoffs', 'Res', 'Scrap', 'Snd', 'TE', 'Win', '__doc__', '__file__', '__name__', 'macfs']
>>> 

A simple test:

>>> import Snd
>>> Snd.SysBeep( 100 )

The main one missing is macosmodule, which has some functions for handling
resource forks of files, file types and creators, and how and whether
MacPython handles events or your script does. Some of these functions 
require others in Python/macmain.c, which may drag in other symbols, either
undefined or multiply defined ( because there is a unix-Python implementation
parallel to the macpython one. ) It's likely that we will need (or at least
want) new macosx implementations of some of these functions. 

If you're interested in trying to add them, there is an additional target
in the Makefile "make Experimental", which compiles and links with the 
$(XXX) modules:  currently just macosmodule.c  and macmain.c -- you 
can add others to try to resolve undefined symbols or edit the files 
to remove some symbols or functions. 

-- Steve Majewski <sdm7g@Virginia.EDU> 



