"""functools.py - Tools for working with functions
"""
# Python module wrapper for _functools C module
# to allow utilities written in Python to be added
# to the functools module.
# Written by Nick Coghlan <ncoghlan at gmail.com>
#   Copyright (c) 2006 Python Software Foundation.

from _functools import partial
__all__ = [
    "partial",
]

# Still to come here (need to write tests and docs):
#   update_wrapper - utility function to transfer basic function
#                    metadata to wrapper functions
#   WRAPPER_ASSIGNMENTS & WRAPPER_UPDATES - defaults args to above
#           (update_wrapper has been approved by BDFL)
#   wraps - decorator factory equivalent to:
#               def wraps(f):
#                     return partial(update_wrapper, wrapped=f)
#
# The wraps function makes it easy to avoid the bug that afflicts the
# decorator example in the python-dev email proposing the
# update_wrapper function:
# http://mail.python.org/pipermail/python-dev/2006-May/064775.html