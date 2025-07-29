.. highlight:: shell-session

.. _perf_profiling:

========================================================
Python support for the ``perf map`` compatible profilers
========================================================

:author: Pablo Galindo

`The Linux perf profiler <https://perf.wiki.kernel.org>`_ and
`samply <https://github.com/mstange/samply>`_ are powerful tools that allow you to
profile and obtain information about the performance of your application.
Both tools have vibrant ecosystems that aid with the analysis of the data they produce.

The main problem with using these profilers with Python applications is that
they only get information about native symbols, that is, the names of
functions and procedures written in C. This means that the names and file names
of Python functions in your code will not appear in the profiler output.

Since Python 3.12, the interpreter can run in a special mode that allows Python
functions to appear in the output of compatible profilers. When this mode is
enabled, the interpreter will interpose a small piece of code compiled on the
fly before the execution of every Python function and it will teach the profiler the
relationship between this piece of code and the associated Python function using
:doc:`perf map files <../c-api/perfmaps>`.

.. note::

    Support for profiling is available on Linux and macOS on select architectures.
    Perf is available on Linux, while samply can be used on both Linux and macOS.
    samply support on macOS is available starting from Python 3.15.
    Check the output of the ``configure`` build step or
    check the output of ``python -m sysconfig | grep HAVE_PERF_TRAMPOLINE``
    to see if your system is supported.

For example, consider the following script:

.. code-block:: python

    def foo(n):
        result = 0
        for _ in range(n):
            result += 1
        return result

    def bar(n):
        foo(n)

    def baz(n):
        bar(n)

    if __name__ == "__main__":
        baz(1000000)

We can run ``perf`` to sample CPU stack traces at 9999 hertz::

    $ perf record -F 9999 -g -o perf.data python my_script.py

Then we can use ``perf report`` to analyze the data:

.. code-block:: shell-session

    $ perf report --stdio -n -g

    # Children      Self       Samples  Command     Shared Object       Symbol
    # ........  ........  ............  ..........  ..................  ..........................................
    #
        91.08%     0.00%             0  python.exe  python.exe          [.] _start
                |
                ---_start
                |
                    --90.71%--__libc_start_main
                            Py_BytesMain
                            |
                            |--56.88%--pymain_run_python.constprop.0
                            |          |
                            |          |--56.13%--_PyRun_AnyFileObject
                            |          |          _PyRun_SimpleFileObject
                            |          |          |
                            |          |          |--55.02%--run_mod
                            |          |          |          |
                            |          |          |           --54.65%--PyEval_EvalCode
                            |          |          |                     _PyEval_EvalFrameDefault
                            |          |          |                     PyObject_Vectorcall
                            |          |          |                     _PyEval_Vector
                            |          |          |                     _PyEval_EvalFrameDefault
                            |          |          |                     PyObject_Vectorcall
                            |          |          |                     _PyEval_Vector
                            |          |          |                     _PyEval_EvalFrameDefault
                            |          |          |                     PyObject_Vectorcall
                            |          |          |                     _PyEval_Vector
                            |          |          |                     |
                            |          |          |                     |--51.67%--_PyEval_EvalFrameDefault
                            |          |          |                     |          |
                            |          |          |                     |          |--11.52%--_PyCompactLong_Add
                            |          |          |                     |          |          |
                            |          |          |                     |          |          |--2.97%--_PyObject_Malloc
    ...

As you can see, the Python functions are not shown in the output, only ``_PyEval_EvalFrameDefault``
(the function that evaluates the Python bytecode) shows up. Unfortunately that's not very useful because all Python
functions use the same C function to evaluate bytecode so we cannot know which Python function corresponds to which
bytecode-evaluating function.

Instead, if we run the same experiment with ``perf`` support enabled we get:

