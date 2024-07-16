:mod:`!interpreters.queues` -- A cross-interpreter queue implementation
==================================================================

XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

.. module:: interpreters.queues
   :synopsis: Multiple Interpreters in the Same Process

.. moduleauthor:: Eric Snow <ericsnowcurrently@gmail.com>
.. sectionauthor:: Eric Snow <ericsnowcurrently@gmail.com>

.. versionadded:: 3.14

**Source code:** :source:`Lib/interpreters/queues.py`

--------------


Introduction
------------

This module constructs higher-level interfaces on top of the lower
level ``_interpqueues`` module.

.. seealso::

   :ref:`execcomponents`
       ...

   :mod:`interpreters`
       provides details about communicating between interpreters.

   :ref:`multiple-interpreters-howto`
       demonstrates how to use queues

.. include:: ../includes/wasm-notavail.rst


API Summary
-----------

+-------------------------------------------------------------+--------------------------------------------+
| signature                                                   | description                                |
+=============================================================+============================================+
| ``list_all() -> [Queue]``                                   | Get all existing cross-interpreter queues. |
+-------------------------------------------------------------+--------------------------------------------+
| ``create(*, syncobj=False, unbounditems=UNBOUND) -> Queue`` | Initialize a new cross-interpreter queue.  |
+-------------------------------------------------------------+--------------------------------------------+

|

+------------------------------------------------------------+---------------------------------------------------+
| signature                                                  | description                                       |
+============================================================+===================================================+
| ``class Queue``                                            | A single cross-interpreter queue.                 |
+------------------------------------------------------------+---------------------------------------------------+
| ``.id``                                                    | The queue's ID (read-only).                       |
+------------------------------------------------------------+---------------------------------------------------+
| ``.maxsize``                                               | The queue's capacity, if applicable (read-only).  |
+------------------------------------------------------------+---------------------------------------------------+
| ``.qsize() -> bool``                                       | The queue's current item count.                   |
+------------------------------------------------------------+---------------------------------------------------+
| ``.empty() -> bool``                                       | Does the queue currently have no items it it?     |
+------------------------------------------------------------+---------------------------------------------------+
| ``.full() -> bool``                                        | Has the queue reached its maxsize, if any?        |
+------------------------------------------------------------+---------------------------------------------------+
| ``.put(obj, timeout=None, *, syncobj=None, unbound=None)`` | Add an item to the back of the queue.             |
+------------------------------------------------------------+---------------------------------------------------+
| ``.put_nowait(obj, *, syncobj=None, unbound=None)``        | Add an item to the back of the queue.             |
+------------------------------------------------------------+---------------------------------------------------+
| ``.get(timeout=None) -> object``                           | Pop off the next item in queue.                   |
+------------------------------------------------------------+---------------------------------------------------+
| ``.get_nowait() -> object``                                | Pop off the next item in queue.                   |
+------------------------------------------------------------+---------------------------------------------------+

Exceptions:

+--------------------------+---------------+---------------------------------------------------+
| class                    | base class    | description                                       |
+==========================+==============+====================================================+
| QueueError               | Exception     | A queue-relaed error happened.                    |
+--------------------------+---------------+---------------------------------------------------+
| QueueNotFoundError       | QueueError    | The targeted queue no longer exists.              |
+--------------------------+---------------+---------------------------------------------------+
| QueueEmptyError          | | QueueError  | The targeted queue is empty.                      |
|                          | | queue.Empty |                                                   | 
+--------------------------+---------------+---------------------------------------------------+
| QueueFullError           | | QueueError  | The targeted queue is full.                       |
|                          | | queue.Full  |                                                   | 
+--------------------------+---------------+---------------------------------------------------+
| ItemInterpreterDestroyed | QueueError    | The interpreter that added the next item is gone. |
+--------------------------+---------------+---------------------------------------------------+


Basic Usage
-----------

::

    import interpreters.queues

    queue = interpreters.queues.create()
    interp = interpreters.create()
    interp.prepare_main(queue=queue)

    queue.put('spam!')
    interp.exec("""
        msg = queue.get()
        print(msg)
        """)

::

    queue = interpreters.queues.create()
    interp = interpreters.create()
    interp.prepare_main(queue=queue)

    queue.put('spam!')
    interp.exec("""
        obj = queue.get()
        print(msg)
        """)

::

    queue1 = interpreters.queues.create()
    queue2 = interpreters.queues.create(syncobj=True)
    interp = interpreters.create()
    interp.prepare_main(queue1=queue1, queue2=queue2)

    queue1.put('spam!')
    queue1.put('spam!', syncobj=True)
    queue2.put('spam!')
    queue2.put('spam!', syncobj=False)
    interp.exec("""
        msg1 = queue1.get()
        msg2 = queue1.get()
        msg3 = queue2.get()
        msg4 = queue2.get()
        print(msg)
        """)


Functions
---------

This module defines the following functions:

