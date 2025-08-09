.. highlight:: shell-session

.. _instrumentation:

===============================================
Instrumenting CPython with DTrace and SystemTap
===============================================

:author: David Malcolm
:author: Łukasz Langa

DTrace and SystemTap are monitoring tools, each providing a way to inspect
what the processes on a computer system are doing.  They both use
domain-specific languages allowing a user to write scripts which:

- filter which processes are to be observed
- gather data from the processes of interest
- generate reports on the data

As of Python 3.6, CPython can be built with embedded "markers", also
known as "probes", that can be observed by a DTrace or SystemTap script,
making it easier to monitor what the CPython processes on a system are
doing.

.. impl-detail::

   DTrace markers are implementation details of the CPython interpreter.
   No guarantees are made about probe compatibility between versions of
   CPython. DTrace scripts can stop working or work incorrectly without
   warning when changing CPython versions.


Enabling the static markers
---------------------------

macOS comes with built-in support for DTrace.  On Linux, in order to
build CPython with the embedded markers for SystemTap, the SystemTap
development tools must be installed.

On a Linux machine, this can be done via::

   $ yum install systemtap-sdt-devel

or::

   $ sudo apt-get install systemtap-sdt-dev


CPython must then be :option:`configured with the --with-dtrace option
<--with-dtrace>`:

.. code-block:: none

   checking for --with-dtrace... yes

On macOS, you can list available DTrace probes by running a Python
process in the background and listing all probes made available by the
Python provider::

   $ python3.6 -q &
   $ sudo dtrace -l -P python$!  # or: dtrace -l -m python3.6

      ID   PROVIDER            MODULE                          FUNCTION NAME
   29564 python18035        python3.6          _PyEval_EvalFrameDefault function-entry
   29565 python18035        python3.6             dtrace_function_entry function-entry
   29566 python18035        python3.6          _PyEval_EvalFrameDefault function-return
   29567 python18035        python3.6            dtrace_function_return function-return
   29568 python18035        python3.6                           collect gc-done
   29569 python18035        python3.6                           collect gc-start
   29570 python18035        python3.6          _PyEval_EvalFrameDefault line
   29571 python18035        python3.6                 maybe_dtrace_line line

On Linux, you can verify if the SystemTap static markers are present in
the built binary by seeing if it contains a ".note.stapsdt" section.

::

   $ readelf -S ./python | grep .note.stapsdt
   [30] .note.stapsdt        NOTE         0000000000000000 00308d78

If you've built Python as a shared library
(with the :option:`--enable-shared` configure option), you
need to look instead within the shared library.  For example::

   $ readelf -S libpython3.3dm.so.1.0 | grep .note.stapsdt
   [29] .note.stapsdt        NOTE         0000000000000000 00365b68

Sufficiently modern readelf can print the metadata::

    $ readelf -n ./python

      Displaying notes found in: .note.gnu.property
      Owner                Data size        Description
      GNU                  0x00000030       NT_GNU_PROPERTY_TYPE_0
            Properties: x86 ISA needed: x86-64-baseline
            x86 feature used: x86, XMM
            x86 ISA used: x86-64-baseline

      Displaying notes found in: .note.gnu.build-id
      Owner                Data size        Description
      GNU                  0x00000014       NT_GNU_BUILD_ID (unique build ID bitstring)
         Build ID: 880aff8660aefcd70df49bad72e8695c3bc65ea9

      Displaying notes found in: .note.ABI-tag
      Owner                Data size        Description
      GNU                  0x00000010       NT_GNU_ABI_TAG (ABI version tag)
         OS: Linux, ABI: 4.4.0

      Displaying notes found in: .note.stapsdt
      Owner                Data size        Description
      stapsdt              0x00000032       NT_STAPSDT (SystemTap probe descriptors)
         Provider: python
         Name: gc__start
         Location: 0x0000000000287b01, Base: 0x00000000004a7595, Semaphore: 0x00000000005ff818
         Arguments: -4@%r13d
      stapsdt              0x00000030       NT_STAPSDT (SystemTap probe descriptors)
         Provider: python
         Name: gc__done
         Location: 0x0000000000287b44, Base: 0x00000000004a7595, Semaphore: 0x00000000005ff81a
         Arguments: -8@%rax
      stapsdt              0x00000040       NT_STAPSDT (SystemTap probe descriptors)
         Provider: python
         Name: import__find__load__start
         Location: 0x0000000000297c25, Base: 0x00000000004a7595, Semaphore: 0x00000000005ff81c
         Arguments: 8@%rax
      stapsdt              0x00000047       NT_STAPSDT (SystemTap probe descriptors)
         Provider: python
         Name: import__find__load__done
         Location: 0x0000000000297c3d, Base: 0x00000000004a7595, Semaphore: 0x00000000005ff81e
         Arguments: 8@%rax -4@%edx
      stapsdt              0x00000033       NT_STAPSDT (SystemTap probe descriptors)
         Provider: python
         Name: audit
         Location: 0x00000000002d68c4, Base: 0x00000000004a7595, Semaphore: 0x00000000005ff820
         Arguments: 8@%r12 8@%r13

The above metadata contains information for SystemTap describing how it
can patch strategically placed machine code instructions to enable the
tracing hooks used by a SystemTap script.


Available static markers
------------------------

.. object:: gc__start(int generation)

   Fires when the Python interpreter starts a garbage collection cycle.
   ``arg0`` is the generation to scan, like :func:`gc.collect`.

.. object:: gc__done(long collected)

   Fires when the Python interpreter finishes a garbage collection
   cycle. ``arg0`` is the number of collected objects.

.. object:: import__find__load__start(str modulename)

   Fires before :mod:`importlib` attempts to find and load the module.
   ``arg0`` is the module name.

   .. versionadded:: 3.7

.. object:: import__find__load__done(str modulename, int found)

   Fires after :mod:`importlib`'s find_and_load function is called.
   ``arg0`` is the module name, ``arg1`` indicates if module was
   successfully loaded.

   .. versionadded:: 3.7


.. object:: audit(str event, void *tuple)

   Fires when :func:`sys.audit` or :c:func:`PySys_Audit` is called.
   ``arg0`` is the event name as C string, ``arg1`` is a :c:type:`PyObject`
   pointer to a tuple object.

   .. versionadded:: 3.8
