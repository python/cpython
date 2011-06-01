.. _packaging-examples:

********
Examples
********

This chapter provides a number of basic examples to help get started with
Packaging.


.. _packaging-pure-mod:

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

   from packaging.core import setup
   setup(name='foo',
         version='1.0',
         py_modules=['foo'])

Note that the name of the distribution is specified independently with the
:option:`name` option, and there's no rule that says it has to be the same as
the name of the sole module in the distribution (although that's probably a good
convention to follow).  However, the distribution name is used to generate
filenames, so you should stick to letters, digits, underscores, and hyphens.

Since :option:`py_modules` is a list, you can of course specify multiple
modules, e.g. if you're distributing modules :mod:`foo` and :mod:`bar`, your
setup might look like this::

   <root>/
          setup.py
          foo.py
          bar.py

and the setup script might be  ::

   from packaging.core import setup
   setup(name='foobar',
         version='1.0',
         py_modules=['foo', 'bar'])

You can put module source files into another directory, but if you have enough
modules to do that, it's probably easier to specify modules by package rather
than listing them individually.


.. _packaging-pure-pkg:

Pure Python distribution (by package)
=====================================

If you have more than a couple of modules to distribute, especially if they are
in multiple packages, it's probably easier to specify whole packages rather than
individual modules.  This works even if your modules are not in a package; you
can just tell the Distutils to process modules from the root package, and that
works the same as any other package (except that you don't have to have an
:file:`__init__.py` file).

The setup script from the last example could also be written as  ::

   from packaging.core import setup
   setup(name='foobar',
         version='1.0',
         packages=[''])

(The empty string stands for the root package.)

If those two files are moved into a subdirectory, but remain in the root
package, e.g.::

   <root>/
          setup.py
          src/
              foo.py
              bar.py

then you would still specify the root package, but you have to tell the
Distutils where source files in the root package live::

   from packaging.core import setup
   setup(name='foobar',
         version='1.0',
         package_dir={'': 'src'},
         packages=[''])

More typically, though, you will want to distribute multiple modules in the same
package (or in sub-packages).  For example, if the :mod:`foo`  and :mod:`bar`
modules belong in package :mod:`foobar`, one way to lay out your source tree is

::

   <root>/
          setup.py
          foobar/
                 __init__.py
                 foo.py
                 bar.py

This is in fact the default layout expected by the Distutils, and the one that
requires the least work to describe in your setup script::

   from packaging.core import setup
   setup(name='foobar',
         version='1.0',
         packages=['foobar'])

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

   from packaging.core import setup
   setup(name='foobar',
         version='1.0',
         package_dir={'foobar': 'src'},
         packages=['foobar'])

Or, you might put modules from your main package right in the distribution
root::

   <root>/
          setup.py
          __init__.py
          foo.py
          bar.py

in which case your setup script would be  ::

   from packaging.core import setup
   setup(name='foobar',
         version='1.0',
         package_dir={'foobar': ''},
         packages=['foobar'])

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

   from packaging.core import setup
   setup(name='foobar',
         version='1.0',
         packages=['foobar', 'foobar.subfoo'])

(Again, the empty string in :option:`package_dir` stands for the current
directory.)


.. _packaging-single-ext:

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

   from packaging.core import setup, Extension
   setup(name='foobar',
         version='1.0',
         ext_modules=[Extension('foo', ['foo.c'])])

If the extension actually belongs in a package, say :mod:`foopkg`, then

With exactly the same source tree layout, this extension can be put in the
:mod:`foopkg` package simply by changing the name of the extension::

   from packaging.core import setup, Extension
   setup(name='foobar',
         version='1.0',
         packages=['foopkg'],
         ext_modules=[Extension('foopkg.foo', ['foo.c'])])


Checking metadata
=================

The ``check`` command allows you to verify if your project's metadata
meets the minimum requirements to build a distribution.

To run it, just call it using your :file:`setup.py` script. If something is
missing, ``check`` will display a warning.

Let's take an example with a simple script::

    from packaging.core import setup

    setup(name='foobar')

.. TODO configure logging StreamHandler to match this output

Running the ``check`` command will display some warnings::

    $ python setup.py check
    running check
    warning: check: missing required metadata: version, home_page
    warning: check: missing metadata: either (author and author_email) or
             (maintainer and maintainer_email) must be supplied


If you use the reStructuredText syntax in the ``long_description`` field and
`Docutils <http://docutils.sourceforge.net/>`_ is installed you can check if
the syntax is fine with the ``check`` command, using the ``restructuredtext``
option.

For example, if the :file:`setup.py` script is changed like this::

    from packaging.core import setup

    desc = """\
    Welcome to foobar!
    ===============

    This is the description of the ``foobar`` project.
    """

    setup(name='foobar',
          version='1.0',
          author=u'Tarek Ziadé',
          author_email='tarek@ziade.org',
          summary='Foobar utilities'
          description=desc,
          home_page='http://example.com')

Where the long description is broken, ``check`` will be able to detect it
by using the :mod:`docutils` parser::

    $ python setup.py check --restructuredtext
    running check
    warning: check: Title underline too short. (line 2)
    warning: check: Could not finish the parsing.


.. _packaging-reading-metadata:

Reading the metadata
====================

The :func:`packaging.core.setup` function provides a command-line interface
that allows you to query the metadata fields of a project through the
:file:`setup.py` script of a given project::

    $ python setup.py --name
    foobar

This call reads the ``name`` metadata by running the
:func:`packaging.core.setup`  function. When a source or binary
distribution is created with Distutils, the metadata fields are written
in a static file called :file:`PKG-INFO`. When a Distutils-based project is
installed in Python, the :file:`PKG-INFO` file is copied alongside the modules
and packages of the distribution under :file:`NAME-VERSION-pyX.X.egg-info`,
where ``NAME`` is the name of the project, ``VERSION`` its version as defined
in the Metadata, and ``pyX.X`` the major and minor version of Python like
``2.7`` or ``3.2``.

You can read back this static file, by using the
:class:`packaging.dist.Metadata` class and its
:func:`read_pkg_file` method::

    >>> from packaging.metadata import Metadata
    >>> metadata = Metadata()
    >>> metadata.read_pkg_file(open('distribute-0.6.8-py2.7.egg-info'))
    >>> metadata.name
    'distribute'
    >>> metadata.version
    '0.6.8'
    >>> metadata.description
    'Easily download, build, install, upgrade, and uninstall Python packages'

Notice that the class can also be instantiated with a metadata file path to
loads its values::

    >>> pkg_info_path = 'distribute-0.6.8-py2.7.egg-info'
    >>> Metadata(pkg_info_path).name
    'distribute'


.. XXX These comments have been here for at least ten years. Write the
       sections or delete the comments (we can maybe ask Greg Ward about
       the planned contents). (Unindent to make them section titles)

    .. multiple-ext::

       Multiple extension modules
       ==========================

       Putting it all together
       =======================
