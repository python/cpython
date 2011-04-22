.. _examples:

********
Examples
********

This chapter provides a number of basic examples to help get started with
distutils.  Additional information about using distutils can be found in the
Distutils Cookbook.


.. seealso::

   `Distutils Cookbook <http://wiki.python.org/moin/Distutils/Cookbook>`_
      Collection of recipes showing how to achieve more control over distutils.


.. _pure-mod:

Pure Python distribution (by module)
====================================

If you're just distributing a couple of modules, especially if they don't live
in a particular package, you can specify them individually using the
:option:`py_modules` option in the setup script.

In the simplest case, you'll have two files to worry about: a setup script and
the single module you're distributing, :file:`foo.py` in this example::

   <root>/
           setup.py
           foo.py

(In all diagrams in this section, *<root>* will refer to the distribution root
directory.)  A minimal setup script to describe this situation would be::

   from distutils.core import setup
   setup(name='foo',
         version='1.0',
         py_modules=['foo'],
         )

Note that the name of the distribution is specified independently with the
:option:`name` option, and there's no rule that says it has to be the same as
the name of the sole module in the distribution (although that's probably a good
convention to follow).  However, the distribution name is used to generate
filenames, so you should stick to letters, digits, underscores, and hyphens.

Since :option:`py_modules` is a list, you can of course specify multiple
modules, eg. if you're distributing modules :mod:`foo` and :mod:`bar`, your
setup might look like this::

   <root>/
           setup.py
           foo.py
           bar.py

and the setup script might be  ::

   from distutils.core import setup
   setup(name='foobar',
         version='1.0',
         py_modules=['foo', 'bar'],
         )

You can put module source files into another directory, but if you have enough
modules to do that, it's probably easier to specify modules by package rather
than listing them individually.


.. _pure-pkg:

Pure Python distribution (by package)
=====================================

If you have more than a couple of modules to distribute, especially if they are
in multiple packages, it's probably easier to specify whole packages rather than
individual modules.  This works even if your modules are not in a package; you
can just tell the Distutils to process modules from the root package, and that
works the same as any other package (except that you don't have to have an
:file:`__init__.py` file).

The setup script from the last example could also be written as  ::

   from distutils.core import setup
   setup(name='foobar',
         version='1.0',
         packages=[''],
         )

(The empty string stands for the root package.)

If those two files are moved into a subdirectory, but remain in the root
package, e.g.::

   <root>/
           setup.py
           src/      foo.py
                     bar.py

then you would still specify the root package, but you have to tell the
Distutils where source files in the root package live::

   from distutils.core import setup
   setup(name='foobar',
         version='1.0',
         package_dir={'': 'src'},
         packages=[''],
         )

More typically, though, you will want to distribute multiple modules in the same
package (or in sub-packages).  For example, if the :mod:`foo`  and :mod:`bar`
modules belong in package :mod:`foobar`, one way to layout your source tree is
::

   <root>/
           setup.py
           foobar/
                    __init__.py
                    foo.py
                    bar.py

This is in fact the default layout expected by the Distutils, and the one that
requires the least work to describe in your setup script::

   from distutils.core import setup
   setup(name='foobar',
         version='1.0',
         packages=['foobar'],
         )

If you want to put modules in directories not named for their package, then you
need to use the :option:`package_dir` option again.  For example, if the
:file:`src` directory holds modules in the :mod:`foobar` package::

   <root>/
           setup.py
           src/
                    __init__.py
                    foo.py
                    bar.py

an appropriate setup script would be  ::

   from distutils.core import setup
   setup(name='foobar',
         version='1.0',
         package_dir={'foobar': 'src'},
         packages=['foobar'],
         )

Or, you might put modules from your main package right in the distribution
root::

   <root>/
           setup.py
           __init__.py
           foo.py
           bar.py

in which case your setup script would be  ::

   from distutils.core import setup
   setup(name='foobar',
         version='1.0',
         package_dir={'foobar': ''},
         packages=['foobar'],
         )

(The empty string also stands for the current directory.)

If you have sub-packages, they must be explicitly listed in :option:`packages`,
but any entries in :option:`package_dir` automatically extend to sub-packages.
(In other words, the Distutils does *not* scan your source tree, trying to
figure out which directories correspond to Python packages by looking for
:file:`__init__.py` files.)  Thus, if the default layout grows a sub-package::

   <root>/
           setup.py
           foobar/
                    __init__.py
                    foo.py
                    bar.py
                    subfoo/
                              __init__.py
                              blah.py

then the corresponding setup script would be  ::

   from distutils.core import setup
   setup(name='foobar',
         version='1.0',
         packages=['foobar', 'foobar.subfoo'],
         )

(Again, the empty string in :option:`package_dir` stands for the current
directory.)


.. _single-ext:

Single extension module
=======================

Extension modules are specified using the :option:`ext_modules` option.
:option:`package_dir` has no effect on where extension source files are found;
it only affects the source for pure Python modules.  The simplest  case, a
single extension module in a single C source file, is::

   <root>/
           setup.py
           foo.c

If the :mod:`foo` extension belongs in the root package, the setup script for
this could be  ::

   from distutils.core import setup
   from distutils.extension import Extension
   setup(name='foobar',
         version='1.0',
         ext_modules=[Extension('foo', ['foo.c'])],
         )

If the extension actually belongs in a package, say :mod:`foopkg`, then

With exactly the same source tree layout, this extension can be put in the
:mod:`foopkg` package simply by changing the name of the extension::

   from distutils.core import setup
   from distutils.extension import Extension
   setup(name='foobar',
         version='1.0',
         ext_modules=[Extension('foopkg.foo', ['foo.c'])],
         )

Checking a package
==================

The ``check`` command allows you to verify if your package meta-data
meet the minimum requirements to build a distribution.

To run it, just call it using your :file:`setup.py` script. If something is
missing, ``check`` will display a warning.

Let's take an example with a simple script::

    from distutils.core import setup

    setup(name='foobar')

Running the ``check`` command will display some warnings::

    $ python setup.py check
    running check
    warning: check: missing required meta-data: version, url
    warning: check: missing meta-data: either (author and author_email) or
             (maintainer and maintainer_email) must be supplied


If you use the reStructuredText syntax in the ``long_description`` field and
`docutils <http://docutils.sourceforge.net/>`_ is installed you can check if
the syntax is fine with the ``check`` command, using the ``restructuredtext``
option.

For example, if the :file:`setup.py` script is changed like this::

    from distutils.core import setup

    desc = """\
    My description
    =============

    This is the description of the ``foobar`` package.
    """

    setup(name='foobar', version='1', author='tarek',
        author_email='tarek@ziade.org',
        url='http://example.com', long_description=desc)

Where the long description is broken, ``check`` will be able to detect it
by using the :mod:`docutils` parser::

    $ python setup.py check --restructuredtext
    running check
    warning: check: Title underline too short. (line 2)
    warning: check: Could not finish the parsing.

.. % \section{Multiple extension modules}
.. % \label{multiple-ext}

.. % \section{Putting it all together}


