.. _package-upload:

***************************************
Uploading Packages to the Package Index
***************************************

.. versionadded:: 2.5

The Python Package Index (PyPI) not only stores the package info, but also  the
package data if the author of the package wishes to. The distutils command
:command:`upload` pushes the distribution files to PyPI.

The command is invoked immediately after building one or more distribution
files.  For example, the command ::

    python setup.py sdist bdist_wininst upload

will cause the source distribution and the Windows installer to be uploaded to
PyPI.  Note that these will be uploaded even if they are built using an earlier
invocation of :file:`setup.py`, but that only distributions named on the command
line for the invocation including the :command:`upload` command are uploaded.

The :command:`upload` command uses the username, password, and repository URL
from the :file:`$HOME/.pypirc` file (see section :ref:`pypirc` for more on this
file). If a :command:`register` command was previously called in the same command,
and if the password was entered in the prompt, :command:`upload` will reuse the
entered password. This is useful if you do not want to store a clear text
password in the :file:`$HOME/.pypirc` file.

You can specify another PyPI server with the :option:`--repository=*url*` option::

    python setup.py sdist bdist_wininst upload -r http://example.com/pypi

See section :ref:`pypirc` for more on defining several servers.

You can use the :option:`--sign` option to tell :command:`upload` to sign each
uploaded file using GPG (GNU Privacy Guard).  The  :program:`gpg` program must
be available for execution on the system :envvar:`PATH`.  You can also specify
which key to use for signing using the :option:`--identity=*name*` option.

Other :command:`upload` options include :option:`--repository=<url>` or
:option:`--repository=<section>` where *url* is the url of the server and
*section* the name of the section in :file:`$HOME/.pypirc`, and
:option:`--show-response` (which displays the full response text from the PyPI
server for help in debugging upload problems).

PyPI package display
====================

The ``long_description`` field plays a special role at PyPI. It is used by
the server to display a home page for the registered package.

If you use the `reStructuredText <http://docutils.sourceforge.net/rst.html>`_
syntax for this field, PyPI will parse it and display an HTML output for
the package home page.

The ``long_description`` field can be attached to a text file located
in the package::

    from distutils.core import setup

    with open('README.txt') as file:
        long_description = file.read()

    setup(name='Distutils',
          long_description=long_description)

In that case, :file:`README.txt` is a regular reStructuredText text file located
in the root of the package besides :file:`setup.py`.

To prevent registering broken reStructuredText content, you can use the
:program:`rst2html` program that is provided by the :mod:`docutils` package
and check the ``long_description`` from the command line::

    $ python setup.py --long-description | rst2html.py > output.html

:mod:`docutils` will display a warning if there's something wrong with your syntax.
