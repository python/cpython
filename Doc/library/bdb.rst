:mod:`!bdb` --- Debugger framework
==================================

.. module:: bdb
   :synopsis: Debugger framework.

**Source code:** :source:`Lib/bdb.py`

--------------

The :mod:`bdb` module handles basic debugger functions, like setting breakpoints
or managing execution via the debugger.

The following exception is defined:

.. exception:: BdbQuit

   Exception raised by the :class:`Bdb` class for quitting the debugger.


The :mod:`bdb` module also defines two classes:

.. class:: Breakpoint(self, file, line, temporary=False, cond=None, funcname=None)

   This class implements temporary breakpoints, ignore counts, disabling and
   (re-)enabling, and conditionals.

   Breakpoints are indexed by number through a list called :attr:`bpbynumber`
   and by ``(file, line)`` pairs through :attr:`bplist`.  The former points to
   a single instance of class :class:`Breakpoint`.  The latter points to a list
   of such instances since there may be more than one breakpoint per line.

   When creating a breakpoint, its associated :attr:`file name <file>` should
   be in canonical form.  If a :attr:`funcname` is defined, a breakpoint
   :attr:`hit <hits>` will be counted when the first line of that function is
   executed.  A :attr:`conditional <cond>` breakpoint always counts a
   :attr:`hit <hits>`.

   :class:`Breakpoint` instances have the following methods:

   .. method:: deleteMe()

      Delete the breakpoint from the list associated to a file/line.  If it is
      the last breakpoint in that position, it also deletes the entry for the
      file/line.


   .. method:: enable()

      Mark the breakpoint as enabled.


   .. method:: disable()

      Mark the breakpoint as disabled.


   .. method:: bpformat()

      Return a string with all the information about the breakpoint, nicely
      formatted:

      * Breakpoint number.
      * Temporary status (del or keep).
      * File/line position.
      * Break condition.
      * Number of times to ignore.
      * Number of times hit.

      .. versionadded:: 3.2

   .. method:: bpprint(out=None)

      Print the output of :meth:`bpformat` to the file *out*, or if it is
      ``None``, to standard output.

   :class:`Breakpoint` instances have the following attributes:

   .. attribute:: file

      File name of the :class:`Breakpoint`.

   .. attribute:: line

      Line number of the :class:`Breakpoint` within :attr:`file`.

   .. attribute:: temporary

      ``True`` if a :class:`Breakpoint` at (file, line) is temporary.

   .. attribute:: cond

      Condition for evaluating a :class:`Breakpoint` at (file, line).

   .. attribute:: funcname

      Function name that defines whether a :class:`Breakpoint` is hit upon
      entering the function.

   .. attribute:: enabled

      ``True`` if :class:`Breakpoint` is enabled.

   .. attribute:: bpbynumber

      Numeric index for a single instance of a :class:`Breakpoint`.

   .. attribute:: bplist

      Dictionary of :class:`Breakpoint` instances indexed by
      (:attr:`file`, :attr:`line`) tuples.

   .. attribute:: ignore

      Number of times to ignore a :class:`Breakpoint`.

   .. attribute:: hits

      Count of the number of times a :class:`Breakpoint` has been hit.

