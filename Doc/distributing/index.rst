.. _distributing-index:

###############################
  Distributing Python Modules
###############################

:Email: distutils-sig@python.org


As a popular open source development project, Python has an active
supporting community of contributors and users that also make their software
available for other Python developers to use under open source license terms.

This allows Python users to share and collaborate effectively, benefiting
from the solutions others have already created to common (and sometimes
even rare!) problems, as well as potentially contributing their own
solutions to the common pool.

This guide covers the distribution part of the process. For a guide to
installing other Python projects, refer to the
:ref:`installation guide <installing-index>`.

.. note::

   For corporate and other institutional users, be aware that many
   organisations have their own policies around using and contributing to
   open source software. Please take such policies into account when making
   use of the distribution and installation tools provided with Python.


Key terms
=========

* the `Python Packaging Index <https://pypi.python.org/pypi>`__ is a public
  repository of open source licensed packages made available for use by
  other Python users
* the `Python Packaging Authority
  <https://packaging.python.org/en/latest/future.html>`__ are the group of
  developers and documentation authors responsible for the maintenance and
  evolution of the standard packaging tools and the associated metadata and
  file format standards. They maintain a variety of tools, documentation
  and issue trackers on both `GitHub <https://github.com/pypa>`__ and
  `BitBucket <https://bitbucket.org/pypa/>`__.
* :mod:`distutils` is the original build and distribution system first added
  to the Python standard library in 1998. While direct use of :mod:`distutils`
  is being phased out, it still laid the foundation for the current packaging
  and distribution infrastructure, and it not only remains part of the
  standard library, but its name lives on in other ways (such as the name
  of the mailing list used to coordinate Python packaging standards
  development).
* `setuptools`_ is a (largely) drop-in replacement for :mod:`distutils` first
  published in 2004. Its most notable addition over the unmodified
  :mod:`distutils` tools was the ability to declare dependencies on other
  packages. It is currently recommended as a more regularly updated
  alternative to :mod:`distutils` that offers consistent support for more
  recent packaging standards across a wide range of Python versions.
* `wheel`_ (in this context) is a project that adds the ``bdist_wheel``
  command to :mod:`distutils`/`setuptools`_. This produces a cross platform
  binary packaging format (called "wheels" or "wheel files" and defined in
  :pep:`427`) that allows Python libraries, even those including binary
  extensions, to be installed on a system without needing to be built
  locally.

.. _setuptools: https://setuptools.pypa.io/en/latest/setuptools.html
.. _wheel: http://wheel.readthedocs.org

Open source licensing and collaboration
=======================================

In most parts of the world, software is automatically covered by copyright.
This means that other developers require explicit permission to copy, use,
modify and redistribute the software.

Open source licensing is a way of explicitly granting such permission in a
relatively consistent way, allowing developers to share and collaborate
efficiently by making common solutions to various problems freely available.
This leaves many developers free to spend more time focusing on the problems
that are relatively unique to their specific situation.

The distribution tools provided with Python are designed to make it
reasonably straightforward for developers to make their own contributions
back to that common pool of software if they choose to do so.

The same distribution tools can also be used to distribute software within
an organisation, regardless of whether that software is published as open
source software or not.


Installing the tools
====================

The standard library does not include build tools that support modern
Python packaging standards, as the core development team has found that it
is important to have standard tools that work consistently, even on older
versions of Python.

The currently recommended build and distribution tools can be installed
by invoking the ``pip`` module at the command line::

    python -m pip install setuptools wheel twine

.. note::

   For POSIX users (including Mac OS X and Linux users), these instructions
   assume the use of a :term:`virtual environment`.

   For Windows users, these instructions assume that the option to
   adjust the system PATH environment variable was selected when installing
   Python.

The Python Packaging User Guide includes more details on the `currently
recommended tools`_.

.. _currently recommended tools: https://packaging.python.org/en/latest/current.html#packaging-tool-recommendations

Reading the guide
=================

The Python Packaging User Guide covers the various key steps and elements
involved in creating a project:

* `Project structure`_
* `Building and packaging the project`_
* `Uploading the project to the Python Packaging Index`_

.. _Project structure: \
   https://packaging.python.org/en/latest/distributing.html#creating-your-own-project
.. _Building and packaging the project: \
   https://packaging.python.org/en/latest/distributing.html#packaging-your-project
.. _Uploading the project to the Python Packaging Index: \
   https://packaging.python.org/en/latest/distributing.html#uploading-your-project-to-pypi


How do I...?
============

These are quick answers or links for some common tasks.

... choose a name for my project?
---------------------------------

This isn't an easy topic, but here are a few tips:

* check the Python Packaging Index to see if the name is already in use
* check popular hosting sites like GitHub, BitBucket, etc to see if there
  is already a project with that name
* check what comes up in a web search for the name you're considering
* avoid particularly common words, especially ones with multiple meanings,
  as they can make it difficult for users to find your software when
  searching for it


... create and distribute binary extensions?
--------------------------------------------

This is actually quite a complex topic, with a variety of alternatives
available depending on exactly what you're aiming to achieve. See the
Python Packaging User Guide for more information and recommendations.

.. seealso::

   `Python Packaging User Guide: Binary Extensions
   <https://packaging.python.org/en/latest/extensions.html>`__

.. other topics:

   Once the Development & Deployment part of PPUG is fleshed out, some of
   those sections should be linked from new questions here (most notably,
   we should have a question about avoiding depending on PyPI that links to
   https://packaging.python.org/en/latest/deployment.html#pypi-mirrors-and-caches)
