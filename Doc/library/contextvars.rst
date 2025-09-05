:mod:`!contextvars` --- Context Variables
=========================================

.. module:: contextvars
   :synopsis: Context Variables

.. sectionauthor:: Yury Selivanov <yury@magic.io>

--------------

This module provides APIs to manage, store, and access context-local
state.  The :class:`~contextvars.ContextVar` class is used to declare
and work with *Context Variables*.  The :func:`~contextvars.copy_context`
function and the :class:`~contextvars.Context` class should be used to
manage the current context in asynchronous frameworks.

Context managers that have state should use Context Variables
instead of :func:`threading.local` to prevent their state from
bleeding to other code unexpectedly, when used in concurrent code.

See also :pep:`567` for additional details.

.. versionadded:: 3.7


Context Variables
-----------------

.. class:: ContextVar(name, [*, default])

   This class is used to declare a new Context Variable, e.g.::

       var: ContextVar[int] = ContextVar('var', default=42)

   The required *name* parameter is used for introspection and debug
   purposes.

   The optional keyword-only *default* parameter is returned by
   :meth:`ContextVar.get` when no value for the variable is found
   in the current context.

   **Important:** Context Variables should be created at the top module
   level and never in closures.  :class:`Context` objects hold strong
   references to context variables which prevents context variables
   from being properly garbage collected.

   .. attribute:: ContextVar.name

      The name of the variable.  This is a read-only property.

      .. versionadded:: 3.7.1

   .. method:: get([default])

      Return a value for the context variable for the current context.

      If there is no value for the variable in the current context,
      the method will:

      * return the value of the *default* argument of the method,
        if provided; or

      * return the default value for the context variable,
        if it was created with one; or

      * raise a :exc:`LookupError`.

   .. method:: set(value)

      Call to set a new value for the context variable in the current
      context.

      The required *value* argument is the new value for the context
      variable.

      Returns a :class:`~contextvars.Token` object that can be used
      to restore the variable to its previous value via the
      :meth:`ContextVar.reset` method.

   .. method:: reset(token)

      Reset the context variable to the value it had before the
      :meth:`ContextVar.set` that created the *token* was used.

      For example::

          var = ContextVar('var')

          token = var.set('new value')
          # code that uses 'var'; var.get() returns 'new value'.
          var.reset(token)

          # After the reset call the var has no value again, so
          # var.get() would raise a LookupError.


.. class:: Token

   *Token* objects are returned by the :meth:`ContextVar.set` method.
   They can be passed to the :meth:`ContextVar.reset` method to revert
   the value of the variable to what it was before the corresponding
   *set*.

   The token supports :ref:`context manager protocol <context-managers>`
   to restore the corresponding context variable value at the exit from
   :keyword:`with` block::

       var = ContextVar('var', default='default value')

       with var.set('new value'):
           assert var.get() == 'new value'

       assert var.get() == 'default value'

   .. versionadded:: 3.14

      Added support for usage as a context manager.

   .. attribute:: Token.var

      A read-only property.  Points to the :class:`ContextVar` object
      that created the token.

   .. attribute:: Token.old_value

      A read-only property.  Set to the value the variable had before
      the :meth:`ContextVar.set` method call that created the token.
      It points to :attr:`Token.MISSING` if the variable was not set
      before the call.

   .. attribute:: Token.MISSING

      A marker object used by :attr:`Token.old_value`.


Manual Context Management
-------------------------

.. function:: copy_context()

   Returns a copy of the current :class:`~contextvars.Context` object.

   The following snippet gets a copy of the current context and prints
   all variables and their values that are set in it::

      ctx: Context = copy_context()
      print(list(ctx.items()))

   The function has an *O*\ (1) complexity, i.e. works equally fast for
   contexts with a few context variables and for contexts that have
   a lot of them.


