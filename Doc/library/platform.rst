:mod:`platform` ---  Access to underlying platform's identifying data
=====================================================================

.. module:: platform
   :synopsis: Retrieves as much platform identifying data as possible.
.. moduleauthor:: Marc-Andre Lemburg <mal@egenix.com>
.. sectionauthor:: Bjorn Pettersen <bpettersen@corp.fairisaac.com>


.. versionadded:: 2.3

**Source code:** :source:`Lib/platform.py`

--------------

.. note::

   Specific platforms listed alphabetically, with Linux included in the Unix
   section.


Cross Platform
--------------


.. function:: architecture(executable=sys.executable, bits='', linkage='')

   Queries the given executable (defaults to the Python interpreter binary) for
   various architecture information.

   Returns a tuple ``(bits, linkage)`` which contain information about the bit
   architecture and the linkage format used for the executable. Both values are
   returned as strings.

   Values that cannot be determined are returned as given by the parameter presets.
   If bits is given as ``''``, the :c:func:`sizeof(pointer)` (or
   :c:func:`sizeof(long)` on Python version < 1.5.2) is used as indicator for the
   supported pointer size.

   The function relies on the system's :file:`file` command to do the actual work.
   This is available on most if not all Unix  platforms and some non-Unix platforms
   and then only if the executable points to the Python interpreter.  Reasonable
   defaults are used when the above needs are not met.

   .. note::

      On Mac OS X (and perhaps other platforms), executable files may be
      universal files containing multiple architectures.

      To get at the "64-bitness" of the current interpreter, it is more
      reliable to query the :attr:`sys.maxsize` attribute::

         is_64bits = sys.maxsize > 2**32


.. function:: machine()

   Returns the machine type, e.g. ``'i386'``. An empty string is returned if the
   value cannot be determined.


.. function:: node()

   Returns the computer's network name (may not be fully qualified!). An empty
   string is returned if the value cannot be determined.


.. function:: platform(aliased=0, terse=0)

   Returns a single string identifying the underlying platform with as much useful
   information as possible.

   The output is intended to be *human readable* rather than machine parseable. It
   may look different on different platforms and this is intended.

   If *aliased* is true, the function will use aliases for various platforms that
   report system names which differ from their common names, for example SunOS will
   be reported as Solaris.  The :func:`system_alias` function is used to implement
   this.

   Setting *terse* to true causes the function to return only the absolute minimum
   information needed to identify the platform.


.. function:: processor()

   Returns the (real) processor name, e.g. ``'amdk6'``.

   An empty string is returned if the value cannot be determined. Note that many
   platforms do not provide this information or simply return the same value as for
   :func:`machine`.  NetBSD does this.


.. function:: python_build()

   Returns a tuple ``(buildno, builddate)`` stating the Python build number and
   date as strings.


.. function:: python_compiler()

   Returns a string identifying the compiler used for compiling Python.


.. function:: python_branch()

   Returns a string identifying the Python implementation SCM branch.

   .. versionadded:: 2.6


.. function:: python_implementation()

   Returns a string identifying the Python implementation. Possible return values
   are: 'CPython', 'IronPython', 'Jython', 'PyPy'.

   .. versionadded:: 2.6


.. function:: python_revision()

   Returns a string identifying the Python implementation SCM revision.

   .. versionadded:: 2.6


.. function:: python_version()

   Returns the Python version as string ``'major.minor.patchlevel'``

   Note that unlike the Python ``sys.version``, the returned value will always
   include the patchlevel (it defaults to 0).


.. function:: python_version_tuple()

   Returns the Python version as tuple ``(major, minor, patchlevel)`` of strings.

   Note that unlike the Python ``sys.version``, the returned value will always
   include the patchlevel (it defaults to ``'0'``).


.. function:: release()

   Returns the system's release, e.g. ``'2.2.0'`` or ``'NT'`` An empty string is
   returned if the value cannot be determined.


.. function:: system()

   Returns the system/OS name, e.g. ``'Linux'``, ``'Windows'``, or ``'Java'``. An
   empty string is returned if the value cannot be determined.


.. function:: system_alias(system, release, version)

   Returns ``(system, release, version)`` aliased to common marketing names used
   for some systems.  It also does some reordering of the information in some cases
   where it would otherwise cause confusion.