.. code-block:: shell-session

    $ perf report --stdio -n -g

    # Children      Self       Samples  Command     Shared Object       Symbol
    # ........  ........  ............  ..........  ..................  .....................................................................
    #
        90.58%     0.36%             1  python.exe  python.exe          [.] _start
                |
                ---_start
                |
                    --89.86%--__libc_start_main
                            Py_BytesMain
                            |
                            |--55.43%--pymain_run_python.constprop.0
                            |          |
                            |          |--54.71%--_PyRun_AnyFileObject
                            |          |          _PyRun_SimpleFileObject
                            |          |          |
                            |          |          |--53.62%--run_mod
                            |          |          |          |
                            |          |          |           --53.26%--PyEval_EvalCode
                            |          |          |                     py::<module>:/src/script.py
                            |          |          |                     _PyEval_EvalFrameDefault
                            |          |          |                     PyObject_Vectorcall
                            |          |          |                     _PyEval_Vector
                            |          |          |                     py::baz:/src/script.py
                            |          |          |                     _PyEval_EvalFrameDefault
                            |          |          |                     PyObject_Vectorcall
                            |          |          |                     _PyEval_Vector
                            |          |          |                     py::bar:/src/script.py
                            |          |          |                     _PyEval_EvalFrameDefault
                            |          |          |                     PyObject_Vectorcall
                            |          |          |                     _PyEval_Vector
                            |          |          |                     py::foo:/src/script.py
                            |          |          |                     |
                            |          |          |                     |--51.81%--_PyEval_EvalFrameDefault
                            |          |          |                     |          |
                            |          |          |                     |          |--13.77%--_PyCompactLong_Add
                            |          |          |                     |          |          |
                            |          |          |                     |          |          |--3.26%--_PyObject_Malloc



Using the samply profiler
-------------------------

samply is a modern profiler that can be used as an alternative to perf.
It uses the same perf map files that Python generates, making it compatible
with Python's profiling support. samply is particularly useful on macOS
where perf is not available.

To use samply with Python, first install it following the instructions at
https://github.com/mstange/samply, then run::

    $ samply record PYTHONPERFSUPPORT=1 python my_script.py

This will open a web interface where you can analyze the profiling data
interactively. The advantage of samply is that it provides a modern
web-based interface for analyzing profiling data and works on both Linux
and macOS.

On macOS, samply support requires Python 3.15 or later. Also on macOS, samply
can't profile signed Python executables due to restrictions by macOS. You can
profile with Python binaries that you've compiled yourself, or which are
unsigned or locally-signed (such as anything installed by Homebrew). In
order to attach to running processes on macOS, run ``samply setup`` once (and
every time samply is updated) to self-sign the samply binary.

How to enable ``perf`` profiling support
----------------------------------------

``perf`` profiling support can be enabled either from the start using
the environment variable :envvar:`PYTHONPERFSUPPORT` or the
:option:`-X perf <-X>` option,
or dynamically using :func:`sys.activate_stack_trampoline` and
:func:`sys.deactivate_stack_trampoline`.

The :mod:`!sys` functions take precedence over the :option:`!-X` option,
the :option:`!-X` option takes precedence over the environment variable.

Example, using the environment variable::

   $ PYTHONPERFSUPPORT=1 perf record -F 9999 -g -o perf.data python my_script.py
   $ perf report -g -i perf.data

Example, using the :option:`!-X` option::

   $ perf record -F 9999 -g -o perf.data python -X perf my_script.py
   $ perf report -g -i perf.data

Example, using the :mod:`sys` APIs in file :file:`example.py`:

.. code-block:: python

   import sys

   sys.activate_stack_trampoline("perf")
   do_profiled_stuff()
   sys.deactivate_stack_trampoline()

   non_profiled_stuff()

...then::

   $ perf record -F 9999 -g -o perf.data python ./example.py
   $ perf report -g -i perf.data


How to obtain the best results
------------------------------

