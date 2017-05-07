.. currentmodule:: asyncio

.. _asyncio-controller:

++++++++++
Controller
++++++++++

There are situations where it's convenient to run an asyncio-based server in a
subthread, with the ability to start and stop the server from the main
thread.  One common example is in a test suite.

Let's say you want to test your implemention of a simple echo server.  In your
test's ``setUp()`` you want to start the echo server, and you want to be sure
your echo server stops in your ``tearDown()``.  Each individual `test_*()`
method would then test a different aspect of your echo server's protocol.

Your echo server cannot run in the foreground, otherwise it will block,
preventing the individual tests from running against your actual server.  Your
echo server must therefore run in the background.  However, you still want to
be able to start and stop your server in the foreground, and ensure that your
tests will not run until your server is started.

There are other use cases where you want the server to run in a subthread, but
you want the main thread to have reliable start and stop semantics.  The
abstract ``Controller`` class serves this purpose.


Server controller
=================

.. class:: Controller(hostname, port, ready_timeout=1.0, loop=None)

   This class provide a framework with which to run your servers in a
   background thread.  The only method you must override is ``factory()``,
   since that's what creates your server instances.

   *hostname* and *port* are passed directly to your loop's
   :meth:`AbstractEventLoop.create_server` method.  Both of these values are
   saved as similarly named attributes on the ``Controller` instance.

   *ready_timeout* is float number of seconds that the controller will wait in
   :meth:`Controller.start` for the subthread to start its server.  You can
   also set the :envvar:`PYTHONASYNCIOCONTROLLERTIMEOUT` to a float number of
   seconds, which takes precedence over the *ready_timeout* argument value.

   You can optionally pass in your own event *loop* for use by the
   controller.  If not given, :method:`new_event_loop()` is called to create
   the event loop.   In either case, the event loop instance is saved as a
   similarly named attribute on the ``Controller`` instance.

   .. attribute:: hostname
                  port

      The values of the *hostname* and *port* arguments.

   .. attribute:: loop

      The event loop being used.  This will either be the given *loop*
      argument, or the new event loop that was created.

   .. attribute:: ready_timeout

      The timeout value used to wait for the server to start.  This will
      either be the float value converted from the
      :envvar:`PYTHONASYNCIOCONTROLLERTIMEOUT` or the *ready_timeout*
      argument.

   .. attribute:: server

      This is the server instance returned by
      :meth:`AbstractEventLoop.create_server` after the server has started.

   .. method:: start()

      Start the server in the subthread.  The subthread is always a daemon
      thread (i.e. we always set ``thread.daemon=True``.  Exceptions can be
      raised if the server does not start within the *ready_timeout*, or if
      any other exception occurs in while creating the server.

   .. method:: stop()

      Stop the server and the event loop, and cancel all tasks.

   .. method:: factory()

      You **must** implement this method and it must at least return a new
      instance of your asyncio-based server.  This method is passed directly
      to :meth:`AbstractEventLoop.create_server`.
