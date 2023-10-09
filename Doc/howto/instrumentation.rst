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

As of Python 3.12, CPython can be built with embedded "markers", also
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

   $ python3.12 -q &
   $ sudo dtrace -l -P python$!  # or: dtrace -l -m python3.12

      ID   PROVIDER            MODULE                          FUNCTION NAME
   29568 python18035        python3.12                           collect gc-done
   29569 python18035        python3.12                           collect gc-start
   29570 python18035        python3.12          _PyEval_EvalFrameDefault line
   29571 python18035        python3.12                 maybe_dtrace_line line

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

    Displaying notes found in: .note.ABI-tag
      Owner                Data size        Description
      GNU                  0x00000010       NT_GNU_ABI_TAG (ABI version tag)
        OS: Linux, ABI: 4.4.0

    Displaying notes found in: .note.stapsdt
      Owner                Data size        Description
      stapsdt              0x00000040       NT_STAPSDT (SystemTap probe descriptors)
        Provider: python
        Name: import__find__load__start
        Location: 0x000000000024bb68, Base: 0x000000000041475d, Semaphore: 0x000000000059fa02
        Arguments: 8@%rax
      stapsdt              0x00000047       NT_STAPSDT (SystemTap probe descriptors)
        Provider: python
        Name: import__find__load__done
        Location: 0x000000000024bb80, Base: 0x000000000041475d, Semaphore: 0x000000000059fa04
        Arguments: 8@%rax -4@%edx
      stapsdt              0x00000033       NT_STAPSDT (SystemTap probe descriptors)
        Provider: python
        Name: audit
        Location: 0x0000000000283075, Base: 0x000000000041475d, Semaphore: 0x000000000059fa06
        Arguments: 8@%rbp 8@%rbx
      stapsdt              0x00000036       NT_STAPSDT (SystemTap probe descriptors)
        Provider: python
        Name: gc__start
        Location: 0x000000000029e7ad, Base: 0x000000000041475d, Semaphore: 0x000000000059f9fe
        Arguments: -4@120(%rsp)
      stapsdt              0x00000030       NT_STAPSDT (SystemTap probe descriptors)
        Provider: python
        Name: gc__done
        Location: 0x000000000029edd4, Base: 0x000000000041475d, Semaphore: 0x000000000059fa00
        Arguments: -8@%rbx

The above metadata contains information for SystemTap describing how it
can patch strategically placed machine code instructions to enable the
tracing hooks used by a SystemTap script.


Static DTrace probes
--------------------

The following example DTrace script can be used to show the call/return
hierarchy of a Python script, only tracing within the invocation of
a function called "start". In other words, import-time function
invocations are not going to be listed:

.. code-block:: none

    self int indent;

    python$target:::function-entry
    /copyinstr(arg1) == "start"/
    {
            self->trace = 1;
    }

    python$target:::function-entry
    /self->trace/
    {
            printf("%d\t%*s:", timestamp, 15, probename);
            printf("%*s", self->indent, "");
            printf("%s:%s:%d\n", basename(copyinstr(arg0)), copyinstr(arg1), arg2);
            self->indent++;
    }

    python$target:::function-return
    /self->trace/
    {
            self->indent--;
            printf("%d\t%*s:", timestamp, 15, probename);
            printf("%*s", self->indent, "");
            printf("%s:%s:%d\n", basename(copyinstr(arg0)), copyinstr(arg1), arg2);
    }

    python$target:::function-return
    /copyinstr(arg1) == "start"/
    {
            self->trace = 0;
    }

It can be invoked like this::

  $ sudo dtrace -q -s call_stack.d -c "python3.12 script.py"

The output looks like this:

.. code-block:: none

    156641360502280  function-entry:call_stack.py:start:23
    156641360518804  function-entry: call_stack.py:function_1:1
    156641360532797  function-entry:  call_stack.py:function_3:9
    156641360546807 function-return:  call_stack.py:function_3:10
    156641360563367 function-return: call_stack.py:function_1:2
    156641360578365  function-entry: call_stack.py:function_2:5
    156641360591757  function-entry:  call_stack.py:function_1:1
    156641360605556  function-entry:   call_stack.py:function_3:9
    156641360617482 function-return:   call_stack.py:function_3:10
    156641360629814 function-return:  call_stack.py:function_1:2
    156641360642285 function-return: call_stack.py:function_2:6
    156641360656770  function-entry: call_stack.py:function_3:9
    156641360669707 function-return: call_stack.py:function_3:10
    156641360687853  function-entry: call_stack.py:function_4:13
    156641360700719 function-return: call_stack.py:function_4:14
    156641360719640  function-entry: call_stack.py:function_5:18
    156641360732567 function-return: call_stack.py:function_5:21
    156641360747370 function-return:call_stack.py:start:28


Static SystemTap markers
------------------------

The low-level way to use the SystemTap integration is to use the static
markers directly.  This requires you to explicitly state the binary file
containing them.

For a :option:`--enable-shared` build of CPython, the markers are contained within the
libpython shared library, and the probe's dotted path needs to reflect this. For
example, this line from the above example:

.. code-block:: none

   probe process("python").mark("import__find__load__start") {

should instead read:

.. code-block:: none

   probe process("python").library("libpython3.12dm.so.1.0").mark("import__find__load__start") {

(assuming a :ref:`debug build <debug-build>` of CPython 3.12)


Available static markers
------------------------

.. object:: gc__start(int generation)

   Fires when the Python interpreter starts a garbage collection cycle.
   ``arg0`` is the generation to scan, like :func:`gc.collect()`.

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



