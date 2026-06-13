.. currentmodule:: asyncio

.. _asyncio-introspection-tools:

================================
Command-line introspection tools
================================

**Source code:** :source:`Lib/asyncio/tools.py`

-------------------------------------

The :mod:`!asyncio` module can be executed as a script to inspect asyncio
tasks in another Python process:

.. code-block:: shell-session

   $ python -m asyncio ps [--retries N] PID
   $ python -m asyncio pstree [--retries N] PID

``PID`` is the process ID of the Python process to inspect.  The commands read
the target process state without executing any code in it.  They are only
available on supported platforms and may require permission to inspect another
process.  See :ref:`permission-requirements` for details.

.. seealso::

   :ref:`asyncio-graph`
      Programmatic APIs for inspecting the async call graph of a task or
      future in the current process.

The command examples below use this program, which creates a task hierarchy
suitable for inspection and prints its process ID:

.. code-block:: python

   import asyncio
   import os

   async def play(track):
       await asyncio.sleep(3600)
       print(f"🎵 Finished: {track}")

   async def album(name, tracks):
       async with asyncio.TaskGroup() as tg:
           for track in tracks:
               tg.create_task(play(track), name=track)

   async def main():
       print(f"PID: {os.getpid()}", flush=True)
       async with asyncio.TaskGroup() as tg:
           tg.create_task(
               album("Sundowning", ["TNDNBTG", "Levitate"]),
               name="Sundowning",
           )
           tg.create_task(
               album("TMBTE", ["DYWTYLM", "Aqua Regia"]),
               name="TMBTE",
           )

   asyncio.run(main())

Run the program in one terminal and leave it running:

.. code-block:: shell-session

   $ python example.py
   PID: 12345

Then pass the printed process ID to the commands from another terminal.
Thread IDs, task IDs, file paths, and line numbers vary between runs and
source layouts.

.. versionadded:: 3.14

Command-line options
====================

.. option:: ps PID

   Display a table of pending tasks in the process *PID*.  The table includes
   the thread ID, task ID, task name, coroutine stack, awaiter chain, awaiter
   name, and awaiter ID:

   .. code-block:: shell-session

      $ python -m asyncio ps 12345
      tid        task id              task name            coroutine stack                                    awaiter chain                                      awaiter name    awaiter id
      ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
      18445801   0x10a456060          Task-1               TaskGroup._aexit -> TaskGroup.__aexit__ -> main                                                                       0x0
      18445801   0x10a439f60          Sundowning           TaskGroup._aexit -> TaskGroup.__aexit__ -> album   TaskGroup._aexit -> TaskGroup.__aexit__ -> main    Task-1          0x10a456060
      18445801   0x10a439d70          TMBTE                TaskGroup._aexit -> TaskGroup.__aexit__ -> album   TaskGroup._aexit -> TaskGroup.__aexit__ -> main    Task-1          0x10a456060
      18445801   0x10a2a3a80          TNDNBTG              sleep -> play                                      TaskGroup._aexit -> TaskGroup.__aexit__ -> album   Sundowning      0x10a439f60
      18445801   0x10a2a38a0          Levitate             sleep -> play                                      TaskGroup._aexit -> TaskGroup.__aexit__ -> album   Sundowning      0x10a439f60
      18445801   0x10a2d7150          DYWTYLM              sleep -> play                                      TaskGroup._aexit -> TaskGroup.__aexit__ -> album   TMBTE           0x10a439d70
      18445801   0x10a6bdaa0          Aqua Regia           sleep -> play                                      TaskGroup._aexit -> TaskGroup.__aexit__ -> album   TMBTE           0x10a439d70

.. option:: pstree PID

   Display the same task and coroutine relationships as a tree:

   .. code-block:: shell-session

      $ python -m asyncio pstree 12345
      └── (T) Task-1
          └──  main example.py:12
              └──  TaskGroup.__aexit__ Lib/asyncio/taskgroups.py:75
                  └──  TaskGroup._aexit Lib/asyncio/taskgroups.py:124
                      ├── (T) Sundowning
                      │   └──  album example.py:7
                      │       └──  TaskGroup.__aexit__ Lib/asyncio/taskgroups.py:75
                      │           └──  TaskGroup._aexit Lib/asyncio/taskgroups.py:124
                      │               ├── (T) TNDNBTG
                      │               │   └──  play example.py:4
                      │               │       └──  sleep Lib/asyncio/tasks.py:702
                      │               └── (T) Levitate
                      │                   └──  play example.py:4
                      │                       └──  sleep Lib/asyncio/tasks.py:702
                      └── (T) TMBTE
                          └──  album example.py:7
                              └──  TaskGroup.__aexit__ Lib/asyncio/taskgroups.py:75
                                  └──  TaskGroup._aexit Lib/asyncio/taskgroups.py:124
                                      ├── (T) DYWTYLM
                                      │   └──  play example.py:4
                                      │       └──  sleep Lib/asyncio/tasks.py:702
                                      └── (T) Aqua Regia
                                          └──  play example.py:4
                                              └──  sleep Lib/asyncio/tasks.py:702

   If the await graph contains a cycle, ``pstree`` reports an error instead
   of printing a tree.  A cycle in the await graph is unusual and typically
   indicates a programming error:

   .. code-block:: shell-session

      $ python -m asyncio pstree 12345
      ERROR: await-graph contains cycles - cannot print a tree!

      cycle: Task-2 → Task-3 → Task-2

.. option:: --retries N

   Retry failed attempts to inspect the target process up to *N* times.  See
   :ref:`sampling-efficiency` for details about failed process memory reads.

   .. versionadded:: 3.15
