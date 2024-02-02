:orphan:

.. _pyporting-howto:

*************************************
How to port Python 2 Code to Python 3
*************************************

:author: Brett Cannon

.. topic:: Abstract

   Python 2 reached its official end-of-life at the start of 2020. This means
   that no new bug reports, fixes, or changes will be made to Python 2 - it's
   no longer supported: see :pep:`373` and
   `status of Python versions <https://devguide.python.org/versions>`_.

   This guide is intended to provide you with a path to Python 3 for your
   code, that includes compatibility with Python 2 as a first step.

   If you are looking to port an extension module instead of pure Python code,
   please see :ref:`cporting-howto`.

   The archived python-porting_ mailing list may contain some useful guidance.

   Since Python 3.13 the original official pyporting guide was discontinued.
   You can still find the old guide in the
   `archive <https://docs.python.org/3.12/howto/pyporting.html>`_.


Third-party guides
==================

There are also multiple third-party guides that might be useful:
- `Guide by Fedora <https://portingguide.readthedocs.io>`_
- `PyCon 2020 tutorial <https://www.youtube.com/watch?v=JgIgEjASOlk>`_
- `Guide by Digital Ocean <https://www.digitalocean.com/community/tutorials/how-to-port-python-2-code-to-python-3>`_
- `Guide by ActiveState <https://www.activestate.com/blog/how-to-migrate-python-2-applications-to-python-3>`_


.. _python-porting: https://mail.python.org/pipermail/python-porting/
