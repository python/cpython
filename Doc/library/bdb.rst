:mod:`bdb` --- Debugger framework
=================================

.. module:: bdb
   :synopsis: Debugger framework.

The :mod:`bdb` module handles basic debugger functions, like setting breakpoints
or managing execution via the debugger.

The following exception is defined:

.. exception:: BdbQuit

   Exception raised by the :class:`Bdb` class for quitting the debugger.


The :mod:`bdb` module also defines two classes:

.. class:: Breakpoint(self, file, line[, temporary=0[, cond=None [, funcname=None]]])

   This class implements temporary breakpoints, ignore counts, disabling and
   (re-)enabling, and conditionals.

   Breakpoints are indexed by number through a list called :attr:`bpbynumber`
   and by ``(file, line)`` pairs through :attr:`bplist`.  The former points to a
   single instance of class :class:`Breakpoint`.  The latter points to a list of
   such instances since there may be more than one breakpoint per line.

   When creating a breakpoint, its associated filename should be in canonical
   form.  If a *funcname* is defined, a breakpoint hit will be counted when the
   first line of that function is executed.  A conditional breakpoint always
   counts a hit.

:class:`Breakpoint` instances have the following methods:

.. method:: Breakpoint.deleteMe()

   Delete the breakpoint from the list associated to a file/line.  If it is the
   last breakpoint in that position, it also deletes the entry for the
   file/line.

.. method:: Breakpoint.enable()

   Mark the breakpoint as enabled.

.. method:: Breakpoint.disable()

   Mark the breakpoint as disabled.

.. method:: Breakpoint.bpprint([out])

   Print all the information about the breakpoint:

   * The breakpoint number.
   * If it is temporary or not.
   * Its file,line position.
   * The condition that causes a break.
   * If it must be ignored the next N times.
   * The breakpoint hit count.


.. class:: Bdb()

   The :class:`Bdb` acts as a generic Python debugger base class.

   This class takes care of the details of the trace facility; a derived class
   should implement user interaction.  The standard debugger class
   (:class:`pdb.Pdb`) is an example.


The following methods of :class:`Bdb` normally don't need to be overridden.

.. method:: Bdb.canonic(filename)

   Auxiliary method for getting a filename in a canonical form, that is, as a
   case-normalized (on case-insensitive filesystems) absolute path, stripped
   of surrounding angle brackets.

.. method:: Bdb.reset()

   Set the :attr:`botframe`, :attr:`stopframe`, :attr:`returnframe` and
   :attr:`quitting` attributes with values ready to start debugging.


.. method:: Bdb.trace_dispatch(frame, event, arg)

   This function is installed as the trace function of debugged frames.  Its
   return value is the new trace function (in most cases, that is, itself).

   The default implementation decides how to dispatch a frame, depending on the
   type of event (passed as a string) that is about to be executed.  *event* can
   be one of the following:

   * ``"line"``: A new line of code is going to be executed.
   * ``"call"``: A function is about to be called, or another code block
     entered.
   * ``"return"``: A function or other code block is about to return.
   * ``"exception"``: An exception has occurred.
   * ``"c_call"``: A C function is about to be called.
   * ``"c_return"``: A C function has returned.
   * ``"c_exception"``: A C function has thrown an exception.

   For the Python events, specialized functions (see below) are called.  For the
   C events, no action is taken.

   The *arg* parameter depends on the previous event.

   For more information on trace functions, see :ref:`debugger-hooks`.  For more
   information on code and frame objects, refer to :ref:`types`.

.. method:: Bdb.dispatch_line(frame)

   If the debugger should stop on the current line, invoke the :meth:`user_line`
   method (which should be overridden in subclasses).  Raise a :exc:`BdbQuit`
   exception if the :attr:`Bdb.quitting` flag is set (which can be set from
   :meth:`user_line`).  Return a reference to the :meth:`trace_dispatch` method
   for further tracing in that scope.

.. method:: Bdb.dispatch_call(frame, arg)

   If the debugger should stop on this function call, invoke the
   :meth:`user_call` method (which should be overridden in subclasses).  Raise a
   :exc:`BdbQuit` exception if the :attr:`Bdb.quitting` flag is set (which can
   be set from :meth:`user_call`).  Return a reference to the
   :meth:`trace_dispatch` method for further tracing in that scope.

.. method:: Bdb.dispatch_return(frame, arg)

   If the debugger should stop on this function return, invoke the
   :meth:`user_return` method (which should be overridden in subclasses).  Raise
   a :exc:`BdbQuit` exception if the :attr:`Bdb.quitting` flag is set (which can
   be set from :meth:`user_return`).  Return a reference to the
   :meth:`trace_dispatch` method for further tracing in that scope.

.. method:: Bdb.dispatch_exception(frame, arg)

   If the debugger should stop at this exception, invokes the
   :meth:`user_exception` method (which should be overridden in subclasses).
   Raise a :exc:`BdbQuit` exception if the :attr:`Bdb.quitting` flag is set
   (which can be set from :meth:`user_exception`).  Return a reference to the
   :meth:`trace_dispatch` method for further tracing in that scope.

Normally derived classes don't override the following methods, but they may if
they want to redefine the definition of stopping and breakpoints.

.. method:: Bdb.stop_here(frame)

   This method checks if the *frame* is somewhere below :attr:`botframe` in the
   call stack.  :attr:`botframe` is the frame in which debugging started.

