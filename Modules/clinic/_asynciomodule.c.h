/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_asyncio_Future___init____doc__,
"Future(*, loop=None)\n"
"--\n"
"\n"
"This class is *almost* compatible with concurrent.futures.Future.\n"
"\n"
"    Differences:\n"
"\n"
"    - result() and exception() do not take a timeout argument and\n"
"      raise an exception when the future isn\'t done yet.\n"
"\n"
"    - Callbacks registered with add_done_callback() are always called\n"
"      via the event loop\'s call_soon_threadsafe().\n"
"\n"
"    - This class is not compatible with the wait() and as_completed()\n"
"      methods in the concurrent.futures package.");

static int
_asyncio_Future___init___impl(FutureObj *self, PyObject *loop);

static int
_asyncio_Future___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static const char * const _keywords[] = {"loop", NULL};
    static _PyArg_Parser _parser = {"|$O:Future", _keywords, 0};
    PyObject *loop = NULL;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &loop)) {
        goto exit;
    }
    return_value = _asyncio_Future___init___impl((FutureObj *)self, loop);

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio_Future_result__doc__,
"result($self, /)\n"
"--\n"
"\n"
"Return the result this future represents.\n"
"\n"
"If the future has been cancelled, raises CancelledError.  If the\n"
"future\'s result isn\'t yet available, raises InvalidStateError.  If\n"
"the future is done and has an exception set, this exception is raised.");

#define _ASYNCIO_FUTURE_RESULT_METHODDEF    \
    {"result", (PyCFunction)_asyncio_Future_result, METH_NOARGS, _asyncio_Future_result__doc__},

static PyObject *
_asyncio_Future_result_impl(FutureObj *self);

static PyObject *
_asyncio_Future_result(FutureObj *self, PyObject *Py_UNUSED(ignored))
{
    return _asyncio_Future_result_impl(self);
}

PyDoc_STRVAR(_asyncio_Future_exception__doc__,
"exception($self, /)\n"
"--\n"
"\n"
"Return the exception that was set on this future.\n"
"\n"
"The exception (or None if no exception was set) is returned only if\n"
"the future is done.  If the future has been cancelled, raises\n"
"CancelledError.  If the future isn\'t done yet, raises\n"
"InvalidStateError.");

#define _ASYNCIO_FUTURE_EXCEPTION_METHODDEF    \
    {"exception", (PyCFunction)_asyncio_Future_exception, METH_NOARGS, _asyncio_Future_exception__doc__},

static PyObject *
_asyncio_Future_exception_impl(FutureObj *self);

static PyObject *
_asyncio_Future_exception(FutureObj *self, PyObject *Py_UNUSED(ignored))
{
    return _asyncio_Future_exception_impl(self);
}

PyDoc_STRVAR(_asyncio_Future_set_result__doc__,
"set_result($self, res, /)\n"
"--\n"
"\n"
"Mark the future done and set its result.\n"
"\n"
"If the future is already done when this method is called, raises\n"
"InvalidStateError.");

#define _ASYNCIO_FUTURE_SET_RESULT_METHODDEF    \
    {"set_result", (PyCFunction)_asyncio_Future_set_result, METH_O, _asyncio_Future_set_result__doc__},

PyDoc_STRVAR(_asyncio_Future_set_exception__doc__,
"set_exception($self, exception, /)\n"
"--\n"
"\n"
"Mark the future done and set an exception.\n"
"\n"
"If the future is already done when this method is called, raises\n"
"InvalidStateError.");

#define _ASYNCIO_FUTURE_SET_EXCEPTION_METHODDEF    \
    {"set_exception", (PyCFunction)_asyncio_Future_set_exception, METH_O, _asyncio_Future_set_exception__doc__},

PyDoc_STRVAR(_asyncio_Future_add_done_callback__doc__,
"add_done_callback($self, fn, /)\n"
"--\n"
"\n"
"Add a callback to be run when the future becomes done.\n"
"\n"
"The callback is called with a single argument - the future object. If\n"
"the future is already done when this is called, the callback is\n"
"scheduled with call_soon.");

#define _ASYNCIO_FUTURE_ADD_DONE_CALLBACK_METHODDEF    \
    {"add_done_callback", (PyCFunction)_asyncio_Future_add_done_callback, METH_O, _asyncio_Future_add_done_callback__doc__},

