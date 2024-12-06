# Copyright (C) 2016-2024 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import gdb


class _AsString(gdb.Function):
    """Return the string representation of a value.

    Usage: $_as_string (VALUE)

    Arguments:

      VALUE: any value

    Returns:
      The string representation of the value."""

    def __init__(self):
        super(_AsString, self).__init__("_as_string")

    def invoke(self, val):
        return str(val)


_AsString()