For best results, Python should be compiled with
``CFLAGS="-fno-omit-frame-pointer -mno-omit-leaf-frame-pointer"`` as this allows
profilers to unwind using only the frame pointer and not on DWARF debug
information. This is because as the code that is interposed to allow ``perf``
support is dynamically generated it doesn't have any DWARF debugging information
available.

You can check if your system has been compiled with this flag by running::

    $ python -m sysconfig | grep 'no-omit-frame-pointer'

If you don't see any output it means that your interpreter has not been compiled with
frame pointers and therefore it may not be able to show Python functions in the output
of ``perf``.


How to work without frame pointers
----------------------------------

If you are working with a Python interpreter that has been compiled without
frame pointers, you can still use the ``perf`` profiler, but the overhead will be
a bit higher because Python needs to generate unwinding information for every
Python function call on the fly. Additionally, ``perf`` will take more time to
process the data because it will need to use the DWARF debugging information to
unwind the stack and this is a slow process.

To enable this mode, you can use the environment variable
:envvar:`PYTHON_PERF_JIT_SUPPORT` or the :option:`-X perf_jit <-X>` option,
which will enable the JIT mode for the ``perf`` profiler.

.. note::

    Due to a bug in the ``perf`` tool, only ``perf`` versions higher than v6.8
    will work with the JIT mode.  The fix was also backported to the v6.7.2
    version of the tool.

    Note that when checking the version of the ``perf`` tool (which can be done
    by running ``perf version``) you must take into account that some distros
    add some custom version numbers including a ``-`` character.  This means
    that ``perf 6.7-3`` is not necessarily ``perf 6.7.3``.

When using the perf JIT mode, you need an extra step before you can run ``perf
report``. You need to call the ``perf inject`` command to inject the JIT
information into the ``perf.data`` file.::

    $ perf record -F 9999 -g -k 1 --call-graph dwarf -o perf.data python -Xperf_jit my_script.py
    $ perf inject -i perf.data --jit --output perf.jit.data
    $ perf report -g -i perf.jit.data

or using the environment variable::

    $ PYTHON_PERF_JIT_SUPPORT=1 perf record -F 9999 -g --call-graph dwarf -o perf.data python my_script.py
    $ perf inject -i perf.data --jit --output perf.jit.data
    $ perf report -g -i perf.jit.data

``perf inject --jit`` command will read ``perf.data``,
automatically pick up the perf dump file that Python creates (in
``/tmp/perf-$PID.dump``), and then create ``perf.jit.data`` which merges all the
JIT information together. It should also create a lot of ``jitted-XXXX-N.so``
files in the current directory which are ELF images for all the JIT trampolines
that were created by Python.

.. warning::
    When using ``--call-graph dwarf``, the ``perf`` tool will take
    snapshots of the stack of the process being profiled and save the
    information in the ``perf.data`` file. By default, the size of the stack dump
    is 8192 bytes, but you can change the size by passing it after
    a comma like ``--call-graph dwarf,16384``.

    The size of the stack dump is important because if the size is too small
    ``perf`` will not be able to unwind the stack and the output will be
    incomplete. On the other hand, if the size is too big, then ``perf`` won't
    be able to sample the process as frequently as it would like as the overhead
    will be higher.

    The stack size is particularly important when profiling Python code compiled
    with low optimization levels (like ``-O0``), as these builds tend to have
    larger stack frames. If you are compiling Python with ``-O0`` and not seeing
    Python functions in your profiling output, try increasing the stack dump
    size to 65528 bytes (the maximum)::

        $ perf record -F 9999 -g -k 1 --call-graph dwarf,65528 -o perf.data python -Xperf_jit my_script.py

    Different compilation flags can significantly impact stack sizes:

    - Builds with ``-O0`` typically have much larger stack frames than those with ``-O1`` or higher
    - Adding optimizations (``-O1``, ``-O2``, etc.) typically reduces stack size
    - Frame pointers (``-fno-omit-frame-pointer``) generally provide more reliable stack unwinding