PyDoc_STRVAR(_asyncio_Future_remove_done_callback__doc__,
"remove_done_callback($self, fn, /)\n"
"--\n"
"\n"
"Remove all instances of a callback from the \"call when done\" list.\n"
"\n"
"Returns the number of callbacks removed.");

#define _ASYNCIO_FUTURE_REMOVE_DONE_CALLBACK_METHODDEF    \
    {"remove_done_callback", (PyCFunction)_asyncio_Future_remove_done_callback, METH_O, _asyncio_Future_remove_done_callback__doc__},

PyDoc_STRVAR(_asyncio_Future_cancel__doc__,
"cancel($self, /)\n"
"--\n"
"\n"
"Cancel the future and schedule callbacks.\n"
"\n"
"If the future is already done or cancelled, return False.  Otherwise,\n"
"change the future\'s state to cancelled, schedule the callbacks and\n"
"return True.");

#define _ASYNCIO_FUTURE_CANCEL_METHODDEF    \
    {"cancel", (PyCFunction)_asyncio_Future_cancel, METH_NOARGS, _asyncio_Future_cancel__doc__},

static PyObject *
_asyncio_Future_cancel_impl(FutureObj *self);

static PyObject *
_asyncio_Future_cancel(FutureObj *self, PyObject *Py_UNUSED(ignored))
{
    return _asyncio_Future_cancel_impl(self);
}

PyDoc_STRVAR(_asyncio_Future_cancelled__doc__,
"cancelled($self, /)\n"
"--\n"
"\n"
"Return True if the future was cancelled.");

#define _ASYNCIO_FUTURE_CANCELLED_METHODDEF    \
    {"cancelled", (PyCFunction)_asyncio_Future_cancelled, METH_NOARGS, _asyncio_Future_cancelled__doc__},

static PyObject *
_asyncio_Future_cancelled_impl(FutureObj *self);

static PyObject *
_asyncio_Future_cancelled(FutureObj *self, PyObject *Py_UNUSED(ignored))
{
    return _asyncio_Future_cancelled_impl(self);
}

PyDoc_STRVAR(_asyncio_Future_done__doc__,
"done($self, /)\n"
"--\n"
"\n"
"Return True if the future is done.\n"
"\n"
"Done means either that a result / exception are available, or that the\n"
"future was cancelled.");

#define _ASYNCIO_FUTURE_DONE_METHODDEF    \
    {"done", (PyCFunction)_asyncio_Future_done, METH_NOARGS, _asyncio_Future_done__doc__},

static PyObject *
_asyncio_Future_done_impl(FutureObj *self);

static PyObject *
_asyncio_Future_done(FutureObj *self, PyObject *Py_UNUSED(ignored))
{
    return _asyncio_Future_done_impl(self);
}

PyDoc_STRVAR(_asyncio_Future__repr_info__doc__,
"_repr_info($self, /)\n"
"--\n"
"\n");

#define _ASYNCIO_FUTURE__REPR_INFO_METHODDEF    \
    {"_repr_info", (PyCFunction)_asyncio_Future__repr_info, METH_NOARGS, _asyncio_Future__repr_info__doc__},

static PyObject *
_asyncio_Future__repr_info_impl(FutureObj *self);

static PyObject *
_asyncio_Future__repr_info(FutureObj *self, PyObject *Py_UNUSED(ignored))
{
    return _asyncio_Future__repr_info_impl(self);
}

PyDoc_STRVAR(_asyncio_Future__schedule_callbacks__doc__,
"_schedule_callbacks($self, /)\n"
"--\n"
"\n");

#define _ASYNCIO_FUTURE__SCHEDULE_CALLBACKS_METHODDEF    \
    {"_schedule_callbacks", (PyCFunction)_asyncio_Future__schedule_callbacks, METH_NOARGS, _asyncio_Future__schedule_callbacks__doc__},

static PyObject *
_asyncio_Future__schedule_callbacks_impl(FutureObj *self);

static PyObject *
_asyncio_Future__schedule_callbacks(FutureObj *self, PyObject *Py_UNUSED(ignored))
{
    return _asyncio_Future__schedule_callbacks_impl(self);
}

PyDoc_STRVAR(_asyncio_Task___init____doc__,
"Task(coro, *, loop=None)\n"
"--\n"
"\n"
"A coroutine wrapped in a Future.");

static int
_asyncio_Task___init___impl(TaskObj *self, PyObject *coro, PyObject *loop);

