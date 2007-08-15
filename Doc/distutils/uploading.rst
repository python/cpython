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
file).

You can use the :option:`--sign` option to tell :command:`upload` to sign each
uploaded file using GPG (GNU Privacy Guard).  The  :program:`gpg` program must
be available for execution on the system :envvar:`PATH`.  You can also specify
which key to use for signing using the :option:`--identity=*name*` option.

Other :command:`upload` options include  :option:`--repository=*url*` (which
lets you override the repository setting from :file:`$HOME/.pypirc`), and
:option:`--show-response` (which displays the full response text from the PyPI
server for help in debugging upload problems).


