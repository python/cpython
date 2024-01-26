.. bpo: 33851
.. date: 2018-06-13-15-12-25
.. nonce: SVbqlz
.. release date: 2018-06-27
.. section: Library

Fix :func:`ast.get_docstring` for a node that lacks a docstring.

..

.. bpo: 33932
.. date: 2018-06-21-15-29-59
.. nonce: VSlXyS
.. section: C API

Calling Py_Initialize() twice does nothing, instead of failing with a fatal
error: restore the Python 3.6 behaviour.
