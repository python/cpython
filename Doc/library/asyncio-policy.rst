.. currentmodule:: asyncio


.. _asyncio-policies:

========
Policies
========

.. warning::

   Policies are deprecated and will be removed in Python 3.16.
   Users are encouraged to use the :func:`asyncio.run` function
   or the :class:`asyncio.Runner` with *loop_factory* to use
   the desired loop implementation.


An event loop policy is a global object
used to get and set the current :ref:`event loop <asyncio-event-loop>`,
as well as create new event loops.
The default policy can be :ref:`replaced <asyncio-policy-get-set>` with
:ref:`built-in alternatives <asyncio-policy-builtin>`
to use different event loop implementations,
or substituted by a :ref:`custom policy <asyncio-custom-policies>`
that can override these behaviors.

The :ref:`policy object <asyncio-policy-objects>`
gets and sets a separate event loop per *context*.
This is per-thread by default,
though custom policies could define *context* differently.

Custom event loop policies can control the behavior of
:func:`get_event_loop`, :func:`set_event_loop`, and :func:`new_event_loop`.

Policy objects should implement the APIs defined
in the :class:`AbstractEventLoopPolicy` abstract base class.


.. _asyncio-policy-get-set:

Getting and Setting the Policy
==============================

The following functions can be used to get and set the policy
for the current process:

.. function:: get_event_loop_policy()

   Return the current process-wide policy.

   .. deprecated:: 3.14
      The :func:`get_event_loop_policy` function is deprecated and
      will be removed in Python 3.16.

.. function:: set_event_loop_policy(policy)

   Set the current process-wide policy to *policy*.

   If *policy* is set to ``None``, the default policy is restored.

   .. deprecated:: 3.14
      The :func:`set_event_loop_policy` function is deprecated and
      will be removed in Python 3.16.


.. _asyncio-policy-objects:

Policy Objects
==============

The abstract event loop policy base class is defined as follows:

.. class:: AbstractEventLoopPolicy

   An abstract base class for asyncio policies.

   .. method:: get_event_loop()

      Get the event loop for the current context.

      Return an event loop object implementing the
      :class:`AbstractEventLoop` interface.

      This method should never return ``None``.

      .. versionchanged:: 3.6

   .. method:: set_event_loop(loop)

      Set the event loop for the current context to *loop*.

   .. method:: new_event_loop()

      Create and return a new event loop object.

      This method should never return ``None``.

   .. deprecated:: 3.14
      The :class:`AbstractEventLoopPolicy` class is deprecated and
      will be removed in Python 3.16.


.. _asyncio-policy-builtin:

asyncio ships with the following built-in policies:


.. class:: DefaultEventLoopPolicy

   The default asyncio policy.  Uses :class:`SelectorEventLoop`
   on Unix and :class:`ProactorEventLoop` on Windows.

   There is no need to install the default policy manually. asyncio
   is configured to use the default policy automatically.

   .. versionchanged:: 3.8

      On Windows, :class:`ProactorEventLoop` is now used by default.

   .. versionchanged:: 3.14
      The :meth:`get_event_loop` method of the default asyncio policy now
      raises a :exc:`RuntimeError` if there is no set event loop.

   .. deprecated:: 3.14
      The :class:`DefaultEventLoopPolicy` class is deprecated and
      will be removed in Python 3.16.


.. class:: WindowsSelectorEventLoopPolicy

   An alternative event loop policy that uses the
   :class:`SelectorEventLoop` event loop implementation.

   .. availability:: Windows.

   .. deprecated:: 3.14
      The :class:`WindowsSelectorEventLoopPolicy` class is deprecated and
      will be removed in Python 3.16.


.. class:: WindowsProactorEventLoopPolicy

   An alternative event loop policy that uses the
   :class:`ProactorEventLoop` event loop implementation.

   .. availability:: Windows.

   .. deprecated:: 3.14
      The :class:`WindowsProactorEventLoopPolicy` class is deprecated and
      will be removed in Python 3.16.


.. _asyncio-custom-policies:

Custom Policies
===============

To implement a new event loop policy, it is recommended to subclass
:class:`DefaultEventLoopPolicy` and override the methods for which
custom behavior is wanted, e.g.::

    class MyEventLoopPolicy(asyncio.DefaultEventLoopPolicy):

        def get_event_loop(self):
            """Get the event loop.

            This may be None or an instance of EventLoop.
            """
            loop = super().get_event_loop()
            # Do something with loop ...
            return loop

    asyncio.set_event_loop_policy(MyEventLoopPolicy())
