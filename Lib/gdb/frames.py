# Frame-filter commands.
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

"""Internal functions for working with frame-filters."""

import collections
import itertools

import gdb
from gdb.FrameDecorator import DAPFrameDecorator, FrameDecorator
from gdb.FrameIterator import FrameIterator


def get_priority(filter_item):
    """Internal worker function to return the frame-filter's priority
    from a frame filter object.  This is a fail free function as it is
    used in sorting and filtering.  If a badly implemented frame
    filter does not implement the priority attribute, return zero
    (otherwise sorting/filtering will fail and prevent other frame
    filters from executing).

    Arguments:
        filter_item: An object conforming to the frame filter
                     interface.

    Returns:
        The priority of the frame filter from the "priority"
        attribute, or zero.
    """
    # Do not fail here, as the sort will fail.  If a filter has not
    # (incorrectly) set a priority, set it to zero.
    return getattr(filter_item, "priority", 0)


def set_priority(filter_item, priority):
    """Internal worker function to set the frame-filter's priority.

    Arguments:
        filter_item: An object conforming to the frame filter
                     interface.
        priority: The priority to assign as an integer.
    """

    filter_item.priority = priority


def get_enabled(filter_item):
    """Internal worker function to return a filter's enabled state
    from a frame filter object.  This is a fail free function as it is
    used in sorting and filtering.  If a badly implemented frame
    filter does not implement the enabled attribute, return False
    (otherwise sorting/filtering will fail and prevent other frame
    filters from executing).

    Arguments:
        filter_item: An object conforming to the frame filter
                     interface.

    Returns:
        The enabled state of the frame filter from the "enabled"
        attribute, or False.
    """

    # If the filter class is badly implemented when called from the
    # Python filter command, do not cease filter operations, just set
    # enabled to False.
    return getattr(filter_item, "enabled", False)


def set_enabled(filter_item, state):
    """Internal Worker function to set the frame-filter's enabled
    state.

    Arguments:
        filter_item: An object conforming to the frame filter
                     interface.
        state: True or False, depending on desired state.
    """

    filter_item.enabled = state


def return_list(name):
    """Internal Worker function to return the frame filter
    dictionary, depending on the name supplied as an argument.  If the
    name is not "all", "global" or "progspace", it is assumed to name
    an object-file.

    Arguments:
        name: The name of the list, as specified by GDB user commands.

    Returns:
        A dictionary object for a single specified dictionary, or a
        list containing all the items for "all"

    Raises:
        gdb.GdbError:  A dictionary of that name cannot be found.
    """

    # If all dictionaries are wanted in the case of "all" we
    # cannot return a combined dictionary as keys() may clash in
    # between different dictionaries.  As we just want all the frame
    # filters to enable/disable them all, just return the combined
    # items() as a chained iterator of dictionary values.
    if name == "all":
        glob = gdb.frame_filters.values()
        prog = gdb.current_progspace().frame_filters.values()
        return_iter = itertools.chain(glob, prog)
        for objfile in gdb.objfiles():
            return_iter = itertools.chain(return_iter, objfile.frame_filters.values())

        return return_iter

    if name == "global":
        return gdb.frame_filters
    else:
        if name == "progspace":
            cp = gdb.current_progspace()
            return cp.frame_filters
        else:
            for objfile in gdb.objfiles():
                if name == objfile.filename:
                    return objfile.frame_filters

    msg = "Cannot find frame-filter dictionary for '" + name + "'"
    raise gdb.GdbError(msg)


def _sort_list():
    """Internal Worker function to merge all known frame-filter
    lists, prune any filters with the state set to "disabled", and
    sort the list on the frame-filter's "priority" attribute.

    Returns:
        sorted_list: A sorted, pruned list of frame filters to
                     execute.
    """

    all_filters = return_list("all")
    sorted_frame_filters = sorted(all_filters, key=get_priority, reverse=True)

    sorted_frame_filters = filter(get_enabled, sorted_frame_filters)

    return sorted_frame_filters


# Internal function that implements frame_iterator and
# execute_frame_filters.  If DAP_SEMANTICS is True, then this will
# always return an iterator and will wrap frames in DAPFrameDecorator.
def _frame_iterator(frame, frame_low, frame_high, dap_semantics):
    # Get a sorted list of frame filters.
    sorted_list = list(_sort_list())

    # Check to see if there are any frame-filters.  If not, just
    # return None and let default backtrace printing occur.
    if not dap_semantics and len(sorted_list) == 0:
        return None

    frame_iterator = FrameIterator(frame)

    # Apply a basic frame decorator to all gdb.Frames.  This unifies
    # the interface.
    if dap_semantics:
        decorator = DAPFrameDecorator
    else:
        decorator = FrameDecorator
    frame_iterator = map(decorator, frame_iterator)

    for ff in sorted_list:
        frame_iterator = ff.filter(frame_iterator)

    # Slicing

    # Is this a slice from the end of the backtrace, ie bt -2?
    if frame_low < 0:
        count = 0
        slice_length = abs(frame_low)
        # We cannot use MAXLEN argument for deque as it is 2.6 onwards
        # and some GDB versions might be < 2.6.
        sliced = collections.deque()

        for frame_item in frame_iterator:
            if count >= slice_length:
                sliced.popleft()
            count = count + 1
            sliced.append(frame_item)

        return iter(sliced)

    # -1 for frame_high means until the end of the backtrace.  Set to
    # None if that is the case, to indicate to itertools.islice to
    # slice to the end of the iterator.
    if frame_high == -1:
        frame_high = None
    else:
        # As frames start from 0, add one to frame_high so islice
        # correctly finds the end
        frame_high = frame_high + 1

    sliced = itertools.islice(frame_iterator, frame_low, frame_high)

    return sliced


def frame_iterator(frame, frame_low, frame_high):
    """Helper function that will execute the chain of frame filters.
    Each filter is executed in priority order.  After the execution
    completes, slice the iterator to frame_low - frame_high range.  An
    iterator is always returned.  The iterator will always yield
    frame decorator objects, but note that these decorators have
    slightly different semantics from the ordinary ones: they will
    always return a fully-qualified 'filename' (if possible) and will
    never substitute the objfile name.

    Arguments:
        frame: The initial frame.

        frame_low: The low range of the slice, counting from 0.  If
        this is a negative integer then it indicates a backward slice
        (ie bt -4) which counts backward from the last frame in the
        backtrace.

        frame_high: The high range of the slice, inclusive.  If this
        is -1 then it indicates all frames until the end of the stack
        from frame_low.

    Returns:
        frame_iterator: The sliced iterator after all frame
        filters have had a chance to execute.
    """

    return _frame_iterator(frame, frame_low, frame_high, True)


def execute_frame_filters(frame, frame_low, frame_high):
    """Internal function called from GDB that will execute the chain
    of frame filters.  Each filter is executed in priority order.
    After the execution completes, slice the iterator to frame_low -
    frame_high range.

    Arguments:
        frame: The initial frame.

        frame_low: The low range of the slice, counting from 0.  If
        this is a negative integer then it indicates a backward slice
        (ie bt -4) which counts backward from the last frame in the
        backtrace.

        frame_high: The high range of the slice, inclusive.  If this
        is -1 then it indicates all frames until the end of the stack
        from frame_low.

    Returns:
        frame_iterator: The sliced iterator after all frame
        filters have had a chance to execute, or None if no frame
        filters are registered.

    """

    return _frame_iterator(frame, frame_low, frame_high, False)
