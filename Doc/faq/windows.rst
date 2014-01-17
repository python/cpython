:tocdepth: 2

.. _windows-faq:

=====================
Python on Windows FAQ
=====================

.. only:: html

   .. contents::

.. XXX need review for Python 3.
   XXX need review for Windows Vista/Seven?


How do I run a Python program under Windows?
--------------------------------------------

This is not necessarily a straightforward question. If you are already familiar
with running programs from the Windows command line then everything will seem
obvious; otherwise, you might need a little more guidance.

.. sidebar:: |Python Development on XP|_
   :subtitle: `Python Development on XP`_

   This series of screencasts aims to get you up and running with Python on
   Windows XP.  The knowledge is distilled into 1.5 hours and will get you up
   and running with the right Python distribution, coding in your choice of IDE,
   and debugging and writing solid code with unit-tests.

.. |Python Development on XP| image:: python-video-icon.png
.. _`Python Development on XP`:
   http://www.showmedo.com/videos/series?name=pythonOzsvaldPyNewbieSeries

Unless you use some sort of integrated development environment, you will end up
*typing* Windows commands into what is variously referred to as a "DOS window"
or "Command prompt window".  Usually you can create such a window from your
Start menu; under Windows 7 the menu selection is :menuselection:`Start -->
Programs --> Accessories --> Command Prompt`.  You should be able to recognize
when you have started such a window because you will see a Windows "command
prompt", which usually looks like this::

   C:\>

The letter may be different, and there might be other things after it, so you
might just as easily see something like::

   D:\YourName\Projects\Python>

depending on how your computer has been set up and what else you have recently
done with it.  Once you have started such a window, you are well on the way to
running Python programs.

You need to realize that your Python scripts have to be processed by another
program called the Python *interpreter*.  The interpreter reads your script,
compiles it into bytecodes, and then executes the bytecodes to run your
program. So, how do you arrange for the interpreter to handle your Python?

First, you need to make sure that your command window recognises the word
"python" as an instruction to start the interpreter.  If you have opened a
command window, you should try entering the command ``python`` and hitting
return.::

   C:\Users\YourName> python

You should then see something like::

   Python 3.3.0 (v3.3.0:bd8afb90ebf2, Sep 29 2012, 10:55:48) [MSC v.1600 32 bit (Intel)] on win32
   Type "help", "copyright", "credits" or "license" for more information.
   >>>

You have started the interpreter in "interactive mode". That means you can enter
Python statements or expressions interactively and have them executed or
evaluated while you wait.  This is one of Python's strongest features.  Check it
by entering a few expressions of your choice and seeing the results::

    >>> print("Hello")
    Hello
    >>> "Hello" * 3
    HelloHelloHello

Many people use the interactive mode as a convenient yet highly programmable
calculator.  When you want to end your interactive Python session, hold the Ctrl
key down while you enter a Z, then hit the "Enter" key to get back to your
Windows command prompt.

You may also find that you have a Start-menu entry such as :menuselection:`Start
--> Programs --> Python 3.3 --> Python (command line)` that results in you
seeing the ``>>>`` prompt in a new window.  If so, the window will disappear
after you enter the Ctrl-Z character; Windows is running a single "python"
command in the window, and closes it when you terminate the interpreter.

If the ``python`` command, instead of displaying the interpreter prompt ``>>>``,
gives you a message like::

   'python' is not recognized as an internal or external command, operable program or batch file.

.. sidebar:: |Adding Python to DOS Path|_
   :subtitle: `Adding Python to DOS Path`_

   Python is not added to the DOS path by default.  This screencast will walk
   you through the steps to add the correct entry to the `System Path`, allowing
   Python to be executed from the command-line by all users.

.. |Adding Python to DOS Path| image:: python-video-icon.png
.. _`Adding Python to DOS Path`:
   http://showmedo.com/videos/video?name=960000&fromSeriesID=96


or::

   Bad command or filename

then you need to make sure that your computer knows where to find the Python
interpreter.  To do this you will have to modify a setting called PATH, which is
a list of directories where Windows will look for programs.

You should arrange for Python's installation directory to be added to the PATH
of every command window as it starts.  If you installed Python fairly recently
then the command ::

   dir C:\py*

