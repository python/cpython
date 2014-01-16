"""
Some helper functions to analyze the output of sys.getdxp() (which is
only available if Python was built with -DDYNAMIC_EXECUTION_PROFILE).
These will tell you which opcodes have been executed most frequently
in the current process, and, if Python was also built with -DDXPAIRS,
will tell you which instruction _pairs_ were executed most frequently,
which may help in choosing new instructions.

If Python was built without -DDYNAMIC_EXECUTION_PROFILE, importing
this module will raise a RuntimeError.

If you're running a script you want to profile, a simple way to get
the common pairs is:

$ PYTHONPATH=$PYTHONPATH:<python_srcdir>/Tools/scripts \
./python -i -O the_script.py --args
...
> from analyze_dxp import *
> s = render_common_pairs()
> open('/tmp/some_file', 'w').write(s)
"""

import copy
import opcode
import operator
import sys
import threading

if not hasattr(sys, "getdxp"):
    raise RuntimeError("Can't import analyze_dxp: Python built without"
                       " -DDYNAMIC_EXECUTION_PROFILE.")


_profile_lock = threading.RLock()
_cumulative_profile = sys.getdxp()

# If Python was built with -DDXPAIRS, sys.getdxp() returns a list of
# lists of ints.  Otherwise it returns just a list of ints.
def has_pairs(profile):
    """Returns True if the Python that produced the argument profile
    was built with -DDXPAIRS."""

    return len(profile) > 0 and isinstance(profile[0], list)


def reset_profile():
    """Forgets any execution profile that has been gathered so far."""
    with _profile_lock:
        sys.getdxp()  # Resets the internal profile
        global _cumulative_profile
        _cumulative_profile = sys.getdxp()  # 0s out our copy.


def merge_profile():
    """Reads sys.getdxp() and merges it into this module's cached copy.

    We need this because sys.getdxp() 0s itself every time it's called."""

    with _profile_lock:
        new_profile = sys.getdxp()
        if has_pairs(new_profile):
            for first_inst in range(len(_cumulative_profile)):
                for second_inst in range(len(_cumulative_profile[first_inst])):
                    _cumulative_profile[first_inst][second_inst] += (
                        new_profile[first_inst][second_inst])
        else:
            for inst in range(len(_cumulative_profile)):
                _cumulative_profile[inst] += new_profile[inst]


def snapshot_profile():
    """Returns the cumulative execution profile until this call."""
    with _profile_lock:
        merge_profile()
        return copy.deepcopy(_cumulative_profile)


def common_instructions(profile):
    """Returns the most common opcodes in order of descending frequency.

    The result is a list of tuples of the form
      (opcode, opname, # of occurrences)

    """
    if has_pairs(profile) and profile:
        inst_list = profile[-1]
    else:
        inst_list = profile
    result = [(op, opcode.opname[op], count)
              for op, count in enumerate(inst_list)
              if count > 0]
    result.sort(key=operator.itemgetter(2), reverse=True)
    return result


def common_pairs(profile):
    """Returns the most common opcode pairs in order of descending frequency.

    The result is a list of tuples of the form
      ((1st opcode, 2nd opcode),
       (1st opname, 2nd opname),
       # of occurrences of the pair)

    """
    if not has_pairs(profile):
        return []
    result = [((op1, op2), (opcode.opname[op1], opcode.opname[op2]), count)
              # Drop the row of single-op profiles with [:-1]
              for op1, op1profile in enumerate(profile[:-1])
              for op2, count in enumerate(op1profile)
              if count > 0]
    result.sort(key=operator.itemgetter(2), reverse=True)
    return result


def render_common_pairs(profile=None):
    """Renders the most common opcode pairs to a string in order of
    descending frequency.

    The result is a series of lines of the form:
      # of occurrences: ('1st opname', '2nd opname')

    """
    if profile is None:
        profile = snapshot_profile()
    def seq():
        for _, ops, count in common_pairs(profile):
            yield "%s: %s\n" % (count, ops)
    return ''.join(seq())