.. method:: Bdb.break_here(frame)

   This method checks if there is a breakpoint in the filename and line
   belonging to *frame* or, at least, in the current function.  If the
   breakpoint is a temporary one, this method deletes it.

.. method:: Bdb.break_anywhere(frame)

   This method checks if there is a breakpoint in the filename of the current
   frame.

Derived classes should override these methods to gain control over debugger
operation.

.. method:: Bdb.user_call(frame, argument_list)

   This method is called from :meth:`dispatch_call` when there is the
   possibility that a break might be necessary anywhere inside the called
   function.

.. method:: Bdb.user_line(frame)

   This method is called from :meth:`dispatch_line` when either
   :meth:`stop_here` or :meth:`break_here` yields True.

.. method:: Bdb.user_return(frame, return_value)

   This method is called from :meth:`dispatch_return` when :meth:`stop_here`
   yields True.

.. method:: Bdb.user_exception(frame, exc_info)

   This method is called from :meth:`dispatch_exception` when :meth:`stop_here`
   yields True.

.. method:: Bdb.do_clear(arg)

   Handle how a breakpoint must be removed when it is a temporary one.

   This method must be implemented by derived classes.


Derived classes and clients can call the following methods to affect the 
stepping state.

.. method:: Bdb.set_step()

   Stop after one line of code.

.. method:: Bdb.set_next(frame)

   Stop on the next line in or below the given frame.

.. method:: Bdb.set_return(frame)

   Stop when returning from the given frame.

.. method:: Bdb.set_trace([frame])

   Start debugging from *frame*.  If *frame* is not specified, debugging starts
   from caller's frame.

.. method:: Bdb.set_continue()

   Stop only at breakpoints or when finished.  If there are no breakpoints, set
   the system trace function to None.

.. method:: Bdb.set_quit()

   Set the :attr:`quitting` attribute to True.  This raises :exc:`BdbQuit` in
   the next call to one of the :meth:`dispatch_\*` methods.


Derived classes and clients can call the following methods to manipulate
breakpoints.  These methods return a string containing an error message if
something went wrong, or ``None`` if all is well.

.. method:: Bdb.set_break(filename, lineno[, temporary=0[, cond[, funcname]]])

   Set a new breakpoint.  If the *lineno* line doesn't exist for the *filename*
   passed as argument, return an error message.  The *filename* should be in
   canonical form, as described in the :meth:`canonic` method.

.. method:: Bdb.clear_break(filename, lineno)

   Delete the breakpoints in *filename* and *lineno*.  If none were set, an
   error message is returned.

.. method:: Bdb.clear_bpbynumber(arg)

   Delete the breakpoint which has the index *arg* in the
   :attr:`Breakpoint.bpbynumber`.  If `arg` is not numeric or out of range,
   return an error message.

.. method:: Bdb.clear_all_file_breaks(filename)

   Delete all breakpoints in *filename*.  If none were set, an error message is
   returned.

.. method:: Bdb.clear_all_breaks()

   Delete all existing breakpoints.

.. method:: Bdb.get_break(filename, lineno)

   Check if there is a breakpoint for *lineno* of *filename*.

.. method:: Bdb.get_breaks(filename, lineno)

   Return all breakpoints for *lineno* in *filename*, or an empty list if none
   are set.

.. method:: Bdb.get_file_breaks(filename)

   Return all breakpoints in *filename*, or an empty list if none are set.

.. method:: Bdb.get_all_breaks()

   Return all breakpoints that are set.


Derived classes and clients can call the following methods to get a data
structure representing a stack trace.

.. method:: Bdb.get_stack(f, t)

   Get a list of records for a frame and all higher (calling) and lower frames,
   and the size of the higher part.

.. method:: Bdb.format_stack_entry(frame_lineno, [lprefix=': '])

   Return a string with information about a stack entry, identified by a
   ``(frame, lineno)`` tuple:

   * The canonical form of the filename which contains the frame.
   * The function name, or ``"<lambda>"``.
   * The input arguments.
   * The return value.
   * The line of code (if it exists).


The following two methods can be called by clients to use a debugger to debug a
statement, given as a string.

.. method:: Bdb.run(cmd, [globals, [locals]])

   Debug a statement executed via the :keyword:`exec` statement.  *globals*
   defaults to :attr:`__main__.__dict__`, *locals* defaults to *globals*.

.. method:: Bdb.runeval(expr, [globals, [locals]])

   Debug an expression executed via the :func:`eval` function.  *globals* and
   *locals* have the same meaning as in :meth:`run`.

.. method:: Bdb.runctx(cmd, globals, locals)

   For backwards compatibility.  Calls the :meth:`run` method.

.. method:: Bdb.runcall(func, *args, **kwds)

   Debug a single function call, and return its result.


Finally, the module defines the following functions:

.. function:: checkfuncname(b, frame)

   Check whether we should break here, depending on the way the breakpoint *b*
   was set.
   
   If it was set via line number, it checks if ``b.line`` is the same as the one
   in the frame also passed as argument.  If the breakpoint was set via function
   name, we have to check we are in the right frame (the right function) and if
   we are in its first executable line.

.. function:: effective(file, line, frame)

   Determine if there is an effective (active) breakpoint at this line of code.
   Return breakpoint number or 0 if none.
	
   Called only if we know there is a breakpoint at this location.  Returns the
   breakpoint that was triggered and a flag that indicates if it is ok to delete
   a temporary breakpoint.

.. function:: set_trace()

   Starts debugging with a :class:`Bdb` instance from caller's frame.
