.. currentmodule:: asyncio


===================
Low-level API Index
===================

This page lists all low-level asyncio APIs.


Obtaining the Event Loop
========================

.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - :func:`asyncio.get_running_loop`
      - The **preferred** function to get the running event loop.

    * - :func:`asyncio.get_event_loop`
      - Get an event loop instance (current or via the policy).

    * - :func:`asyncio.set_event_loop`
      - Set the event loop as current via the current policy.

    * - :func:`asyncio.new_event_loop`
      - Create a new event loop.


.. rubric:: Examples

* :ref:`Using asyncio.get_running_loop() <asyncio_example_future>`.


Event Loop Methods
==================

See also the main documentation section about the
:ref:`event loop methods <asyncio-event-loop>`.

.. rubric:: Lifecycle
.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - :meth:`loop.run_until_complete`
      - Run a Future/Task/awaitable until complete.

    * - :meth:`loop.run_forever`
      - Run the event loop forever.

    * - :meth:`loop.stop`
      - Stop the event loop.

    * - :meth:`loop.close`
      - Close the event loop.

    * - :meth:`loop.is_running()`
      - Return ``True`` if the event loop is running.

    * - :meth:`loop.is_closed()`
      - Return ``True`` if the event loop is closed.

    * - ``await`` :meth:`loop.shutdown_asyncgens`
      - Close asynchronous generators.


.. rubric:: Debugging
.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - :meth:`loop.set_debug`
      - Enable or disable the debug mode.

    * - :meth:`loop.get_debug`
      - Get the current debug mode.


.. rubric:: Scheduling Callbacks
.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - :meth:`loop.call_soon`
      - Invoke a callback soon.

    * - :meth:`loop.call_soon_threadsafe`
      - A thread-safe variant of :meth:`loop.call_soon`.

    * - :meth:`loop.call_later`
      - Invoke a callback *after* the given time.

    * - :meth:`loop.call_at`
      - Invoke a callback *at* the given time.


.. rubric:: Thread/Process Pool
.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - ``await`` :meth:`loop.run_in_executor`
      - Run a CPU-bound or other blocking function in
        a :mod:`concurrent.futures` executor.

    * - :meth:`loop.set_default_executor`
      - Set the default executor for :meth:`loop.run_in_executor`.


.. rubric:: Tasks and Futures
.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - :meth:`loop.create_future`
      - Create a :class:`Future` object.

    * - :meth:`loop.create_task`
      - Schedule coroutine as a :class:`Task`.

    * - :meth:`loop.set_task_factory`
      - Set a factory used by :meth:`loop.create_task` to
        create :class:`Tasks <Task>`.

    * - :meth:`loop.get_task_factory`
      - Get the factory :meth:`loop.create_task` uses
        to create :class:`Tasks <Task>`.


.. rubric:: DNS
.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - ``await`` :meth:`loop.getaddrinfo`
      - Asynchronous version of :meth:`socket.getaddrinfo`.

    * - ``await`` :meth:`loop.getnameinfo`
      - Asynchronous version of :meth:`socket.getnameinfo`.


.. rubric:: Networking and IPC
.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - ``await`` :meth:`loop.create_connection`
      - Open a TCP connection.

    * - ``await`` :meth:`loop.create_server`
      - Create a TCP server.

    * - ``await`` :meth:`loop.create_unix_connection`
      - Open a Unix socket connection.

    * - ``await`` :meth:`loop.create_unix_server`
      - Create a Unix socket server.

    * - ``await`` :meth:`loop.connect_accepted_socket`
      - Wrap a :class:`~socket.socket` into a ``(transport, protocol)``
        pair.

    * - ``await`` :meth:`loop.create_datagram_endpoint`
      - Open a datagram (UDP) connection.

    * - ``await`` :meth:`loop.sendfile`
      - Send a file over a transport.

    * - ``await`` :meth:`loop.start_tls`
      - Upgrade an existing connection to TLS.

    * - ``await`` :meth:`loop.connect_read_pipe`
      - Wrap a read end of a pipe into a ``(transport, protocol)`` pair.

    * - ``await`` :meth:`loop.connect_write_pipe`
      - Wrap a write end of a pipe into a ``(transport, protocol)`` pair.


