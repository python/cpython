.. _introspection:

***********************************
Introspecting a Python installation
***********************************

The mechanisms available to introspect a Python installation are highly
dependent on the installation you have. Their availability may depend on the
Python implementation in question, the build format, the target platform, or
several other details.

This document aims to give an overview of these mechanisms and provide the user
a general guide on how to approach this task, but users should still refer to
the documentation of their installation for more accurate information regarding
implementation details and availability.


Introduction
============

Generally, the most portable way to do this is by running some Python code to
extract information from the target interpreter, though this can often be
automated with the use of built-in helpers (see
:ref:`introspection-stdlib-clis`, :ref:`introspection-python-config-script`).

However, when introspecting a Python installation, running code is often
undesirable or impossible. For this purpose, Python defines a standard
:ref:`static install description file <introspection-install-details>` (please
see the :ref:`introspection-install-details-format` section for the format
specification), which may be provided together with your interpreter (please
see the :ref:`introspection-install-details-location` section for more details).
When available, this file can be used to lookup certain details from the
installation in question without having to run the interpreter.


Reference
=========


.. _introspection-install-details:

Install details file
--------------------


.. _introspection-install-details-format:

Format
~~~~~~

The details file is JSON file and the top-level object is a dictionary
containing the data described in :ref:`introspection-install-details-fields`.


.. _introspection-install-details-fields:

Fields
~~~~~~

This section documents some of the data fields that may be present. It is not
extensive — different implementations, platforms etc. may include additional
fields.

Dictionaries represent sub-sections, and their names are separated with
dots (``.``) in this documentation.

``python.version``
++++++++++++++++++

:Type: ``str``
:Description: A string representation of the Python version.

``python.version_parts``
++++++++++++++++++++++++

:Type: ``dict[str, int | str]``
:Description: A dictionary containing the different components of the Python
              version, as found in :py:data:`sys.version_info`.

``python.executable``
+++++++++++++++++++++

:Type: ``str``
:Description: An absolute path pointing to an interpreter from the installation.
:Availability: May be omitted in situations where it is unavailable, there isn't
               a reliable path that can be specified, etc.

``python.stdlib``
+++++++++++++++++++++

:Type: ``str``
:Description: An absolute path pointing to the directory where the standard
              library is installed.
:Availability: May be omitted in situations where it is unavailable, there isn't
               a reliable path that can be specified, etc.


.. _introspection-install-details-location:

Location
~~~~~~~~

On standard CPython installations, when available, the details file will be
installed in the same directory as the standard library, with the name
``build-details.json``.

Different implementations may place it in a different path, choose to provide
the file via a different mechanism, or choose not to include it at all.

.. note:: For Python implementation maintainers

   It would be good to try to align the install location between
   implementations, when possible, to help with compatibility.

   We just do not choose a installation path for everyone because different
   implementations may work differently, and it should be up to each project to
   decide what makes the most sense for them.


.. _introspection-stdlib-clis:

Standard library modules with a CLI
-----------------------------------

Some standard library modules include a CLI that exposes information, which can
be helpful for introspection.

- :mod:`sysconfig` (see :ref:`sysconfig-commandline`)
- :mod:`site` (see :ref:`site-commandline`)


.. _introspection-python-config-script:

The ``python-config`` script
----------------------------

.. TODO: Currently, we don't have any documentation covering python-config, but
         if/when we add some, we refer to it here, instead.

.. availability:: POSIX

A ``python-config`` script may be available alongside the interpreter
executable. This script exposes information especially relevant when building C
extensions.

When using it via ``PATH``, you should be careful with your environment, and
make sure that the first ``python-config`` entry does in fact belong to the
correct interpreter.

Additionally, the current implementation does not need to run the interpreter,
so this script may be helpful in situtation where that is not possible, or
undesirable. Though, please keep in mind that this is an implementation detail
and no guarantees are made regarding this aspect of the implementation.
