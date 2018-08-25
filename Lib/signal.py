import _signal
from _signal import *
from functools import wraps as _wraps
from enum import IntEnum as _IntEnum

_globals = globals()

_IntEnum._convert(
        'Signals', __name__,
        lambda name:
            name.isupper()
            and (name.startswith('SIG') and not name.startswith('SIG_'))
            or name.startswith('CTRL_'))

if 'pthread_sigmask' in _globals:
    _IntEnum._convert(
            'Sigmasks', __name__,
            lambda name: name in ('SIG_BLOCK', 'SIG_UNBLOCK', 'SIG_SETMASK'))


def _int_to_enum(value, enum_klass):
    """Convert a numeric value to an IntEnum member.
    If it's not a known member, return the numeric value itself.
    """
    try:
        return enum_klass(value)
    except ValueError:
        return value


if 'pthread_sigmask' in _globals:
    @_wraps(_signal.pthread_sigmask)
    def pthread_sigmask(how, mask):
        sigs_set = _signal.pthread_sigmask(how, mask)
        return set(_int_to_enum(x, Signals) for x in sigs_set)
    pthread_sigmask.__doc__ = _signal.pthread_sigmask.__doc__


if 'sigpending' in _globals:
    @_wraps(_signal.sigpending)
    def sigpending():
        return {_int_to_enum(x, Signals) for x in _signal.sigpending()}


if 'sigwait' in _globals:
    @_wraps(_signal.sigwait)
    def sigwait(sigset):
        retsig = _signal.sigwait(sigset)
        return _int_to_enum(retsig, Signals)
    sigwait.__doc__ = _signal.sigwait


if 'valid_signals' in _globals:
    @_wraps(_signal.valid_signals)
    def valid_signals():
        return {_int_to_enum(x, Signals) for x in _signal.valid_signals()}


del _globals, _wraps
