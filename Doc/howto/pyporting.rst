:orphan:

.. _pyporting-howto:

*************************************
How to port Python 2 Code to Python 3
*************************************

:author: Brett Cannon

Python 2 reached its official end-of-life at the start of 2020. This means
that no new bug reports, fixes, or changes will be made to Python 2 - it's
no longer supported: see :pep:`373` and
`status of Python versions <https://devguide.python.org/versions>`_.

If you are looking to port an extension module instead of pure Python code,
please see :ref:`cporting-howto`.

The archived python-porting_ mailing list may contain some useful guidance.

Since Python 3.11 the original porting guide was discontinued.
You can find the old guide in the
`archive <https://docs.python.org/3.10/howto/pyporting.html>`_.


Third-party guides
==================

There are also multiple third-party guides that might be useful:

- `Guide by Fedora <https://portingguide.readthedocs.io>`_
- `PyCon 2020 tutorial <https://www.youtube.com/watch?v=JgIgEjASOlk>`_
- `Guide by DigitalOcean <https://www.digitalocean.com/community/tutorials/how-to-port-python-2-code-to-python-3>`_
- `Guide by ActiveState <https://www.activestate.com/blog/how-to-migrate-python-2-applications-to-python-3>`_


.. _python-porting: https://mail.python.org/pipermail/python-porting/