static int
_asyncio_Task___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static const char * const _keywords[] = {"coro", "loop", NULL};
    static _PyArg_Parser _parser = {"O|$O:Task", _keywords, 0};
    PyObject *coro;
    PyObject *loop = NULL;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &coro, &loop)) {
        goto exit;
    }
    return_value = _asyncio_Task___init___impl((TaskObj *)self, coro, loop);

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio_Task_current_task__doc__,
"current_task($type, /, loop=None)\n"
"--\n"
"\n"
"Return the currently running task in an event loop or None.\n"
"\n"
"By default the current task for the current event loop is returned.\n"
"\n"
"None is returned when called not in the context of a Task.");

#define _ASYNCIO_TASK_CURRENT_TASK_METHODDEF    \
    {"current_task", (PyCFunction)_asyncio_Task_current_task, METH_FASTCALL|METH_CLASS, _asyncio_Task_current_task__doc__},

static PyObject *
_asyncio_Task_current_task_impl(PyTypeObject *type, PyObject *loop);

static PyObject *
_asyncio_Task_current_task(PyTypeObject *type, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"loop", NULL};
    static _PyArg_Parser _parser = {"|O:current_task", _keywords, 0};
    PyObject *loop = Py_None;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &loop)) {
        goto exit;
    }
    return_value = _asyncio_Task_current_task_impl(type, loop);

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio_Task_all_tasks__doc__,
"all_tasks($type, /, loop=None)\n"
"--\n"
"\n"
"Return a set of all tasks for an event loop.\n"
"\n"
"By default all tasks for the current event loop are returned.");

#define _ASYNCIO_TASK_ALL_TASKS_METHODDEF    \
    {"all_tasks", (PyCFunction)_asyncio_Task_all_tasks, METH_FASTCALL|METH_CLASS, _asyncio_Task_all_tasks__doc__},

static PyObject *
_asyncio_Task_all_tasks_impl(PyTypeObject *type, PyObject *loop);

static PyObject *
_asyncio_Task_all_tasks(PyTypeObject *type, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"loop", NULL};
    static _PyArg_Parser _parser = {"|O:all_tasks", _keywords, 0};
    PyObject *loop = Py_None;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &loop)) {
        goto exit;
    }
    return_value = _asyncio_Task_all_tasks_impl(type, loop);

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio_Task__repr_info__doc__,
"_repr_info($self, /)\n"
"--\n"
"\n");

#define _ASYNCIO_TASK__REPR_INFO_METHODDEF    \
    {"_repr_info", (PyCFunction)_asyncio_Task__repr_info, METH_NOARGS, _asyncio_Task__repr_info__doc__},

static PyObject *
_asyncio_Task__repr_info_impl(TaskObj *self);

static PyObject *
_asyncio_Task__repr_info(TaskObj *self, PyObject *Py_UNUSED(ignored))
{
    return _asyncio_Task__repr_info_impl(self);
}

PyDoc_STRVAR(_asyncio_Task_cancel__doc__,
"cancel($self, /)\n"
"--\n"
"\n"
"Request that this task cancel itself.\n"
"\n"
"This arranges for a CancelledError to be thrown into the\n"
"wrapped coroutine on the next cycle through the event loop.\n"
"The coroutine then has a chance to clean up or even deny\n"
"the request using try/except/finally.\n"
"\n"
"Unlike Future.cancel, this does not guarantee that the\n"
"task will be cancelled: the exception might be caught and\n"
"acted upon, delaying cancellation of the task or preventing\n"
"cancellation completely.  The task may also return a value or\n"
"raise a different exception.\n"
"\n"
"Immediately after this method is called, Task.cancelled() will\n"
"not return True (unless the task was already cancelled).  A\n"
"task will be marked as cancelled when the wrapped coroutine\n"
"terminates with a CancelledError exception (even if cancel()\n"
"was not called).");

#define _ASYNCIO_TASK_CANCEL_METHODDEF    \
    {"cancel", (PyCFunction)_asyncio_Task_cancel, METH_NOARGS, _asyncio_Task_cancel__doc__},

static PyObject *
_asyncio_Task_cancel_impl(TaskObj *self);

static PyObject *
_asyncio_Task_cancel(TaskObj *self, PyObject *Py_UNUSED(ignored))
{
    return _asyncio_Task_cancel_impl(self);
}