.. class:: Bdb(skip=None, backend='settrace')

   The :class:`Bdb` class acts as a generic Python debugger base class.

   This class takes care of the details of the trace facility; a derived class
   should implement user interaction.  The standard debugger class
   (:class:`pdb.Pdb`) is an example.

   The *skip* argument, if given, must be an iterable of glob-style
   module name patterns.  The debugger will not step into frames that
   originate in a module that matches one of these patterns. Whether a
   frame is considered to originate in a certain module is determined
   by the ``__name__`` in the frame globals.

   The *backend* argument specifies the backend to use for :class:`Bdb`. It
   can be either ``'settrace'`` or ``'monitoring'``. ``'settrace'`` uses
   :func:`sys.settrace` which has the best backward compatibility. The
   ``'monitoring'`` backend uses the new :mod:`sys.monitoring` that was
   introduced in Python 3.12, which can be much more efficient because it
   can disable unused events. We are trying to keep the exact interfaces
   for both backends, but there are some differences. The debugger developers
   are encouraged to use the ``'monitoring'`` backend to achieve better
   performance.

   .. versionchanged:: 3.1
      Added the *skip* parameter.

   .. versionchanged:: 3.14
      Added the *backend* parameter.

   The following methods of :class:`Bdb` normally don't need to be overridden.

   .. method:: canonic(filename)

      Return canonical form of *filename*.

      For real file names, the canonical form is an operating-system-dependent,
      :func:`case-normalized <os.path.normcase>` :func:`absolute path
      <os.path.abspath>`. A *filename* with angle brackets, such as ``"<stdin>"``
      generated in interactive mode, is returned unchanged.

   .. method:: start_trace(self)

      Start tracing. For ``'settrace'`` backend, this method is equivalent to
      ``sys.settrace(self.trace_dispatch)``

      .. versionadded:: 3.14

   .. method:: stop_trace(self)

      Stop tracing. For ``'settrace'`` backend, this method is equivalent to
      ``sys.settrace(None)``

      .. versionadded:: 3.14

   .. method:: reset()

      Set the :attr:`!botframe`, :attr:`!stopframe`, :attr:`!returnframe` and
      :attr:`quitting <Bdb.set_quit>` attributes with values ready to start debugging.

   .. method:: trace_dispatch(frame, event, arg)

      This function is installed as the trace function of debugged frames.  Its
      return value is the new trace function (in most cases, that is, itself).

      The default implementation decides how to dispatch a frame, depending on
      the type of event (passed as a string) that is about to be executed.
      *event* can be one of the following:

      * ``"line"``: A new line of code is going to be executed.
      * ``"call"``: A function is about to be called, or another code block
        entered.
      * ``"return"``: A function or other code block is about to return.
      * ``"exception"``: An exception has occurred.
      * ``"c_call"``: A C function is about to be called.
      * ``"c_return"``: A C function has returned.
      * ``"c_exception"``: A C function has raised an exception.

      For the Python events, specialized functions (see below) are called.  For
      the C events, no action is taken.

      The *arg* parameter depends on the previous event.

      See the documentation for :func:`sys.settrace` for more information on the
      trace function.  For more information on code and frame objects, refer to
      :ref:`types`.

   .. method:: dispatch_line(frame)

      If the debugger should stop on the current line, invoke the
      :meth:`user_line` method (which should be overridden in subclasses).
      Raise a :exc:`BdbQuit` exception if the :attr:`quitting  <Bdb.set_quit>` flag is set
      (which can be set from :meth:`user_line`).  Return a reference to the
      :meth:`trace_dispatch` method for further tracing in that scope.

   .. method:: dispatch_call(frame, arg)

      If the debugger should stop on this function call, invoke the
      :meth:`user_call` method (which should be overridden in subclasses).
      Raise a :exc:`BdbQuit` exception if the :attr:`quitting  <Bdb.set_quit>` flag is set
      (which can be set from :meth:`user_call`).  Return a reference to the
      :meth:`trace_dispatch` method for further tracing in that scope.

   .. method:: dispatch_return(frame, arg)

      If the debugger should stop on this function return, invoke the
      :meth:`user_return` method (which should be overridden in subclasses).
      Raise a :exc:`BdbQuit` exception if the :attr:`quitting  <Bdb.set_quit>` flag is set
      (which can be set from :meth:`user_return`).  Return a reference to the
      :meth:`trace_dispatch` method for further tracing in that scope.

   .. method:: dispatch_exception(frame, arg)

      If the debugger should stop at this exception, invokes the
      :meth:`user_exception` method (which should be overridden in subclasses).
      Raise a :exc:`BdbQuit` exception if the :attr:`quitting  <Bdb.set_quit>` flag is set
      (which can be set from :meth:`user_exception`).  Return a reference to the
      :meth:`trace_dispatch` method for further tracing in that scope.

   Normally derived classes don't override the following methods, but they may
   if they want to redefine the definition of stopping and breakpoints.

   .. method:: is_skipped_line(module_name)

      Return ``True`` if *module_name* matches any skip pattern.

   .. method:: stop_here(frame)

      Return ``True`` if *frame* is below the starting frame in the stack.

   .. method:: break_here(frame)

      Return ``True`` if there is an effective breakpoint for this line.

      Check whether a line or function breakpoint exists and is in effect.  Delete temporary
      breakpoints based on information from :func:`effective`.

   .. method:: break_anywhere(frame)

      Return ``True`` if any breakpoint exists for *frame*'s filename.

   Derived classes should override these methods to gain control over debugger
   operation.

   .. method:: user_call(frame, argument_list)

      Called from :meth:`dispatch_call` if a break might stop inside the
      called function.

      *argument_list* is not used anymore and will always be ``None``.
      The argument is kept for backwards compatibility.

   .. method:: user_line(frame)

      Called from :meth:`dispatch_line` when either :meth:`stop_here` or
      :meth:`break_here` returns ``True``.

   .. method:: user_return(frame, return_value)

      Called from :meth:`dispatch_return` when :meth:`stop_here` returns ``True``.

   .. method:: user_exception(frame, exc_info)

      Called from :meth:`dispatch_exception` when :meth:`stop_here`
      returns ``True``.

   .. method:: do_clear(arg)

      Handle how a breakpoint must be removed when it is a temporary one.

      This method must be implemented by derived classes.


   Derived classes and clients can call the following methods to affect the
   stepping state.

   .. method:: set_step()

      Stop after one line of code.

   .. method:: set_next(frame)

      Stop on the next line in or below the given frame.

   .. method:: set_return(frame)

      Stop when returning from the given frame.

   .. method:: set_until(frame, lineno=None)

      Stop when the line with the *lineno* greater than the current one is
      reached or when returning from current frame.

   .. method:: set_trace([frame])

      Start debugging from *frame*.  If *frame* is not specified, debugging
      starts from caller's frame.

      .. versionchanged:: 3.13
         :func:`set_trace` will enter the debugger immediately, rather than
         on the next line of code to be executed.

   .. method:: set_continue()

      Stop only at breakpoints or when finished.  If there are no breakpoints,
      set the system trace function to ``None``.

   .. method:: set_quit()

      .. index:: single: quitting (bdb.Bdb attribute)

      Set the :attr:`!quitting` attribute to ``True``.  This raises :exc:`BdbQuit` in
      the next call to one of the :meth:`!dispatch_\*` methods.


   Derived classes and clients can call the following methods to manipulate
   breakpoints.  These methods return a string containing an error message if
   something went wrong, or ``None`` if all is well.

   .. method:: set_break(filename, lineno, temporary=False, cond=None, funcname=None)

      Set a new breakpoint.  If the *lineno* line doesn't exist for the
      *filename* passed as argument, return an error message.  The *filename*
      should be in canonical form, as described in the :meth:`canonic` method.

   .. method:: clear_break(filename, lineno)

      Delete the breakpoints in *filename* and *lineno*.  If none were set,
      return an error message.

   .. method:: clear_bpbynumber(arg)

      Delete the breakpoint which has the index *arg* in the
      :attr:`Breakpoint.bpbynumber`.  If *arg* is not numeric or out of range,
      return an error message.

   .. method:: clear_all_file_breaks(filename)

      Delete all breakpoints in *filename*.  If none were set, return an error
      message.

   .. method:: clear_all_breaks()

      Delete all existing breakpoints.  If none were set, return an error
      message.

   .. method:: get_bpbynumber(arg)

      Return a breakpoint specified by the given number.  If *arg* is a string,
      it will be converted to a number.  If *arg* is a non-numeric string, if
      the given breakpoint never existed or has been deleted, a
      :exc:`ValueError` is raised.

      .. versionadded:: 3.2

   .. method:: get_break(filename, lineno)

      Return ``True`` if there is a breakpoint for *lineno* in *filename*.

   .. method:: get_breaks(filename, lineno)

      Return all breakpoints for *lineno* in *filename*, or an empty list if
      none are set.

   .. method:: get_file_breaks(filename)

      Return all breakpoints in *filename*, or an empty list if none are set.

   .. method:: get_all_breaks()

      Return all breakpoints that are set.


   Derived classes and clients can call the following methods to disable and
   restart events to achieve better performance. These methods only work
   when using the ``'monitoring'`` backend.

   .. method:: disable_current_event()

      Disable the current event until the next time :func:`restart_events` is
      called. This is helpful when the debugger is not interested in the current
      line.

      .. versionadded:: 3.14

   .. method:: restart_events()

      Restart all the disabled events. This function is automatically called in
      ``dispatch_*`` methods after ``user_*`` methods are called. If the
      ``dispatch_*`` methods are not overridden, the disabled events will be
      restarted after each user interaction.

      .. versionadded:: 3.14


   Derived classes and clients can call the following methods to get a data
   structure representing a stack trace.

   .. method:: get_stack(f, t)

      Return a list of (frame, lineno) tuples in a stack trace, and a size.

      The most recently called frame is last in the list. The size is the number
      of frames below the frame where the debugger was invoked.

   .. method:: format_stack_entry(frame_lineno, lprefix=': ')

      Return a string with information about a stack entry, which is a
      ``(frame, lineno)`` tuple.  The return string contains:

      * The canonical filename which contains the frame.
      * The function name or ``"<lambda>"``.
      * The input arguments.
      * The return value.
      * The line of code (if it exists).


   The following two methods can be called by clients to use a debugger to debug
   a :term:`statement`, given as a string.

   .. method:: run(cmd, globals=None, locals=None)

      Debug a statement executed via the :func:`exec` function.  *globals*
      defaults to :attr:`!__main__.__dict__`, *locals* defaults to *globals*.

   .. method:: runeval(expr, globals=None, locals=None)

      Debug an expression executed via the :func:`eval` function.  *globals* and
      *locals* have the same meaning as in :meth:`run`.

   .. method:: runctx(cmd, globals, locals)

      For backwards compatibility.  Calls the :meth:`run` method.

   .. method:: runcall(func, /, *args, **kwds)

      Debug a single function call, and return its result.


