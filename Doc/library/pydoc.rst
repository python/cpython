
:mod:`pydoc` --- Documentation generator and online help system
===============================================================

.. module:: pydoc
   :synopsis: Documentation generator and online help system.
.. moduleauthor:: Ka-Ping Yee <ping@lfw.org>
.. sectionauthor:: Ka-Ping Yee <ping@lfw.org>


.. index::
   single: documentation; generation
   single: documentation; online
   single: help; online

The :mod:`pydoc` module automatically generates documentation from Python
modules.  The documentation can be presented as pages of text on the console,
served to a Web browser, or saved to HTML files.

The built-in function :func:`help` invokes the online help system in the
interactive interpreter, which uses :mod:`pydoc` to generate its documentation
as text on the console.  The same text documentation can also be viewed from
outside the Python interpreter by running :program:`pydoc` as a script at the
operating system's command prompt. For example, running ::

   pydoc sys

at a shell prompt will display documentation on the :mod:`sys` module, in a
style similar to the manual pages shown by the Unix :program:`man` command.  The
argument to :program:`pydoc` can be the name of a function, module, or package,
or a dotted reference to a class, method, or function within a module or module
in a package.  If the argument to :program:`pydoc` looks like a path (that is,
it contains the path separator for your operating system, such as a slash in
Unix), and refers to an existing Python source file, then documentation is
produced for that file.

.. note::

   In order to find objects and their documentation, :mod:`pydoc` imports the
   module(s) to be documented.  Therefore, any code on module level will be
   executed on that occasion.  Use an ``if __name__ == '__main__':`` guard to
   only execute code when a file is invoked as a script and not just imported.

Specifying a :option:`-w` flag before the argument will cause HTML documentation
to be written out to a file in the current directory, instead of displaying text
on the console.

Specifying a :option:`-k` flag before the argument will search the synopsis
lines of all available modules for the keyword given as the argument, again in a
manner similar to the Unix :program:`man` command.  The synopsis line of a
module is the first line of its documentation string.

You can also use :program:`pydoc` to start an HTTP server on the local machine
that will serve documentation to visiting Web browsers. :program:`pydoc`
:option:`-p 1234` will start a HTTP server on port 1234, allowing you to browse
the documentation at ``http://localhost:1234/`` in your preferred Web browser.
:program:`pydoc` :option:`-g` will start the server and additionally bring up a
small :mod:`tkinter`\ -based graphical interface to help you search for
documentation pages.

When :program:`pydoc` generates documentation, it uses the current environment
and path to locate modules.  Thus, invoking :program:`pydoc` :option:`spam`
documents precisely the version of the module you would get if you started the
Python interpreter and typed ``import spam``.

Module docs for core modules are assumed to reside in
``http://docs.python.org/X.Y/library/`` where ``X`` and ``Y`` are the
major and minor version numbers of the Python interpreter.  This can
be overridden by setting the :envvar:`PYTHONDOCS` environment variable
to a different URL or to a local directory containing the Library
Reference Manual pages.

