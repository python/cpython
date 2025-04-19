.. _remote-debugging:

Remote Debugging Attachment Protocol
====================================

This section explains the low-level protocol that allows external code to inject and execute
a Python script inside a running CPython process.

This is the mechanism implemented by the :func:`sys.remote_exec` function, which
instructs a remote Python process to execute a ``.py`` file. This section is not about using that
function, instead, it explains how the underlying protocol works so that it can be
reimplemented in any language.

The protocol assumes you already know the process you want to target and the code you want it to run.
That’s why it takes two pieces of information:

- The process ID (``pid``) of the Python process you want to interact with.
- A path to a Python script file (``.py``) that contains the code to be executed.

Once injected, the script is executed by the target process’s interpreter the next time it reaches
a safe evaluation point. This allows tools to trigger
code execution remotely without modifying the Python program itself.

In the sections that follow, we’ll walk through each step of this protocol in detail: how to locate
the interpreter in memory, how to access internal structures safely, and how to trigger the execution
of your script. Where necessary, we’ll highlight differences across platforms (Linux, macOS, Windows),
and include example code to help clarify each part of the process.

Locating the PyRuntime Structure
================================

The ``PyRuntime`` structure holds CPython's global interpreter state and serves as
the entry point to other internal data, including the list of interpreters,
thread states, and debugger support fields.

To interact with a remote Python process, a debugger must first compute the memory
address of the ``PyRuntime`` structure inside the target process. This cannot be
hardcoded or inferred symbolically, since its location depends on how the binary was
mapped into memory by the operating system.

The process for locating ``PyRuntime`` is platform-specific, but follows the same
high-level approach:

1. Identify where the Python executable or shared library was loaded in the target process.
2. Parse the corresponding binary file on disk to find the offset of the
   ``.PyRuntime`` section.
3. Compute the in-memory address of ``PyRuntime`` by relocating the section offset
   to the base address found in step 1.

Each subsection below explains what must be done and provides a short example of how this
can be implemented.

.. rubric:: Linux (ELF)

To locate the ``PyRuntime`` structure on Linux:

1. Inspect the memory mappings of the target process (e.g. from ``/proc/<pid>/maps``)
   to find the memory region where the Python executable or shared ``libpython``
   library is loaded. Record its base address.
2. Load the binary file from disk and parse its ELF section headers.
   Locate the ``.PyRuntime`` section and determine its file offset.
3. Add the section offset to the base address to compute the address of the
   ``PyRuntime`` structure in memory.

An example implementation might look like:

.. code-block:: python

    def find_py_runtime_linux(pid):
        # Step 1: Try to find the Python executable in memory
        binary_path, base_address = find_mapped_binary(pid, name_contains="python")
        # Step 2: Fallback to shared library if executable is not found
        if binary_path is None:
            binary_path, base_address = find_mapped_binary(pid, name_contains="libpython")
        # Step 3: Parse ELF headers of the binary to get .PyRuntime section offset
        section_offset = parse_elf_section_offset(binary_path, ".PyRuntime")
        # Step 4: Compute PyRuntime address in memory
        return base_address + section_offset

.. rubric:: macOS (Mach-O)

To locate the ``PyRuntime`` structure on macOS:

1. Obtain a handle to the target process that allows memory inspection.
2. Walk the memory regions of the process to identify the one that contains the
   Python binary or shared library. Record its base address and associated file path.
3. Load that binary file from disk and parse the Mach-O headers to find the
   ``__DATA,__PyRuntime`` section.
4. Add the section's offset to the base address of the loaded binary to compute
   the address of the ``PyRuntime`` structure.

An example implementation might look like:

.. code-block:: python

    def find_py_runtime_macos(pid):
        # Step 1: Get access to the process's memory
        handle = get_memory_access_handle(pid)
        # Step 2: Try to find the Python executable in memory
        binary_path, base_address = find_mapped_binary(handle, name_contains="python")
        # Step 3: Fallback to libpython if executable is not found
        if binary_path is None:
            binary_path, base_address = find_mapped_binary(handle, name_contains="libpython")
        # Step 4: Parse Mach-O headers to get __DATA,__PyRuntime section offset
        section_offset = parse_macho_section_offset(binary_path, "__DATA", "__PyRuntime")
        # Step 5: Compute PyRuntime address in memory
        return base_address + section_offset

.. rubric:: Windows (PE)

To locate the ``PyRuntime`` structure on Windows:

1. Enumerate all modules loaded in the target process.
   Identify the module corresponding to ``python.exe`` or ``pythonXY.dll``, where X and Y
   are the major and minor version numbers of the Python version, and record its base address.