PyDoc_STRVAR(_asyncio_Task_get_stack__doc__,
"get_stack($self, /, *, limit=None)\n"
"--\n"
"\n"
"Return the list of stack frames for this task\'s coroutine.\n"
"\n"
"If the coroutine is not done, this returns the stack where it is\n"
"suspended.  If the coroutine has completed successfully or was\n"
"cancelled, this returns an empty list.  If the coroutine was\n"
"terminated by an exception, this returns the list of traceback\n"
"frames.\n"
"\n"
"The frames are always ordered from oldest to newest.\n"
"\n"
"The optional limit gives the maximum number of frames to\n"
"return; by default all available frames are returned.  Its\n"
"meaning differs depending on whether a stack or a traceback is\n"
"returned: the newest frames of a stack are returned, but the\n"
"oldest frames of a traceback are returned.  (This matches the\n"
"behavior of the traceback module.)\n"
"\n"
"For reasons beyond our control, only one stack frame is\n"
"returned for a suspended coroutine.");

#define _ASYNCIO_TASK_GET_STACK_METHODDEF    \
    {"get_stack", (PyCFunction)_asyncio_Task_get_stack, METH_FASTCALL, _asyncio_Task_get_stack__doc__},

static PyObject *
_asyncio_Task_get_stack_impl(TaskObj *self, PyObject *limit);

static PyObject *
_asyncio_Task_get_stack(TaskObj *self, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"limit", NULL};
    static _PyArg_Parser _parser = {"|$O:get_stack", _keywords, 0};
    PyObject *limit = Py_None;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &limit)) {
        goto exit;
    }
    return_value = _asyncio_Task_get_stack_impl(self, limit);

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio_Task_print_stack__doc__,
"print_stack($self, /, *, limit=None, file=None)\n"
"--\n"
"\n"
"Print the stack or traceback for this task\'s coroutine.\n"
"\n"
"This produces output similar to that of the traceback module,\n"
"for the frames retrieved by get_stack().  The limit argument\n"
"is passed to get_stack().  The file argument is an I/O stream\n"
"to which the output is written; by default output is written\n"
"to sys.stderr.");

#define _ASYNCIO_TASK_PRINT_STACK_METHODDEF    \
    {"print_stack", (PyCFunction)_asyncio_Task_print_stack, METH_FASTCALL, _asyncio_Task_print_stack__doc__},

static PyObject *
_asyncio_Task_print_stack_impl(TaskObj *self, PyObject *limit,
                               PyObject *file);

static PyObject *
_asyncio_Task_print_stack(TaskObj *self, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"limit", "file", NULL};
    static _PyArg_Parser _parser = {"|$OO:print_stack", _keywords, 0};
    PyObject *limit = Py_None;
    PyObject *file = Py_None;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &limit, &file)) {
        goto exit;
    }
    return_value = _asyncio_Task_print_stack_impl(self, limit, file);

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio_Task__step__doc__,
"_step($self, /, exc=None)\n"
"--\n"
"\n");

#define _ASYNCIO_TASK__STEP_METHODDEF    \
    {"_step", (PyCFunction)_asyncio_Task__step, METH_FASTCALL, _asyncio_Task__step__doc__},

static PyObject *
_asyncio_Task__step_impl(TaskObj *self, PyObject *exc);

static PyObject *
_asyncio_Task__step(TaskObj *self, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"exc", NULL};
    static _PyArg_Parser _parser = {"|O:_step", _keywords, 0};
    PyObject *exc = NULL;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &exc)) {
        goto exit;
    }
    return_value = _asyncio_Task__step_impl(self, exc);

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio_Task__wakeup__doc__,
"_wakeup($self, /, fut)\n"
"--\n"
"\n");

#define _ASYNCIO_TASK__WAKEUP_METHODDEF    \
    {"_wakeup", (PyCFunction)_asyncio_Task__wakeup, METH_FASTCALL, _asyncio_Task__wakeup__doc__},

static PyObject *
_asyncio_Task__wakeup_impl(TaskObj *self, PyObject *fut);

static PyObject *
_asyncio_Task__wakeup(TaskObj *self, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fut", NULL};
    static _PyArg_Parser _parser = {"O:_wakeup", _keywords, 0};
    PyObject *fut;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &fut)) {
        goto exit;
    }
    return_value = _asyncio_Task__wakeup_impl(self, fut);

exit:
    return return_value;
}
/*[clinic end generated code: output=40ca6c9da517da73 input=a9049054013a1b77]*/