.. class:: Context()

   A mapping of :class:`ContextVars <ContextVar>` to their values.

   ``Context()`` creates an empty context with no values in it.
   To get a copy of the current context use the
   :func:`~contextvars.copy_context` function.

   Each thread has its own effective stack of :class:`!Context` objects.  The
   :term:`current context` is the :class:`!Context` object at the top of the
   current thread's stack.  All :class:`!Context` objects in the stacks are
   considered to be *entered*.

   *Entering* a context, which can be done by calling its :meth:`~Context.run`
   method, makes the context the current context by pushing it onto the top of
   the current thread's context stack.

   *Exiting* from the current context, which can be done by returning from the
   callback passed to the :meth:`~Context.run` method, restores the current
   context to what it was before the context was entered by popping the context
   off the top of the context stack.

   Since each thread has its own context stack, :class:`ContextVar` objects
   behave in a similar fashion to :func:`threading.local` when values are
   assigned in different threads.

   Attempting to enter an already entered context, including contexts entered in
   other threads, raises a :exc:`RuntimeError`.

   After exiting a context, it can later be re-entered (from any thread).

   Any changes to :class:`ContextVar` values via the :meth:`ContextVar.set`
   method are recorded in the current context.  The :meth:`ContextVar.get`
   method returns the value associated with the current context.  Exiting a
   context effectively reverts any changes made to context variables while the
   context was entered (if needed, the values can be restored by re-entering the
   context).

   Context implements the :class:`collections.abc.Mapping` interface.

   .. method:: run(callable, *args, **kwargs)

      Enters the Context, executes ``callable(*args, **kwargs)``, then exits the
      Context.  Returns *callable*'s return value, or propagates an exception if
      one occurred.

      Example:

      .. testcode::

         import contextvars

         var = contextvars.ContextVar('var')
         var.set('spam')
         print(var.get())  # 'spam'

         ctx = contextvars.copy_context()

         def main():
             # 'var' was set to 'spam' before
             # calling 'copy_context()' and 'ctx.run(main)', so:
             print(var.get())  # 'spam'
             print(ctx[var])  # 'spam'

             var.set('ham')

             # Now, after setting 'var' to 'ham':
             print(var.get())  # 'ham'
             print(ctx[var])  # 'ham'

         # Any changes that the 'main' function makes to 'var'
         # will be contained in 'ctx'.
         ctx.run(main)

         # The 'main()' function was run in the 'ctx' context,
         # so changes to 'var' are contained in it:
         print(ctx[var])  # 'ham'

         # However, outside of 'ctx', 'var' is still set to 'spam':
         print(var.get())  # 'spam'

      .. testoutput::
         :hide:

         spam
         spam
         spam
         ham
         ham
         ham
         spam

   .. method:: copy()

      Return a shallow copy of the context object.

   .. describe:: var in context

      Return ``True`` if the *context* has a value for *var* set;
      return ``False`` otherwise.

   .. describe:: context[var]

      Return the value of the *var* :class:`ContextVar` variable.
      If the variable is not set in the context object, a
      :exc:`KeyError` is raised.

   .. method:: get(var, [default])

      Return the value for *var* if *var* has the value in the context
      object.  Return *default* otherwise.  If *default* is not given,
      return ``None``.

   .. describe:: iter(context)

      Return an iterator over the variables stored in the context
      object.

   .. describe:: len(proxy)

      Return the number of variables set in the context object.

   .. method:: keys()

      Return a list of all variables in the context object.

   .. method:: values()

      Return a list of all variables' values in the context object.


   .. method:: items()

      Return a list of 2-tuples containing all variables and their
      values in the context object.


asyncio support
---------------

Context variables are natively supported in :mod:`asyncio` and are
ready to be used without any extra configuration.  For example, here
is a simple echo server, that uses a context variable to make the
address of a remote client available in the Task that handles that
client::

    import asyncio
    import contextvars

    client_addr_var = contextvars.ContextVar('client_addr')

    def render_goodbye():
        # The address of the currently handled client can be accessed
        # without passing it explicitly to this function.

        client_addr = client_addr_var.get()
        return f'Good bye, client @ {client_addr}\r\n'.encode()

    async def handle_request(reader, writer):
        addr = writer.transport.get_extra_info('socket').getpeername()
        client_addr_var.set(addr)

        # In any code that we call is now possible to get
        # client's address by calling 'client_addr_var.get()'.

        while True:
            line = await reader.readline()
            print(line)
            if not line.strip():
                break

        writer.write(b'HTTP/1.1 200 OK\r\n')  # status line
        writer.write(b'\r\n')  # headers
        writer.write(render_goodbye())  # body
        writer.close()

    async def main():
        srv = await asyncio.start_server(
            handle_request, '127.0.0.1', 8081)

        async with srv:
            await srv.serve_forever()

    asyncio.run(main())

    # To test it you can use telnet or curl:
    #     telnet 127.0.0.1 8081
    #     curl 127.0.0.1:8081
