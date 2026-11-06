.. _concurrency:

********************
Concurrent Execution
********************

The modules described in this chapter provide support for concurrent
execution of code. The appropriate choice of tool will depend on the
task to be executed (CPU bound vs IO bound) and preferred style of
development (event driven cooperative multitasking vs preemptive
multitasking). Here's an overview:


.. toctree::

   threading.rst
   multiprocessing.rst
   multiprocessing.shared_memory.rst
   concurrent.rst
   concurrent.futures.rst
   concurrent.interpreters.rst
   subprocess.rst
   sched.rst
   queue.rst
   contextvars.rst


The following are support modules for some of the above services:

.. toctree::

   _thread.rst
