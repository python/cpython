.. highlight:: sh

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
   https://docs.fedoraproject.org/en-US/package-maintainers/Packaging_Tutorial_GNU_Hello/
      for Fedora users
   https://slackbook.org/html/package-management-making-packages.html
      for Slackware users


On FreeBSD and OpenBSD
----------------------

* FreeBSD users, to add the package use::

     pkg install python3

* OpenBSD users, to add the package use::

     pkg_add -r python

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
<https://devguide.python.org/setup/#get-the-source-code>`_.  (If you want
to contribute patches, you will need a clone.)

The build process consists of the usual commands::

   ./configure
   make
   make install

:ref:`Configuration options <configure-options>` and caveats for specific Unix
platforms are extensively documented in the :source:`README.rst` file in the
root of the Python source tree.

.. warning::

   ``make install`` can overwrite or masquerade the :file:`python3` binary.
   ``make altinstall`` is therefore recommended instead of ``make install``
   since it only installs :file:`{exec_prefix}/bin/python{version}`.


Python-related paths and files
==============================

These are subject to difference depending on local installation conventions;
:option:`prefix <--prefix>` and :option:`exec_prefix <--exec-prefix>`
are installation-dependent and should be interpreted as for GNU software; they
may be the same.

For example, on most Linux systems, the default for both is :file:`/usr`.

+-----------------------------------------------+------------------------------------------+
| File/directory                                | Meaning                                  |
+===============================================+==========================================+
| :file:`{exec_prefix}/bin/python3`             | Recommended location of the interpreter. |
+-----------------------------------------------+------------------------------------------+
| :file:`{prefix}/lib/python{version}`,         | Recommended locations of the directories |
| :file:`{exec_prefix}/lib/python{version}`     | containing the standard modules.         |
+-----------------------------------------------+------------------------------------------+
| :file:`{prefix}/include/python{version}`,     | Recommended locations of the directories |
| :file:`{exec_prefix}/include/python{version}` | containing the include files needed for  |
|                                               | developing Python extensions and         |
|                                               | embedding the interpreter.               |
+-----------------------------------------------+------------------------------------------+


Miscellaneous
=============

To easily use Python scripts on Unix, you need to make them executable,
e.g. with

.. code-block:: shell-session

   $ chmod +x script

and put an appropriate Shebang line at the top of the script.  A good choice is
usually ::

   #!/usr/bin/env python3

which searches for the Python interpreter in the whole :envvar:`PATH`.  However,
some Unices may not have the :program:`env` command, so you may need to hardcode
``/usr/bin/python3`` as the interpreter path.

To use shell commands in your Python scripts, look at the :mod:`subprocess` module.

.. _unix_custom_openssl:

Custom OpenSSL
==============

1. To use your vendor's OpenSSL configuration and system trust store, locate
   the directory with ``openssl.cnf`` file or symlink in ``/etc``. On most
   distribution the file is either in ``/etc/ssl`` or ``/etc/pki/tls``. The
   directory should also contain a ``cert.pem`` file and/or a ``certs``
   directory.

   .. code-block:: shell-session

      $ find /etc/ -name openssl.cnf -printf "%h\n"
      /etc/ssl

2. Download, build, and install OpenSSL. Make sure you use ``install_sw`` and
   not ``install``. The ``install_sw`` target does not override
   ``openssl.cnf``.

   .. code-block:: shell-session

      $ curl -O https://www.openssl.org/source/openssl-VERSION.tar.gz
      $ tar xzf openssl-VERSION
      $ pushd openssl-VERSION
      $ ./config \
          --prefix=/usr/local/custom-openssl \
          --libdir=lib \
          --openssldir=/etc/ssl
      $ make -j1 depend
      $ make -j8
      $ make install_sw
      $ popd

3. Build Python with custom OpenSSL
   (see the configure ``--with-openssl`` and ``--with-openssl-rpath`` options)

   .. code-block:: shell-session

      $ pushd python-3.x.x
      $ ./configure -C \
          --with-openssl=/usr/local/custom-openssl \
          --with-openssl-rpath=auto \
          --prefix=/usr/local/python-3.x.x
      $ make -j8
      $ make altinstall

.. note::

   Patch releases of OpenSSL have a backwards compatible ABI. You don't need
   to recompile Python to update OpenSSL. It's sufficient to replace the
   custom OpenSSL installation with a newer version.