will probably tell you where it is installed; the usual location is something
like ``C:\Python33``.  Otherwise you will be reduced to a search of your whole
disk ... use :menuselection:`Tools --> Find` or hit the :guilabel:`Search`
button and look for "python.exe".  Supposing you discover that Python is
installed in the ``C:\Python33`` directory (the default at the time of writing),
you should make sure that entering the command ::

   c:\Python33\python

starts up the interpreter as above (and don't forget you'll need a "CTRL-Z" and
an "Enter" to get out of it). Once you have verified the directory, you can
add it to the system path to make it easier to start Python by just running
the ``python`` command. This is currently an option in the installer as of
CPython 3.3.

More information about environment variables can be found on the
:ref:`Using Python on Windows <setting-envvars>` page.

How do I make Python scripts executable?
----------------------------------------

On Windows, the standard Python installer already associates the .py
extension with a file type (Python.File) and gives that file type an open
command that runs the interpreter (``D:\Program Files\Python\python.exe "%1"
%*``).  This is enough to make scripts executable from the command prompt as
'foo.py'.  If you'd rather be able to execute the script by simple typing 'foo'
with no extension you need to add .py to the PATHEXT environment variable.

Why does Python sometimes take so long to start?
------------------------------------------------

Usually Python starts very quickly on Windows, but occasionally there are bug
reports that Python suddenly begins to take a long time to start up.  This is
made even more puzzling because Python will work fine on other Windows systems
which appear to be configured identically.

The problem may be caused by a misconfiguration of virus checking software on
the problem machine.  Some virus scanners have been known to introduce startup
overhead of two orders of magnitude when the scanner is configured to monitor
all reads from the filesystem.  Try checking the configuration of virus scanning
software on your systems to ensure that they are indeed configured identically.
McAfee, when configured to scan all file system read activity, is a particular
offender.


How do I make an executable from a Python script?
-------------------------------------------------

See http://cx-freeze.sourceforge.net/ for a distutils extension that allows you
to create console and GUI executables from Python code.
`py2exe <http://www.py2exe.org/>`_, the most popular extension for building
Python 2.x-based executables, does not yet support Python 3 but a version that
does is in development.


Is a ``*.pyd`` file the same as a DLL?
--------------------------------------

Yes, .pyd files are dll's, but there are a few differences.  If you have a DLL
named ``foo.pyd``, then it must have a function ``PyInit_foo()``.  You can then
write Python "import foo", and Python will search for foo.pyd (as well as
foo.py, foo.pyc) and if it finds it, will attempt to call ``PyInit_foo()`` to
initialize it.  You do not link your .exe with foo.lib, as that would cause
Windows to require the DLL to be present.

Note that the search path for foo.pyd is PYTHONPATH, not the same as the path
that Windows uses to search for foo.dll.  Also, foo.pyd need not be present to
run your program, whereas if you linked your program with a dll, the dll is
required.  Of course, foo.pyd is required if you want to say ``import foo``.  In
a DLL, linkage is declared in the source code with ``__declspec(dllexport)``.
In a .pyd, linkage is defined in a list of available functions.


How can I embed Python into a Windows application?
--------------------------------------------------

Embedding the Python interpreter in a Windows app can be summarized as follows:

1. Do _not_ build Python into your .exe file directly.  On Windows, Python must
   be a DLL to handle importing modules that are themselves DLL's.  (This is the
   first key undocumented fact.)  Instead, link to :file:`python{NN}.dll`; it is
   typically installed in ``C:\Windows\System``.  *NN* is the Python version, a
   number such as "33" for Python 3.3.

   You can link to Python in two different ways.  Load-time linking means
   linking against :file:`python{NN}.lib`, while run-time linking means linking
   against :file:`python{NN}.dll`.  (General note: :file:`python{NN}.lib` is the
   so-called "import lib" corresponding to :file:`python{NN}.dll`.  It merely
   defines symbols for the linker.)

   Run-time linking greatly simplifies link options; everything happens at run
   time.  Your code must load :file:`python{NN}.dll` using the Windows
   ``LoadLibraryEx()`` routine.  The code must also use access routines and data
   in :file:`python{NN}.dll` (that is, Python's C API's) using pointers obtained
   by the Windows ``GetProcAddress()`` routine.  Macros can make using these
   pointers transparent to any C code that calls routines in Python's C API.

   Borland note: convert :file:`python{NN}.lib` to OMF format using Coff2Omf.exe
   first.

   .. XXX what about static linking?