.. function:: version()

   Returns the system's release version, e.g. ``'#3 on degas'``. An empty string is
   returned if the value cannot be determined.


.. function:: uname()

   Fairly portable uname interface. Returns a tuple of strings ``(system, node,
   release, version, machine, processor)`` identifying the underlying platform.

   Note that unlike the :func:`os.uname` function this also returns possible
   processor information as additional tuple entry.

   Entries which cannot be determined are set to ``''``.


Java Platform
-------------


.. function:: java_ver(release='', vendor='', vminfo=('','',''), osinfo=('','',''))

   Version interface for Jython.

   Returns a tuple ``(release, vendor, vminfo, osinfo)`` with *vminfo* being a
   tuple ``(vm_name, vm_release, vm_vendor)`` and *osinfo* being a tuple
   ``(os_name, os_version, os_arch)``. Values which cannot be determined are set to
   the defaults given as parameters (which all default to ``''``).


Windows Platform
----------------


.. function:: win32_ver(release='', version='', csd='', ptype='')

   Get additional version information from the Windows Registry and return a tuple
   ``(version, csd, ptype)`` referring to version number, CSD level
   (service pack) and OS type (multi/single processor).

   As a hint: *ptype* is ``'Uniprocessor Free'`` on single processor NT machines
   and ``'Multiprocessor Free'`` on multi processor machines. The *'Free'* refers
   to the OS version being free of debugging code. It could also state *'Checked'*
   which means the OS version uses debugging code, i.e. code that checks arguments,
   ranges, etc.

   .. note::

      This function works best with Mark Hammond's
      :mod:`win32all` package installed, but also on Python 2.3 and
      later (support for this was added in Python 2.6). It obviously
      only runs on Win32 compatible platforms.


Win95/98 specific
^^^^^^^^^^^^^^^^^

.. function:: popen(cmd, mode='r', bufsize=None)

   Portable :func:`popen` interface.  Find a working popen implementation
   preferring :func:`win32pipe.popen`.  On Windows NT, :func:`win32pipe.popen`
   should work; on Windows 9x it hangs due to bugs in the MS C library.


Mac OS Platform
---------------


.. function:: mac_ver(release='', versioninfo=('','',''), machine='')

   Get Mac OS version information and return it as tuple ``(release, versioninfo,
   machine)`` with *versioninfo* being a tuple ``(version, dev_stage,
   non_release_version)``.

   Entries which cannot be determined are set to ``''``.  All tuple entries are
   strings.


Unix Platforms
--------------


.. function:: dist(distname='', version='', id='', supported_dists=('SuSE','debian','redhat','mandrake',...))

   This is an old version of the functionality now provided by
   :func:`linux_distribution`. For new code, please use the
   :func:`linux_distribution`.

   The only difference between the two is that ``dist()`` always
   returns the short name of the distribution taken from the
   ``supported_dists`` parameter.

   .. deprecated:: 2.6

.. function:: linux_distribution(distname='', version='', id='', supported_dists=('SuSE','debian','redhat','mandrake',...), full_distribution_name=1)

   Tries to determine the name of the Linux OS distribution name.

   ``supported_dists`` may be given to define the set of Linux distributions to
   look for. It defaults to a list of currently supported Linux distributions
   identified by their release file name.

   If ``full_distribution_name`` is true (default), the full distribution read
   from the OS is returned. Otherwise the short name taken from
   ``supported_dists`` is used.

   Returns a tuple ``(distname,version,id)`` which defaults to the args given as
   parameters.  ``id`` is the item in parentheses after the version number.  It
   is usually the version codename.

   .. versionadded:: 2.6

.. function:: libc_ver(executable=sys.executable, lib='', version='', chunksize=2048)

   Tries to determine the libc version against which the file executable (defaults
   to the Python interpreter) is linked.  Returns a tuple of strings ``(lib,
   version)`` which default to the given parameters in case the lookup fails.

   Note that this function has intimate knowledge of how different libc versions
   add symbols to the executable is probably only usable for executables compiled
   using :program:`gcc`.

   The file is read and scanned in chunks of *chunksize* bytes.