.. rubric:: Sockets
.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - ``await`` :meth:`loop.sock_recv`
      - Receive data from the :class:`~socket.socket`.

    * - ``await`` :meth:`loop.sock_recv_into`
      - Receive data from the :class:`~socket.socket` into a buffer.

    * - ``await`` :meth:`loop.sock_sendall`
      - Send data to the :class:`~socket.socket`.

    * - ``await`` :meth:`loop.sock_connect`
      - Connect the :class:`~socket.socket`.

    * - ``await`` :meth:`loop.sock_accept`
      - Accept a :class:`~socket.socket` connection.

    * - ``await`` :meth:`loop.sock_sendfile`
      - Send a file over the :class:`~socket.socket`.

    * - :meth:`loop.add_reader`
      - Start watching a file descriptor for read availability.

    * - :meth:`loop.remove_reader`
      - Stop watching a file descriptor for read availability.

    * - :meth:`loop.add_writer`
      - Start watching a file descriptor for write availability.

    * - :meth:`loop.remove_writer`
      - Stop watching a file descriptor for write availability.


.. rubric:: Unix Signals
.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - :meth:`loop.add_signal_handler`
      - Add a handler for a :mod:`signal`.

    * - :meth:`loop.remove_signal_handler`
      - Remove a handler for a :mod:`signal`.


.. rubric:: Subprocesses
.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - :meth:`loop.subprocess_exec`
      - Spawn a subprocess.

    * - :meth:`loop.subprocess_shell`
      - Spawn a subprocess from a shell command.


.. rubric:: Error Handling
.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - :meth:`loop.call_exception_handler`
      - Call the exception handler.

    * - :meth:`loop.set_exception_handler`
      - Set a new exception handler.

    * - :meth:`loop.get_exception_handler`
      - Get the current exception handler.

    * - :meth:`loop.default_exception_handler`
      - The default exception handler implementation.


.. rubric:: Examples

* :ref:`Using asyncio.get_event_loop() and loop.run_forever()
  <asyncio_example_lowlevel_helloworld>`.

* :ref:`Using loop.call_later() <asyncio_example_call_later>`.

* Using ``loop.create_connection()`` to implement
  :ref:`an echo-client <asyncio_example_tcp_echo_client_protocol>`.

* Using ``loop.create_connection()`` to
  :ref:`connect a socket <asyncio_example_create_connection>`.

* :ref:`Using add_reader() to watch an FD for read events
  <asyncio_example_watch_fd>`.

* :ref:`Using loop.add_signal_handler() <asyncio_example_unix_signals>`.

* :ref:`Using loop.subprocess_exec() <asyncio_example_subprocess_proto>`.


Transports
==========

All transports implement the following methods:

.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - :meth:`transport.close() <BaseTransport.close>`
      - Close the transport.

    * - :meth:`transport.is_closing() <BaseTransport.is_closing>`
      - Return ``True`` if the transport is closing or is closed.

    * - :meth:`transport.get_extra_info() <BaseTransport.get_extra_info>`
      - Request for information about the transport.

    * - :meth:`transport.set_protocol() <BaseTransport.set_protocol>`
      - Set a new protocol.

    * - :meth:`transport.get_protocol() <BaseTransport.get_protocol>`
      - Return the current protocol.


Transports that can receive data (TCP and Unix connections,
pipes, etc).  Returned from methods like
:meth:`loop.create_connection`, :meth:`loop.create_unix_connection`,
:meth:`loop.connect_read_pipe`, etc:

.. rubric:: Read Transports
.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - :meth:`transport.is_reading() <ReadTransport.is_reading>`
      - Return ``True`` if the transport is receiving.

    * - :meth:`transport.pause_reading() <ReadTransport.pause_reading>`
      - Pause receiving.

    * - :meth:`transport.resume_reading() <ReadTransport.resume_reading>`
      - Resume receiving.


Transports that can Send data (TCP and Unix connections,
pipes, etc).  Returned from methods like
:meth:`loop.create_connection`, :meth:`loop.create_unix_connection`,
:meth:`loop.connect_write_pipe`, etc:

.. rubric:: Write Transports
.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - :meth:`transport.write() <WriteTransport.write>`
      - Write data to the transport.

    * - :meth:`transport.writelines() <WriteTransport.writelines>`
      - Write buffers to the transport.

    * - :meth:`transport.can_write_eof() <WriteTransport.can_write_eof>`
      - Return :const:`True` if the transport supports sending EOF.

    * - :meth:`transport.write_eof() <WriteTransport.write_eof>`
      - Close and send EOF after flushing buffered data.

    * - :meth:`transport.abort() <WriteTransport.abort>`
      - Close the transport immediately.

    * - :meth:`transport.get_write_buffer_size()
        <WriteTransport.get_write_buffer_size>`
      - Return high and low water marks for write flow control.

    * - :meth:`transport.set_write_buffer_limits()
        <WriteTransport.set_write_buffer_limits>`
      - Set new high and low water marks for write flow control.