2. Load the binary from disk and parse the PE section headers.
   Locate the ``.PyRuntime`` section and determine its relative virtual address (RVA).
3. Add the RVA to the module’s base address to compute the full in-memory address
   of the ``PyRuntime`` structure.

An example implementation might look like:

.. code-block:: python

    def find_py_runtime_windows(pid):
        # Step 1: Try to find the Python executable in memory
        binary_path, base_address = find_loaded_module(pid, name_contains="python")
        # Step 2: Fallback to shared pythonXY.dll if executable is not found
        if binary_path is None:
            binary_path, base_address = find_loaded_module(pid, name_contains="python3")
        # Step 3: Parse PE section headers to get .PyRuntime RVA
        section_rva = parse_pe_section_offset(binary_path, ".PyRuntime")
        # Step 4: Compute PyRuntime address in memory
        return base_address + section_rva

Reading _Py_DebugOffsets
=========================

Once the address of the ``PyRuntime`` structure has been computed in the target
process, the next step is to read the ``_Py_DebugOffsets`` structure located at
its beginning.

This structure contains version-specific field offsets needed to navigate
interpreter and thread state memory safely.

To read and validate the debug offsets:

1. Read the memory at the address of ``PyRuntime``, up to the size of
   ``_Py_DebugOffsets``. This structure is located at the very start of the
   ``PyRuntime`` block.

2. Verify that the contents of the structure are valid. In particular:

   - The ``cookie`` field must match the expected debug marker.
   - The ``version`` field must match the version of the Python interpreter
     used by the calling process (i.e., the debugger or controlling runtime).
   - If either the caller or the target process is running a pre-release version
     (such as an alpha, beta, or release candidate), then the versions must match
     exactly.
   - The ``free_threaded`` flag must match between the caller and the target process.

3. If the structure passes validation, the debugger may now safely use the
   provided offsets to locate fields in interpreter and thread state structures.

If any validation step fails, the debugger should abort rather than attempting to
access incompatible memory layouts.

An example of how a debugger might read and validate ``_Py_DebugOffsets``:

.. code-block:: python

    def read_debug_offsets(pid, py_runtime_addr):
        # Step 1: Read memory from the target process at the PyRuntime address
        data = read_process_memory(pid, address=py_runtime_addr, size=DEBUG_OFFSETS_SIZE)
        # Step 2: Deserialize the raw bytes into a _Py_DebugOffsets structure
        debug_offsets = parse_debug_offsets(data)
        # Step 3: Validate compatibility
        if debug_offsets.cookie != EXPECTED_COOKIE:
            raise RuntimeError("Invalid or missing debug cookie")
        if debug_offsets.version != LOCAL_PYTHON_VERSION:
            raise RuntimeError("Mismatch between caller and target Python versions")
        if debug_offsets.free_threaded != LOCAL_FREE_THREADED:
            raise RuntimeError("Mismatch in free-threaded configuration")
        return debug_offsets

Locating the Interpreter and Thread State
=========================================

After validating the ``_Py_DebugOffsets`` structure, the next step is to locate the
interpreter and thread state objects within the target process. These structures
hold essential runtime context and are required for writing debugger control
information.

- The ``PyInterpreterState`` structure represents a Python interpreter instance.
  Each interpreter holds its own module imports, built-in state, and thread list.
  Most applications use only one interpreter, but CPython supports creating multiple
  interpreters in the same process.

- The ``PyThreadState`` structure represents a thread running within an interpreter.
  This is where evaluation state and the control fields used by the debugger live.

To inject and run code remotely, the debugger must locate a valid ``PyThreadState``
to target. Typically, this is the main thread, but in some cases, the debugger may
want to attach to a specific thread by its native thread ID.

To locate a thread:

1. Use the offset ``runtime_state.interpreters_head`` to find the address of the
   first interpreter in the ``PyRuntime`` structure. This is the entry point to
   the list of active interpreters.

2. Use the offset ``interpreter_state.threads_main`` to locate the main thread
   of that interpreter. This is the simplest and most reliable thread to target.

3. Optionally, use ``interpreter_state.threads_head`` to walk the linked list of
   all threads. For each ``PyThreadState``, compare the ``native_thread_id``
   field (using ``thread_state.native_thread_id``) to find a specific thread.

   This is useful when the debugger allows the user to select which thread to inject into,
   or when targeting a thread that's actively running.

4. Once a valid ``PyThreadState`` is found, record its address. This will be used
   in the next step to write debugger control fields and schedule execution.

An example of locating the main thread:

.. code-block:: python

    def find_main_thread_state(pid, py_runtime_addr, debug_offsets):
        # Step 1: Read interpreters_head from PyRuntime
        interp_head_ptr = py_runtime_addr + debug_offsets.runtime_state.interpreters_head
        interp_addr = read_pointer(pid, interp_head_ptr)
        if interp_addr == 0:
            raise RuntimeError("No interpreter found in the target process")
        # Step 2: Read the threads_main pointer from the interpreter
        threads_main_ptr = interp_addr + debug_offsets.interpreter_state.threads_main
        thread_state_addr = read_pointer(pid, threads_main_ptr)
        if thread_state_addr == 0:
            raise RuntimeError("Main thread state is not available")
        return thread_state_addr

To locate a specific thread by native thread ID:

.. code-block:: python

    def find_thread_by_id(pid, interp_addr, debug_offsets, target_tid):
        # Start at threads_head and walk the linked list
        thread_ptr = read_pointer(
            pid, interp_addr + debug_offsets.interpreter_state.threads_head
        )
        while thread_ptr:
            native_tid_ptr = thread_ptr + debug_offsets.thread_state.native_thread_id
            native_tid = read_int(pid, native_tid_ptr)
            if native_tid == target_tid:
                return thread_ptr
            thread_ptr = read_pointer(pid, thread_ptr + debug_offsets.thread_state.next)
        raise RuntimeError("Thread with the given ID was not found")

Once a valid thread state has been identified, the debugger can use it to modify
control fields and request execution in the next stage of the protocol.

Writing Control Information
===========================

Once a valid thread state has been located, the debugger can write control fields
that instruct the target process to execute a script at the next safe opportunity.

Each thread state contains a ``_PyRemoteDebuggerSupport`` structure, which is used
to coordinate communication between the debugger and the interpreter. The debugger
uses offsets from ``_Py_DebugOffsets`` to locate three key fields:

- ``debugger_script_path``: A buffer where the debugger writes the full path to
  a Python source file (``.py``). The file must exist and be readable by the
  target process.

- ``debugger_pending_call``: An integer flag. When set to ``1``, it signals
  that a script is ready to be executed.

- ``eval_breaker``: A field checked periodically by the evaluation loop. To
  notify the interpreter of pending debugger activity, the debugger sets the
  ``_PY_EVAL_PLEASE_STOP_BIT`` in this field. This causes the interpreter to pause
  and check for debugger-related actions before continuing with normal execution.

To safely modify these fields, most debuggers should suspend the process before
writing to memory. This avoids race conditions that may occur if the interpreter
is actively running.

To perform the injection:

1. Write the script path into the ``debugger_script_path`` buffer.
2. Set the ``debugger_pending_call`` flag to ``1``.
3. Read the value of ``eval_breaker``, set the stop bit, and write the updated
   value back.

An example implementation might look like:

.. code-block:: python

    def inject_script(pid, thread_state_addr, debug_offsets, script_path):
        # Base offset to the _PyRemoteDebuggerSupport struct
        support_base = (
            thread_state_addr +
            debug_offsets.debugger_support.remote_debugger_support
        )
        # 1. Write script path
        script_path_ptr = support_base + debug_offsets.debugger_support.debugger_script_path
        write_string(pid, script_path_ptr, script_path)
        # 2. Set debugger_pending_call = 1
        pending_ptr = support_base + debug_offsets.debugger_support.debugger_pending_call
        write_int(pid, pending_ptr, 1)
        # 3. Set _PY_EVAL_PLEASE_STOP_BIT in eval_breaker
        eval_breaker_ptr = thread_state_addr + debug_offsets.debugger_support.eval_breaker
        breaker = read_int(pid, eval_breaker_ptr)
        # Set the least significant bit (this is _PY_EVAL_PLEASE_STOP_BIT)
        breaker |= 1
        write_int(pid, eval_breaker_ptr, breaker)

After these writes are complete, the debugger may resume the process (if it was paused).
The interpreter will check ``eval_breaker`` at the next evaluation checkpoint,
detect the pending call, and load and execute the specified Python file. The debugger is responsible
for ensuring that the file remains on disk and readable by the target interpreter
when it is accessed.

Summary
=======

To inject and execute a script in a remote Python process:

1. Locate the ``PyRuntime`` structure in the target process's memory.
2. Read and validate the ``_Py_DebugOffsets`` structure at the start of ``PyRuntime``.
3. Use the offsets to locate a valid ``PyThreadState``.
4. Write the path to a Python script into ``debugger_script_path``.
5. Set ``debugger_pending_call = 1``.
6. Set ``_PY_EVAL_PLEASE_STOP_BIT`` in ``eval_breaker``.
7. Resume the process (if paused). The script will be executed at the next safe eval point.
