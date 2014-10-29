.. highlightlang:: c


.. _building:

********************************************
Building C and C++ Extensions with distutils
********************************************

.. sectionauthor:: Martin v. Löwis <martin@v.loewis.de>


Starting in Python 1.4, Python provides, on Unix, a special make file for
building make files for building dynamically-linked extensions and custom
interpreters.  Starting with Python 2.0, this mechanism (known as related to
Makefile.pre.in, and Setup files) is no longer supported. Building custom
interpreters was rarely used, and extension modules can be built using
distutils.

Building an extension module using distutils requires that distutils is
installed on the build machine, which is included in Python 2.x and available
separately for Python 1.5. Since distutils also supports creation of binary
packages, users don't necessarily need a compiler and distutils to install the
extension.

A distutils package contains a driver script, :file:`setup.py`. This is a plain
Python file, which, in the most simple case, could look like this::

   from distutils.core import setup, Extension

   module1 = Extension('demo',
                       sources = ['demo.c'])

   setup (name = 'PackageName',
          version = '1.0',
          description = 'This is a demo package',
          ext_modules = [module1])


With this :file:`setup.py`, and a file :file:`demo.c`, running ::

   python setup.py build

will compile :file:`demo.c`, and produce an extension module named ``demo`` in
the :file:`build` directory. Depending on the system, the module file will end
up in a subdirectory :file:`build/lib.system`, and may have a name like
:file:`demo.so` or :file:`demo.pyd`.

In the :file:`setup.py`, all execution is performed by calling the ``setup``
function. This takes a variable number of keyword arguments, of which the
example above uses only a subset. Specifically, the example specifies
meta-information to build packages, and it specifies the contents of the
package.  Normally, a package will contain of addition modules, like Python
source modules, documentation, subpackages, etc. Please refer to the distutils
documentation in :ref:`distutils-index` to learn more about the features of
distutils; this section explains building extension modules only.

It is common to pre-compute arguments to :func:`setup`, to better structure the
driver script. In the example above, the\ ``ext_modules`` argument to
:func:`setup` is a list of extension modules, each of which is an instance of
the :class:`~distutils.extension.Extension`. In the example, the instance
defines an extension named ``demo`` which is build by compiling a single source
file, :file:`demo.c`.

In many cases, building an extension is more complex, since additional
preprocessor defines and libraries may be needed. This is demonstrated in the
example below. ::

   from distutils.core import setup, Extension

   module1 = Extension('demo',
                       define_macros = [('MAJOR_VERSION', '1'),
                                        ('MINOR_VERSION', '0')],
                       include_dirs = ['/usr/local/include'],
                       libraries = ['tcl83'],
                       library_dirs = ['/usr/local/lib'],
                       sources = ['demo.c'])

   setup (name = 'PackageName',
          version = '1.0',
          description = 'This is a demo package',
          author = 'Martin v. Loewis',
          author_email = 'martin@v.loewis.de',
          url = 'https://docs.python.org/extending/building',
          long_description = '''
   This is really just a demo package.
   ''',
          ext_modules = [module1])


In this example, :func:`setup` is called with additional meta-information, which
is recommended when distribution packages have to be built. For the extension
itself, it specifies preprocessor defines, include directories, library
directories, and libraries. Depending on the compiler, distutils passes this
information in different ways to the compiler. For example, on Unix, this may
result in the compilation commands ::

   gcc -DNDEBUG -g -O3 -Wall -Wstrict-prototypes -fPIC -DMAJOR_VERSION=1 -DMINOR_VERSION=0 -I/usr/local/include -I/usr/local/include/python2.2 -c demo.c -o build/temp.linux-i686-2.2/demo.o

   gcc -shared build/temp.linux-i686-2.2/demo.o -L/usr/local/lib -ltcl83 -o build/lib.linux-i686-2.2/demo.so

These lines are for demonstration purposes only; distutils users should trust
that distutils gets the invocations right.


.. _distributing:

Distributing your extension modules
===================================

When an extension has been successfully build, there are three ways to use it.

End-users will typically want to install the module, they do so by running ::

   python setup.py install

Module maintainers should produce source packages; to do so, they run ::

   python setup.py sdist

In some cases, additional files need to be included in a source distribution;
this is done through a :file:`MANIFEST.in` file; see the distutils documentation
for details.

If the source distribution has been build successfully, maintainers can also
create binary distributions. Depending on the platform, one of the following
commands can be used to do so. ::

   python setup.py bdist_wininst
   python setup.py bdist_rpm
   python setup.py bdist_dumb