2. If you use SWIG, it is easy to create a Python "extension module" that will
   make the app's data and methods available to Python.  SWIG will handle just
   about all the grungy details for you.  The result is C code that you link
   *into* your .exe file (!)  You do _not_ have to create a DLL file, and this
   also simplifies linking.

3. SWIG will create an init function (a C function) whose name depends on the
   name of the extension module.  For example, if the name of the module is leo,
   the init function will be called initleo().  If you use SWIG shadow classes,
   as you should, the init function will be called initleoc().  This initializes
   a mostly hidden helper class used by the shadow class.

   The reason you can link the C code in step 2 into your .exe file is that
   calling the initialization function is equivalent to importing the module
   into Python! (This is the second key undocumented fact.)

4. In short, you can use the following code to initialize the Python interpreter
   with your extension module.

   .. code-block:: c

      #include "python.h"
      ...
      Py_Initialize();  // Initialize Python.
      initmyAppc();  // Initialize (import) the helper class.
      PyRun_SimpleString("import myApp");  // Import the shadow class.

5. There are two problems with Python's C API which will become apparent if you
   use a compiler other than MSVC, the compiler used to build pythonNN.dll.

   Problem 1: The so-called "Very High Level" functions that take FILE *
   arguments will not work in a multi-compiler environment because each
   compiler's notion of a struct FILE will be different.  From an implementation
   standpoint these are very _low_ level functions.

   Problem 2: SWIG generates the following code when generating wrappers to void
   functions:

   .. code-block:: c

      Py_INCREF(Py_None);
      _resultobj = Py_None;
      return _resultobj;

   Alas, Py_None is a macro that expands to a reference to a complex data
   structure called _Py_NoneStruct inside pythonNN.dll.  Again, this code will
   fail in a mult-compiler environment.  Replace such code by:

   .. code-block:: c

      return Py_BuildValue("");

   It may be possible to use SWIG's ``%typemap`` command to make the change
   automatically, though I have not been able to get this to work (I'm a
   complete SWIG newbie).

6. Using a Python shell script to put up a Python interpreter window from inside
   your Windows app is not a good idea; the resulting window will be independent
   of your app's windowing system.  Rather, you (or the wxPythonWindow class)
   should create a "native" interpreter window.  It is easy to connect that
   window to the Python interpreter.  You can redirect Python's i/o to _any_
   object that supports read and write, so all you need is a Python object
   (defined in your extension module) that contains read() and write() methods.

How do I keep editors from inserting tabs into my Python source?
----------------------------------------------------------------

The FAQ does not recommend using tabs, and the Python style guide, :pep:`8`,
recommends 4 spaces for distributed Python code; this is also the Emacs
python-mode default.

Under any editor, mixing tabs and spaces is a bad idea.  MSVC is no different in
this respect, and is easily configured to use spaces: Take :menuselection:`Tools
--> Options --> Tabs`, and for file type "Default" set "Tab size" and "Indent
size" to 4, and select the "Insert spaces" radio button.

If you suspect mixed tabs and spaces are causing problems in leading whitespace,
run Python with the :option:`-t` switch or run ``Tools/Scripts/tabnanny.py`` to
check a directory tree in batch mode.


How do I check for a keypress without blocking?
-----------------------------------------------

Use the msvcrt module.  This is a standard Windows-specific extension module.
It defines a function ``kbhit()`` which checks whether a keyboard hit is
present, and ``getch()`` which gets one character without echoing it.


How do I emulate os.kill() in Windows?
--------------------------------------

Prior to Python 2.7 and 3.2, to terminate a process, you can use :mod:`ctypes`::

   import ctypes

   def kill(pid):
       """kill function for Win32"""
       kernel32 = ctypes.windll.kernel32
       handle = kernel32.OpenProcess(1, 0, pid)
       return (0 != kernel32.TerminateProcess(handle, 0))

In 2.7 and 3.2, :func:`os.kill` is implemented similar to the above function,
with the additional feature of being able to send CTRL+C and CTRL+BREAK
to console subprocesses which are designed to handle those signals. See
:func:`os.kill` for further details.

How do I extract the downloaded documentation on Windows?
---------------------------------------------------------

Sometimes, when you download the documentation package to a Windows machine
using a web browser, the file extension of the saved file ends up being .EXE.
This is a mistake; the extension should be .TGZ.

Simply rename the downloaded file to have the .TGZ extension, and WinZip will be
able to handle it.  (If your copy of WinZip doesn't, get a newer one from
http://www.winzip.com.)

