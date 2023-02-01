.. bpo: 24769
.. date: 9294
.. nonce: XgRA0n
.. release date: 2015-08-25
.. section: Core and Builtins

Interpreter now starts properly when dynamic loading is disabled.  Patch by
Petr Viktorin.

..

.. bpo: 21167
.. date: 9293
.. nonce: uom-Dq
.. section: Core and Builtins

NAN operations are now handled correctly when python is compiled with ICC
even if -fp-model strict is not specified.

..

.. bpo: 24492
.. date: 9292
.. nonce: LKDAIu
.. section: Core and Builtins

A "package" lacking a __name__ attribute when trying to perform a ``from ..
import ...`` statement will trigger an ImportError instead of an
AttributeError.

..

.. bpo: 24847
.. date: 9291
.. nonce: SHiiO_
.. section: Library

Removes vcruntime140.dll dependency from Tcl/Tk.

..

.. bpo: 24839
.. date: 9290
.. nonce: 7_iQZl
.. section: Library

platform._syscmd_ver raises DeprecationWarning

..

.. bpo: 24867
.. date: 9289
.. nonce: rxJIl7
.. section: Library

Fix Task.get_stack() for 'async def' coroutines