Transports returned by :meth:`loop.create_datagram_endpoint`:

.. rubric:: Datagram Transports
.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - :meth:`transport.sendto() <DatagramTransport.sendto>`
      - Send data to the remote peer.

    * - :meth:`transport.abort() <DatagramTransport.abort>`
      - Close the transport immediately.


Low-level transport abstraction over subprocesses.
Returned by :meth:`loop.subprocess_exec` and
:meth:`loop.subprocess_shell`:

.. rubric:: Subprocess Transports
.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - :meth:`transport.get_pid() <SubprocessTransport.get_pid>`
      - Return the subprocess process id.

    * - :meth:`transport.get_pipe_transport()
        <SubprocessTransport.get_pipe_transport>`
      - Return the transport for the requested communication pipe
        (*stdin*, *stdout*, or *stderr*).

    * - :meth:`transport.get_returncode() <SubprocessTransport.get_returncode>`
      - Return the subprocess return code.

    * - :meth:`transport.kill() <SubprocessTransport.kill>`
      - Kill the subprocess.

    * - :meth:`transport.send_signal() <SubprocessTransport.send_signal>`
      - Send a signal to the subprocess.

    * - :meth:`transport.terminate() <SubprocessTransport.terminate>`
      - Stop the subprocess.

    * - :meth:`transport.close() <SubprocessTransport.close>`
      - Kill the subprocess and close all pipes.


Protocols
=========

Protocol classes can implement the following **callback methods**:

.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - ``callback`` :meth:`connection_made() <BaseProtocol.connection_made>`
      - Called when a connection is made.

    * - ``callback`` :meth:`connection_lost() <BaseProtocol.connection_lost>`
      - Called when the connection is lost or closed.

    * - ``callback`` :meth:`pause_writing() <BaseProtocol.pause_writing>`
      - Called when the transport's buffer goes over the high water mark.

    * - ``callback`` :meth:`resume_writing() <BaseProtocol.resume_writing>`
      - Called when the transport's buffer drains below the low water mark.


.. rubric:: Streaming Protocols (TCP, Unix Sockets, Pipes)
.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - ``callback`` :meth:`data_received() <Protocol.data_received>`
      - Called when some data is received.

    * - ``callback`` :meth:`eof_received() <Protocol.eof_received>`
      - Called when an EOF is received.


.. rubric:: Buffered Streaming Protocols
.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - ``callback`` :meth:`get_buffer() <BufferedProtocol.get_buffer>`
      - Called to allocate a new receive buffer.

    * - ``callback`` :meth:`buffer_updated() <BufferedProtocol.buffer_updated>`
      - Called when the buffer was updated with the received data.

    * - ``callback`` :meth:`eof_received() <BufferedProtocol.eof_received>`
      - Called when an EOF is received.


.. rubric:: Datagram Protocols
.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - ``callback`` :meth:`datagram_received()
        <DatagramProtocol.datagram_received>`
      - Called when a datagram is received.

    * - ``callback`` :meth:`error_received() <DatagramProtocol.error_received>`
      - Called when a previous send or receive operation raises an
        :class:`OSError`.


.. rubric:: Subprocess Protocols
.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - ``callback`` :meth:`pipe_data_received()
        <SubprocessProtocol.pipe_data_received>`
      - Called when the child process writes data into its
        *stdout* or *stderr* pipe.

    * - ``callback`` :meth:`pipe_connection_lost()
        <SubprocessProtocol.pipe_connection_lost>`
      - Called when one of the pipes communicating with
        the child process is closed.

    * - ``callback`` :meth:`process_exited()
        <SubprocessProtocol.process_exited>`
      - Called when the child process has exited.


Event Loop Policies
===================

Policies is a low-level mechanism to alter the behavior of
functions like :func:`asyncio.get_event_loop`.  See also
the main :ref:`policies section <asyncio-policies>` for more
details.


.. rubric:: Accessing Policies
.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - :meth:`asyncio.get_event_loop_policy`
      - Return the current process-wide policy.

    * - :meth:`asyncio.set_event_loop_policy`
      - Set a new process-wide policy.

    * - :class:`AbstractEventLoopPolicy`
      - Base class for policy objects.
