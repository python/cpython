"""Pure Python implementation of unicodedata.iter_graphemes().

Uses the extended grapheme cluster rules from Unicode TR29.
"""

import sys
import unicodedata


class Segment:
    """Represents a grapheme cluster segment within a string."""

    __slots__ = ('_string', 'start', 'end')

    def __init__(self, string, start, end):
        self._string = string
        self.start = start
        self.end = end

    def __str__(self):
        return self._string[self.start:self.end]

    def __repr__(self):
        return f"<Segment {self.start}:{self.end}>"


# Grapheme_Cluster_Break property values (matching C #defines)
_GCB_Other = "Other"
_GCB_Prepend = "Prepend"
_GCB_CR = "CR"
_GCB_LF = "LF"
_GCB_Control = "Control"
_GCB_Extend = "Extend"
_GCB_Regional_Indicator = "Regional_Indicator"
_GCB_SpacingMark = "SpacingMark"
_GCB_L = "L"
_GCB_V = "V"
_GCB_T = "T"
_GCB_LV = "LV"
_GCB_LVT = "LVT"
_GCB_ZWJ = "ZWJ"

# Indic_Conjunct_Break property values
_InCB_None = "None"
_InCB_Linker = "Linker"
_InCB_Consonant = "Consonant"
_InCB_Extend = "Extend"

# Extended Pictographic FSM states (for GB11)
_EP_INIT = 0
_EP_STARTED = 1
_EP_ZWJ = 2
_EP_MATCHED = 3

# Indic Conjunct Break FSM states (for GB9c)
_INCB_INIT = 0
_INCB_STARTED = 1
_INCB_LINKER = 2
_INCB_MATCHED = 3


def _update_ext_pict_state(state, gcb, ext_pict):
    if ext_pict:
        return _EP_MATCHED if state == _EP_ZWJ else _EP_STARTED
    if state == _EP_STARTED or state == _EP_MATCHED:
        if gcb == _GCB_Extend:
            return _EP_STARTED
        if gcb == _GCB_ZWJ:
            return _EP_ZWJ
    return _EP_INIT


def _update_incb_state(state, incb):
    if incb == _InCB_Consonant:
        return _INCB_MATCHED if state == _INCB_LINKER else _INCB_STARTED
    if state != _INCB_INIT:
        if incb == _InCB_Extend:
            return _INCB_LINKER if state == _INCB_LINKER else _INCB_STARTED
        if incb == _InCB_Linker:
            return _INCB_LINKER
    return _INCB_INIT


def _grapheme_break(prev_gcb, curr_gcb, ep_state, ri_flag, incb_state):
    """Return True if a grapheme cluster break occurs between two characters."""
    # GB3: Do not break between a CR and LF.
    if prev_gcb == _GCB_CR and curr_gcb == _GCB_LF:
        return False

    # GB4: Break after controls.
    if prev_gcb in (_GCB_CR, _GCB_LF, _GCB_Control):
        return True

    # GB5: Break before controls.
    if curr_gcb in (_GCB_CR, _GCB_LF, _GCB_Control):
        return True

    # GB6: Do not break Hangul syllable sequences (L).
    if prev_gcb == _GCB_L and curr_gcb in (_GCB_L, _GCB_V, _GCB_LV, _GCB_LVT):
        return False

    # GB7: Do not break Hangul syllable sequences (LV, V).
    if prev_gcb in (_GCB_LV, _GCB_V) and curr_gcb in (_GCB_V, _GCB_T):
        return False

    # GB8: Do not break Hangul syllable sequences (LVT, T).
    if prev_gcb in (_GCB_LVT, _GCB_T) and curr_gcb == _GCB_T:
        return False

    # GB9: Do not break before extending characters or ZWJ.
    if curr_gcb in (_GCB_Extend, _GCB_ZWJ):
        return False

    # GB9a: Do not break before SpacingMarks.
    if curr_gcb == _GCB_SpacingMark:
        return False

    # GB9b: Do not break after Prepend characters.
    if prev_gcb == _GCB_Prepend:
        return False

    # GB9c: Do not break within Indic conjunct clusters.
    if incb_state == _INCB_MATCHED:
        return False

    # GB11: Do not break within emoji ZWJ sequences.
    if ep_state == _EP_MATCHED:
        return False

    # GB12/GB13: Do not break within emoji flag sequences.
    if prev_gcb == _GCB_Regional_Indicator and curr_gcb == _GCB_Regional_Indicator:
        return ri_flag

    # GB999: Otherwise, break everywhere.
    return True


def iter_graphemes(string, /, start=0, end=sys.maxsize):
    """Iterate over grapheme clusters in a string.

    Uses extended grapheme cluster rules from TR29.

    Returns an iterator yielding Segment objects with start/end attributes
    and str() support.
    """
    if not isinstance(string, str):
        raise TypeError(
            "argument must be a unicode character, not "
            f"'{type(string).__name__}'"
        )

    length = len(string)
    # Adjust indices (matching CPython's ADJUST_INDICES macro)
    if end > length:
        end = length
    if end < 0:
        end += length
        if end < 0:
            end = 0
    if start < 0:
        start += length
        if start < 0:
            start = 0

    return _iter_grapheme_clusters(string, start, end)


def _iter_grapheme_clusters(string, start, end):
    gcb = _GCB_Other
    ep_state = _EP_INIT
    incb_state = _INCB_INIT
    ri_flag = False

    cluster_start = start
    pos = start
    while pos < end:
        ch = string[pos]
        curr_gcb = unicodedata.grapheme_cluster_break(ch)
        ext_pict = unicodedata.extended_pictographic(ch)
        incb = unicodedata.indic_conjunct_break(ch)

        ep_state = _update_ext_pict_state(ep_state, curr_gcb, ext_pict)
        ri_flag = (not ri_flag) if curr_gcb == _GCB_Regional_Indicator else False
        incb_state = _update_incb_state(incb_state, incb)

        prev_gcb = gcb
        gcb = curr_gcb

        if pos != cluster_start and _grapheme_break(
            prev_gcb, curr_gcb, ep_state, ri_flag, incb_state
        ):
            yield Segment(string, cluster_start, pos)
            cluster_start = pos

        pos += 1

    if cluster_start < end:
        yield Segment(string, cluster_start, end)
