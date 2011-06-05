.. _packaging-pysetup-config:

=====================
Pysetup Configuration
=====================

Pysetup supports two configuration files: :file:`.pypirc` and :file:`packaging.cfg`.

.. FIXME integrate with configfile instead of duplicating

Configuring indexes
-------------------

You can configure additional indexes in :file:`.pypirc` to be used for index-related
operations. By default, all configured index-servers and package-servers will be used
in an additive fashion. To limit operations to specific indexes, use the :option:`--index`
and :option:`--package-server options`::

  $ pysetup install --index pypi --package-server django some.project

Adding indexes to :file:`.pypirc`::

  [packaging]
  index-servers =
      pypi
      other

  package-servers =
      django

  [pypi]
      repository: <repository-url>
      username: <username>
      password: <password>

  [other]
      repository: <repository-url>
      username: <username>
      password: <password>

  [django]
      repository: <repository-url>
      username: <username>
      password: <password>
