.. _packaging-install-index:

******************************
  Installing Python Projects
******************************

:Author: The Fellowship of the Packaging
:Release: |version|
:Date: |today|

.. TODO: Fill in XXX comments

.. The audience for this document includes people who don't know anything
   about Python and aren't about to learn the language just in order to
   install and maintain it for their users, i.e. system administrators.
   Thus, I have to be sure to explain the basics at some point:
   sys.path and PYTHONPATH at least. Should probably give pointers to
   other docs on "import site", PYTHONSTARTUP, PYTHONHOME, etc.

   Finally, it might be useful to include all the material from my "Care
   and Feeding of a Python Installation" talk in here somewhere. Yow!

.. topic:: Abstract

   This document describes Packaging from the end-user's point of view: it
   explains how to extend the functionality of a standard Python installation by
   building and installing third-party Python modules and applications.


This guide is split into a simple overview  followed by a longer presentation of
the :program:`pysetup` script, the Python package management tool used to
build, distribute, search for, install, remove and list Python distributions.

.. TODO integrate install and pysetup instead of duplicating

.. toctree::
   :maxdepth: 2
   :numbered:

   install
   pysetup
   pysetup-config
   pysetup-servers


.. seealso::

   :ref:`packaging-index`
      The manual for developers of Python projects who want to package and
      distribute them. This describes how to use :mod:`packaging` to make
      projects easily found and added to an existing Python installation.

   :mod:`packaging`
      A library reference for developers of packaging tools wanting to use
      standalone building blocks like :mod:`~packaging.version` or
      :mod:`~packaging.metadata`, or extend Packaging itself.
