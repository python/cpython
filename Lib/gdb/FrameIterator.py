# Copyright (C) 2013-2024 Free Software Foundation, Inc.

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


class FrameIterator(object):
    """A gdb.Frame iterator.  Iterates over gdb.Frames or objects that
    conform to that interface."""

    def __init__(self, frame_obj):
        """Initialize a FrameIterator.

        Arguments:
            frame_obj the starting frame."""

        super(FrameIterator, self).__init__()
        self.frame = frame_obj

    def __iter__(self):
        return self

    def __next__(self):
        """next implementation.

        Returns:
            The next oldest frame."""

        result = self.frame
        if result is None:
            raise StopIteration
        self.frame = result.older()
        return result
