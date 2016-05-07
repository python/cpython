.. highlightlang:: sh

.. _using-on-unix:

********************************
 Using Python on Unix platforms
********************************

.. sectionauthor:: Shriphani Palakodety


Getting and installing the latest version of Python
===================================================

On Linux
--------

Python comes preinstalled on most Linux distributions, and is available as a
package on all others.  However there are certain features you might want to use
that are not available on your distro's package.  You can easily compile the
latest version of Python from source.

In the event that Python doesn't come preinstalled and isn't in the repositories as
well, you can easily make packages for your own distro.  Have a look at the
following links:

.. seealso::

   https://www.debian.org/doc/manuals/maint-guide/first.en.html
      for Debian users
   https://en.opensuse.org/Portal:Packaging
      for OpenSuse users
   https://docs.fedoraproject.org/en-US/Fedora_Draft_Documentation/0.1/html/RPM_Guide/ch-creating-rpms.html
      for Fedora users
   http://www.slackbook.org/html/package-management-making-packages.html
      for Slackware users


On FreeBSD and OpenBSD
----------------------

* FreeBSD users, to add the package use::

     pkg_add -r python

* OpenBSD users use::

     pkg_add ftp://ftp.openbsd.org/pub/OpenBSD/4.2/packages/<insert your architecture here>/python-<version>.tgz

  For example i386 users get the 2.5.1 version of Python using::

     pkg_add ftp://ftp.openbsd.org/pub/OpenBSD/4.2/packages/i386/python-2.5.1p2.tgz


On OpenSolaris
--------------

You can get Python from `OpenCSW <https://www.opencsw.org/>`_.  Various versions
of Python are available and can be installed with e.g. ``pkgutil -i python27``.


.. _building-python-on-unix:

Building Python
===============

If you want to compile CPython yourself, first thing you should do is get the
`source <https://www.python.org/downloads/source/>`_. You can download either the
latest release's source or just grab a fresh `clone
<https://docs.python.org/devguide/setup.html#getting-the-source-code>`_.  (If you want
to contribute patches, you will need a clone.)

The build process consists in the usual ::

   ./configure
   make
   make install

invocations. Configuration options and caveats for specific Unix platforms are
extensively documented in the :source:`README` file in the root of the Python
source tree.

.. warning::

   ``make install`` can overwrite or masquerade the :file:`python` binary.
   ``make altinstall`` is therefore recommended instead of ``make install``
   since it only installs :file:`{exec_prefix}/bin/python{version}`.


Python-related paths and files
==============================

These are subject to difference depending on local installation conventions;
:envvar:`prefix` (``${prefix}``) and :envvar:`exec_prefix` (``${exec_prefix}``)
are installation-dependent and should be interpreted as for GNU software; they
may be the same.

For example, on most Linux systems, the default for both is :file:`/usr`.

+-----------------------------------------------+------------------------------------------+
| File/directory                                | Meaning                                  |
+===============================================+==========================================+
| :file:`{exec_prefix}/bin/python`              | Recommended location of the interpreter. |
+-----------------------------------------------+------------------------------------------+
| :file:`{prefix}/lib/python{version}`,         | Recommended locations of the directories |
| :file:`{exec_prefix}/lib/python{version}`     | containing the standard modules.         |
+-----------------------------------------------+------------------------------------------+
| :file:`{prefix}/include/python{version}`,     | Recommended locations of the directories |
| :file:`{exec_prefix}/include/python{version}` | containing the include files needed for  |
|                                               | developing Python extensions and         |
|                                               | embedding the interpreter.               |
+-----------------------------------------------+------------------------------------------+
| :file:`~/.pythonrc.py`                        | User-specific initialization file loaded |
|                                               | by the user module; not used by default  |
|                                               | or by most applications.                 |
+-----------------------------------------------+------------------------------------------+


Miscellaneous
=============

To easily use Python scripts on Unix, you need to make them executable,
e.g. with ::

   $ chmod +x script

and put an appropriate Shebang line at the top of the script.  A good choice is
usually ::

   #!/usr/bin/env python

which searches for the Python interpreter in the whole :envvar:`PATH`.  However,
some Unices may not have the :program:`env` command, so you may need to hardcode
``/usr/bin/python`` as the interpreter path.

To use shell commands in your Python scripts, look at the :mod:`subprocess` module.


Editors
=======

Vim and Emacs are excellent editors which support Python very well.  For more
information on how to code in Python in these editors, look at:

* http://www.vim.org/scripts/script.php?script_id=790
* https://sourceforge.net/projects/python-mode

Geany is an excellent IDE with support for a lot of languages. For more
information, read: https://www.geany.org/

Komodo edit is another extremely good IDE.  It also has support for a lot of
languages. For more information, read https://komodoide.com/.
