.. _package-index:

**********************************
Registering with the Package Index
**********************************

The Python Package Index (PyPI) holds meta-data describing distributions
packaged with distutils. The distutils command :command:`register` is used to
submit your distribution's meta-data to the index. It is invoked as follows::

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

By default PyPI will list all versions of a given package. To hide certain
versions, the Hidden property should be set to yes. This must be edited through
the web interface.


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

*repository* can be omitted and defaults to ``http://www.python.org/pypi``.

If you want to define another server a new section can be created::

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

The command can then be called with the -r option::

   python setup.py register -r http://example.com/pypi

Or even with the section name::

   python setup.py register -r other

