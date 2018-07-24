.. index::
   single: Python Package Index (PyPI)
   single: PyPI; (see Python Package Index (PyPI))

.. _package-index:

*******************************
The Python Package Index (PyPI)
*******************************

The `Python Package Index (PyPI)`_ stores :ref:`meta-data <meta-data>`
describing distributions packaged with distutils, as well as package data like
distribution files if a package author wishes.

Distutils provides the :command:`register` and :command:`upload` commands for
pushing meta-data and distribution files to PyPI, respectively.  See
:ref:`package-commands` for information on these commands.


PyPI overview
=============

PyPI lets you submit any number of versions of your distribution to the index.
If you alter the meta-data for a particular version, you can submit it again
and the index will be updated.

PyPI holds a record for each (name, version) combination submitted.  The first
user to submit information for a given name is designated the Owner of that
name.  Changes can be submitted through the :command:`register` command or
through the web interface.  Owners can designate other users as Owners or
Maintainers.  Maintainers can edit the package information, but not designate
new Owners or Maintainers.

By default PyPI displays only the newest version of a given package.  The web
interface lets one change this default behavior and manually select which
versions to display and hide.

For each version, PyPI displays a home page.  The home page is created from
the ``long_description`` which can be submitted via the :command:`register`
command.  See :ref:`package-display` for more information.


.. _package-commands:

Distutils commands
==================

Distutils exposes two commands for submitting package data to PyPI: the
:ref:`register <package-register>` command for submitting meta-data to PyPI
and the :ref:`upload <package-upload>` command for submitting distribution
files.  Both commands read configuration data from a special file called a
:ref:`.pypirc file <pypirc>`.


.. _package-register:

The ``register`` command
------------------------

The distutils command :command:`register` is used to submit your distribution's
meta-data to an index server. It is invoked as follows::

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
menu.  Also, refer to :ref:`pypirc` for how to store your credentials in a
:file:`.pypirc` file.

If you have not registered with PyPI, then you will need to do so now. You
should choose option 2, and enter your details as required. Soon after
submitting your details, you will receive an email which will be used to confirm
your registration.

Once you are registered, you may choose option 1 from the menu. You will be
prompted for your PyPI username and password, and :command:`register` will then
submit your meta-data to the index.

See :ref:`package-cmdoptions` for options to the :command:`register` command.


.. _package-upload:

The ``upload`` command
----------------------

The distutils command :command:`upload` pushes the distribution files to PyPI.

The command is invoked immediately after building one or more distribution
files.  For example, the command ::

    python setup.py sdist bdist_wininst upload

will cause the source distribution and the Windows installer to be uploaded to
PyPI.  Note that these will be uploaded even if they are built using an earlier
invocation of :file:`setup.py`, but that only distributions named on the command
line for the invocation including the :command:`upload` command are uploaded.

If a :command:`register` command was previously called in the same command,
and if the password was entered in the prompt, :command:`upload` will reuse the
entered password.  This is useful if you do not want to store a password in
clear text in a :file:`.pypirc` file.

You can use the ``--sign`` option to tell :command:`upload` to sign each
uploaded file using GPG (GNU Privacy Guard).  The  :program:`gpg` program must
be available for execution on the system :envvar:`PATH`.  You can also specify
which key to use for signing using the ``--identity=name`` option.

See :ref:`package-cmdoptions` for additional options to the :command:`upload`
command.


.. _package-cmdoptions:

Additional command options
--------------------------

This section describes options common to both the :command:`register` and
:command:`upload` commands.

The ``--repository`` or ``-r`` option lets you specify a PyPI server
different from the default.  For example::

    python setup.py sdist bdist_wininst upload -r https://example.com/pypi

For convenience, a name can be used in place of the URL when the
:file:`.pypirc` file is configured to do so.  For example::

    python setup.py register -r other

See :ref:`pypirc` for more information on defining alternate servers.

The ``--show-response`` option displays the full response text from the PyPI
server, which is useful when debugging problems with registering and uploading.


.. index::
   single: .pypirc file
   single: Python Package Index (PyPI); .pypirc file

.. _pypirc:

The ``.pypirc`` file
--------------------

The :command:`register` and :command:`upload` commands both check for the
existence of a :file:`.pypirc` file at the location :file:`$HOME/.pypirc`.
If this file exists, the command uses the username, password, and repository
URL configured in the file.  The format of a :file:`.pypirc` file is as
follows:

.. code-block:: ini

    [distutils]
    index-servers =
        pypi

    [pypi]
    repository: <repository-url>
    username: <username>
    password: <password>

The *distutils* section defines an *index-servers* variable that lists the
name of all sections describing a repository.

Each section describing a repository defines three variables:

- *repository*, that defines the url of the PyPI server. Defaults to
    ``https://upload.pypi.org/legacy/``.
- *username*, which is the registered username on the PyPI server.
- *password*, that will be used to authenticate. If omitted the user
    will be prompt to type it when needed.

If you want to define another server a new section can be created and
listed in the *index-servers* variable:

.. code-block:: ini

    [distutils]
    index-servers =
        pypi
        other

    [pypi]
    repository: <repository-url>
    username: <username>
    password: <password>

    [other]
    repository: https://example.com/pypi
    username: <username>
    password: <password>

This allows the :command:`register` and :command:`upload` commands to be
called with the ``--repository`` option as described in
:ref:`package-cmdoptions`.

Specifically, you might want to add the `PyPI Test Repository
<https://wiki.python.org/moin/TestPyPI>`_ to your ``.pypirc`` to facilitate
testing before doing your first upload to ``PyPI`` itself.


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
check the ``long_description`` from the command line:

.. code-block:: shell-session

    $ python setup.py --long-description | rst2html.py > output.html

:mod:`docutils` will display a warning if there's something wrong with your
syntax.  Because PyPI applies additional checks (e.g. by passing ``--no-raw``
to ``rst2html.py`` in the command above), being able to run the command above
without warnings does not guarantee that PyPI will convert the content
successfully.


.. _Python Package Index (PyPI): https://pypi.org
