"""Faux ``threading`` version using ``dummy_thread`` instead of ``thread``.

The module ``_dummy_threading`` is added to ``sys.modules`` in order
to not have ``threading`` considered imported.  Had ``threading`` been
directly imported it would have made all subsequent imports succeed
regardless of whether ``thread`` was available which is not desired.

:Author: Brett Cannon
:Contact: brett@python.org

XXX: Try to get rid of ``_dummy_threading``.

"""
from sys import modules as sys_modules

import dummy_thread

# Declaring now so as to not have to nest ``try``s to get proper clean-up.
holding_thread = False
holding_threading = False

try:
    # Could have checked if ``thread`` was not in sys.modules and gone
    # a different route, but decided to mirror technique used with
    # ``threading`` below.
    if 'thread' in sys_modules:
        held_thread = sys_modules['thread']
        holding_thread = True
    # Must have some module named ``thread`` that implements its API
    # in order to initially import ``threading``.
    sys_modules['thread'] = sys_modules['dummy_thread']

    if 'threading' in sys_modules:
        # If ``threading`` is already imported, might as well prevent
        # trying to import it more than needed by saving it if it is
        # already imported before deleting it.
        held_threading = sys_modules['threading']
        holding_threading = True
        del sys_modules['threading']
    import threading
    # Need a copy of the code kept somewhere...
    sys_modules['_dummy_threading'] = sys_modules['threading']
    del sys_modules['threading']
    from _dummy_threading import *
    from _dummy_threading import __all__

finally:
    # Put back ``threading`` if we overwrote earlier
    if holding_threading:
        sys_modules['threading'] = held_threading
        del held_threading
    del holding_threading

    # Put back ``thread`` if we overwrote, else del the entry we made
    if holding_thread:
        sys_modules['thread'] = held_thread
        del held_thread
    else:
        del sys_modules['thread']
    del holding_thread

    del dummy_thread
    del sys_modules
