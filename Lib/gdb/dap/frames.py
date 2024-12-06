# Copyright 2022-2024 Free Software Foundation, Inc.

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

import itertools

import gdb
from gdb.frames import frame_iterator

from .startup import in_gdb_thread

# A list of all the frames we've reported.  A frame's index in the
# list is its ID.  We don't use a hash here because frames are not
# hashable.
_all_frames = []


# Map from a global thread ID to a memoizing frame iterator.
_iter_map = {}


# Clear all the frame IDs.
@in_gdb_thread
def _clear_frame_ids(evt):
    global _all_frames
    _all_frames = []
    global _iter_map
    _iter_map = {}


# Clear the frame ID map whenever the inferior runs.
gdb.events.cont.connect(_clear_frame_ids)


@in_gdb_thread
def frame_for_id(id):
    """Given a frame identifier ID, return the corresponding frame."""
    global _all_frames
    return _all_frames[id]


@in_gdb_thread
def select_frame(id):
    """Given a frame identifier ID, select the corresponding frame."""
    frame = frame_for_id(id)
    frame.inferior_frame().select()


# A simple memoizing iterator.  Note that this is not very robust.
# For example, you can't start two copies of the iterator and then
# alternate fetching items from each.  Instead, it implements just
# what is needed for the current callers.
class _MemoizingIterator:
    def __init__(self, iterator):
        self.iterator = iterator
        self.seen = []

    def __iter__(self):
        # First the memoized items.
        for item in self.seen:
            yield item
        # Now memoize new items.
        for item in self.iterator:
            self.seen.append(item)
            yield item


# A generator that fetches frames and pairs them with a frame ID.  It
# yields tuples of the form (ID, ELIDED, FRAME), where ID is the
# generated ID, ELIDED is a boolean indicating if the frame should be
# elided, and FRAME is the frame itself.  This approach lets us
# memoize the result and assign consistent IDs, independent of how
# "includeAll" is set in the request.
@in_gdb_thread
def _frame_id_generator():
    try:
        base_iterator = frame_iterator(gdb.newest_frame(), 0, -1)
    except gdb.error:
        base_iterator = ()

    # Helper function to assign an ID to a frame.
    def get_id(frame):
        global _all_frames
        num = len(_all_frames)
        _all_frames.append(frame)
        return num

    def yield_frames(iterator, for_elided):
        for frame in iterator:
            # Unfortunately the frame filter docs don't describe
            # whether the elided frames conceptually come before or
            # after the eliding frame.  Here we choose after.
            yield (get_id(frame), for_elided, frame)

            elided = frame.elided()
            if elided is not None:
                yield from yield_frames(frame.elided(), True)

    yield from yield_frames(base_iterator, False)


# Return the memoizing frame iterator for the selected thread.
@in_gdb_thread
def _get_frame_iterator():
    thread_id = gdb.selected_thread().global_num
    global _iter_map
    if thread_id not in _iter_map:
        _iter_map[thread_id] = _MemoizingIterator(_frame_id_generator())
    return _iter_map[thread_id]


# A helper function that creates an iterable that returns (ID, FRAME)
# pairs.  It uses the memoizing frame iterator, but also handles the
# "includeAll" member of StackFrameFormat.
@in_gdb_thread
def dap_frame_generator(frame_low, levels, include_all):
    """A generator that yields identifiers and frames.

    Each element is a pair of the form (ID, FRAME).
    ID is the internally-assigned frame ID.
    FRAME is a FrameDecorator of some kind.

    Arguments are as to the stackTrace request."""

    base_iterator = _get_frame_iterator()

    if not include_all:
        base_iterator = itertools.filterfalse(lambda item: item[1], base_iterator)

    if levels == 0:
        # Zero means all remaining frames.
        frame_high = None
    else:
        frame_high = frame_low + levels
    base_iterator = itertools.islice(base_iterator, frame_low, frame_high)

    for ident, _, frame in base_iterator:
        yield (ident, frame)
