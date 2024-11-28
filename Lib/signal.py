import _signal
import os
from _signal import *
from enum import IntEnum as _IntEnum
import threading
import queue

_globals = globals()

_IntEnum._convert_(
        'Signals', __name__,
        lambda name:
            name.isupper()
            and (name.startswith('SIG') and not name.startswith('SIG_'))
            or name.startswith('CTRL_'))

_IntEnum._convert_(
        'Handlers', __name__,
        lambda name: name in ('SIG_DFL', 'SIG_IGN'))

if 'pthread_sigmask' in _globals:
    _IntEnum._convert_(
            'Sigmasks', __name__,
            lambda name: name in ('SIG_BLOCK', 'SIG_UNBLOCK', 'SIG_SETMASK'))


def _int_to_enum(value, enum_klass):
    """Convert a possible numeric value to an IntEnum member.
    If it's not a known member, return the value itself.
    """
    if not isinstance(value, int):
        return value
    try:
        return enum_klass(value)
    except ValueError:
        return value


def _enum_to_int(value):
    """Convert an IntEnum member to a numeric value.
    If it's not an IntEnum member return the value itself.
    """
    try:
        return int(value)
    except (ValueError, TypeError):
        return value

_init_lock = threading.Lock()
_signal_queue = queue.SimpleQueue() # SimpleQueue has reentrant put, so it can safely be called from signal handlers. https://github.com/python/cpython/issues/59181
_bubble_queue = queue.SimpleQueue()
_signal_thread = None
_signo_to_handler = {}

def _signal_queue_handler():
    assert threading.current_thread() is not threading.main_thread()
    global _signal_queue, _signo_to_handler, _bubble_queue
    while True:
        (signo, stack_frame) = _signal_queue.get()
        try:
            handler = _signo_to_handler.get(signo, None)
            handler(signo, stack_frame)
        except Exception as e:
            _bubble_queue.put(e)
            # _signal.raise_signal(SIGTERM) # does not work when using event.wait()
            # _thread.interrupt_main(SIGTERM) # does not work when using event.wait()
            os.kill(os.getpid(), signo)

def _init_signal_thread():
    assert threading.current_thread() is threading.main_thread()
    global _signal_thread, _init_lock
    with _init_lock:
        if _signal_thread is None:
            _signal_thread = threading.Thread(target=_signal_queue_handler, daemon=True)
            _signal_thread.start()

def _push_signal_to_queue_handler(signo, stack_frame):
    assert threading.current_thread() is threading.main_thread()
    try:
        global _bubble_queue
        bubble_exception = _bubble_queue.get(block=False)
        raise bubble_exception
    except queue.Empty:
        global _signal_queue
        _signal_queue.put((signo, stack_frame))

# Similar to functools.wraps(), but only assign __doc__.
# __module__ should be preserved,
# __name__ and __qualname__ are already fine,
# __annotations__ is not set.
def _wraps(wrapped):
    def decorator(wrapper):
        wrapper.__doc__ = wrapped.__doc__
        return wrapper
    return decorator

def signal(signalnum, handler, use_dedicated_thread=True):
    assert threading.current_thread() is threading.main_thread()
    global _signo_to_handler
    signal_int = _enum_to_int(signalnum)
    old_handler = _signo_to_handler.get(signal_int, None)
    if use_dedicated_thread and callable(handler):
        assert callable(handler)
        global _signal_thread
        if _signal_thread is None:
            _init_signal_thread()
        _signo_to_handler[signal_int] = handler
        handler = _signal.signal(signal_int, _enum_to_int(_push_signal_to_queue_handler))
        return old_handler or _int_to_enum(handler, Handlers)
    else:
        if signal_int in _signo_to_handler:
            del _signo_to_handler[signal_int]
        handler = _signal.signal(signal_int, _enum_to_int(handler))
        return old_handler or _int_to_enum(handler, Handlers)


@_wraps(_signal.getsignal)
def getsignal(signalnum):
    global _signo_to_handler
    if signalnum in _signo_to_handler:
        return _signo_to_handler[signalnum]
    else:
        handler = _signal.getsignal(signalnum)
        return _int_to_enum(handler, Handlers)


if 'pthread_sigmask' in _globals:
    @_wraps(_signal.pthread_sigmask)
    def pthread_sigmask(how, mask):
        sigs_set = _signal.pthread_sigmask(how, mask)
        return set(_int_to_enum(x, Signals) for x in sigs_set)


if 'sigpending' in _globals:
    @_wraps(_signal.sigpending)
    def sigpending():
        return {_int_to_enum(x, Signals) for x in _signal.sigpending()}


if 'sigwait' in _globals:
    @_wraps(_signal.sigwait)
    def sigwait(sigset):
        retsig = _signal.sigwait(sigset)
        return _int_to_enum(retsig, Signals)


if 'valid_signals' in _globals:
    @_wraps(_signal.valid_signals)
    def valid_signals():
        return {_int_to_enum(x, Signals) for x in _signal.valid_signals()}


del _globals, _wraps
