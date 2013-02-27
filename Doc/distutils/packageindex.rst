.. index::
   single: Python Package Index (PyPI)
   single: PyPI; (see Python Package Index (PyPI))

.. _package-index:

*******************************
The Python Package Index (PyPI)
*******************************

The `Python Package Index (PyPI)`_ holds :ref:`meta-data <meta-data>`
describing distributions packaged with distutils, as well as package data like
distribution files if the package author wishes.

Distutils exposes two commands for submitting package data to PyPI: the
:ref:`register <package-register>` command for submitting meta-data to PyPI
and the :ref:`upload <package-upload>` command for submitting distribution
files.  Both commands read configuration data from a special file called the
:ref:`.pypirc file <pypirc>`.  PyPI :ref:`displays a home page
<package-display>` for each package created from the ``long_description``
submitted by the :command:`register` command.


.. _package-register:

Registering Packages
====================

The distutils command :command:`register` is used to submit your distribution's
meta-data to the index. It is invoked as follows::

    python setup.py register

Distutils will respond with the following prompt::

    running register
    We need to know who you are, so please choose either:
        1. use your existing login,
        2. register as a new user,
        3. have the server generate a new password for you (and email it to you), or
        4. quit
    Your selection [default 1]:

Note: if your username and password are saved locally, you will not see this
menu.

If you have not registered with PyPI, then you will need to do so now. You
should choose option 2, and enter your details as required. Soon after
submitting your details, you will receive an email which will be used to confirm
your registration.

Once you are registered, you may choose option 1 from the menu. You will be
prompted for your PyPI username and password, and :command:`register` will then
submit your meta-data to the index.

You may submit any number of versions of your distribution to the index. If you
alter the meta-data for a particular version, you may submit it again and the
index will be updated.

PyPI holds a record for each (name, version) combination submitted. The first
user to submit information for a given name is designated the Owner of that
name. They may submit changes through the :command:`register` command or through
the web interface. They may also designate other users as Owners or Maintainers.
Maintainers may edit the package information, but not designate other Owners or
Maintainers.

By default PyPI displays only the newest version of a given package. The web
interface lets one change this default behavior and manually select which
versions to display and hide.


.. _package-upload:

Uploading Packages
==================

.. versionadded:: 2.5

The distutils command :command:`upload` pushes the distribution files to PyPI.

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

You can specify another PyPI server with the ``--repository=url`` option::

    python setup.py sdist bdist_wininst upload -r http://example.com/pypi

See section :ref:`pypirc` for more on defining several servers.

You can use the ``--sign`` option to tell :command:`upload` to sign each
uploaded file using GPG (GNU Privacy Guard).  The  :program:`gpg` program must
be available for execution on the system :envvar:`PATH`.  You can also specify
which key to use for signing using the ``--identity=name`` option.

Other :command:`upload` options include ``--repository=url`` or
``--repository=section`` where *url* is the url of the server and
*section* the name of the section in :file:`$HOME/.pypirc`, and
``--show-response`` (which displays the full response text from the PyPI
server for help in debugging upload problems).


.. index::
   single: .pypirc file
   single: Python Package Index (PyPI); .pypirc file

.. _pypirc:

The .pypirc file
================

The format of the :file:`.pypirc` file is as follows::

    [distutils]
    index-servers =
        pypi

    [pypi]
    repository: <repository-url>
    username: <username>
    password: <password>

The *distutils* section defines a *index-servers* variable that lists the
name of all sections describing a repository.

Each section describing a repository defines three variables:

- *repository*, that defines the url of the PyPI server. Defaults to
    ``http://www.python.org/pypi``.
- *username*, which is the registered username on the PyPI server.
- *password*, that will be used to authenticate. If omitted the user
    will be prompt to type it when needed.

If you want to define another server a new section can be created and
listed in the *index-servers* variable::

    [distutils]
    index-servers =
        pypi
        other

    [pypi]
    repository: <repository-url>
    username: <username>
    password: <password>

    [other]
    repository: http://example.com/pypi
    username: <username>
    password: <password>

:command:`register` can then be called with the -r option to point the
repository to work with::

    python setup.py register -r http://example.com/pypi

For convenience, the name of the section that describes the repository
may also be used::

    python setup.py register -r other


.. _package-display:

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
:program:`rst2html` program that is provided by the :mod:`docutils` package and
check the ``long_description`` from the command line::

    $ python setup.py --long-description | rst2html.py > output.html

:mod:`docutils` will display a warning if there's something wrong with your
syntax.  Because PyPI applies additional checks (e.g. by passing ``--no-raw``
to ``rst2html.py`` in the command above), being able to run the command above
without warnings does not guarantee that PyPI will convert the content
successfully.


.. _Python Package Index (PyPI): http://pypi.python.org/