Finally, the module defines the following functions:

.. function:: checkfuncname(b, frame)

   Return ``True`` if we should break here, depending on the way the
   :class:`Breakpoint` *b* was set.

   If it was set via line number, it checks if
   :attr:`b.line <bdb.Breakpoint.line>` is the same as the one in *frame*.
   If the breakpoint was set via
   :attr:`function name <bdb.Breakpoint.funcname>`, we have to check we are in
   the right *frame* (the right function) and if we are on its first executable
   line.

.. function:: effective(file, line, frame)

   Return ``(active breakpoint, delete temporary flag)`` or ``(None, None)`` as the
   breakpoint to act upon.

   The *active breakpoint* is the first entry in
   :attr:`bplist <bdb.Breakpoint.bplist>` for the
   (:attr:`file <bdb.Breakpoint.file>`, :attr:`line <bdb.Breakpoint.line>`)
   (which must exist) that is :attr:`enabled <bdb.Breakpoint.enabled>`, for
   which :func:`checkfuncname` is true, and that has neither a false
   :attr:`condition <bdb.Breakpoint.cond>` nor positive
   :attr:`ignore <bdb.Breakpoint.ignore>` count.  The *flag*, meaning that a
   temporary breakpoint should be deleted, is ``False`` only when the
   :attr:`cond <bdb.Breakpoint.cond>` cannot be evaluated (in which case,
   :attr:`ignore <bdb.Breakpoint.ignore>` count is ignored).

   If no such entry exists, then ``(None, None)`` is returned.


.. function:: set_trace()

   Start debugging with a :class:`Bdb` instance from caller's frame.
