"""Perfetto trace collector for Python sampling profiles.

Exports sampled call stacks as StackSample packets and thread states as
TrackEvent slices for visualization in the Perfetto UI.
"""
import os
import sys
import time

from .collector import Collector, extract_lineno, filter_internal_frames
from .constants import (
    THREAD_STATUS_GIL_REQUESTED,
    THREAD_STATUS_HAS_EXCEPTION,
    THREAD_STATUS_HAS_GIL,
    THREAD_STATUS_ON_CPU,
)
from .opcode_utils import format_opcode


# --------------------------- wire-format primitives ---------------------------
def _append_bytes_field(buffer, field, data):
    _append_tag(buffer, field, 2)
    _append_varint(buffer, len(data))
    buffer.extend(data)


def _append_string_field(buffer, field, value):
    _append_bytes_field(buffer, field, value.encode("utf-8"))


def _append_tag(buffer, field, wire_type):
    _append_varint(buffer, (field << 3) | wire_type)


def _append_varint(buffer, value):
    value &= (1 << 64) - 1  # unsigned
    while value > 0x7F:
        buffer.append((value & 0x7F) | 0x80)
        value >>= 7
    buffer.append(value)


def _append_varint_field(buffer, field, value):
    _append_tag(buffer, field, 0)
    _append_varint(buffer, value)


# --------------------------- Perfetto trace schema ---------------------------
# These are field numbers from the Perfetto protobuf schema, grouped by their
# containing message.  Keeping them as integers avoids enum lookups while
# encoding every packet.

# Trace
_F_TRACE_PACKET = 1

# TracePacket
_F_TP_TIMESTAMP = 8
_F_TP_SEQ_ID = 10
_F_TP_TRACK_EVENT = 11
_F_TP_INTERNED = 12
_F_TP_SEQUENCE_FLAGS = 13
_F_TP_INCREMENTAL_STATE_CLEARED = 41
_F_TP_DEFAULTS = 59
_F_TP_TRACK_DESCRIPTOR = 60
_F_TP_STACK_SAMPLE = 135
_SEQUENCE_NEEDS_INCREMENTAL_STATE = 2

# InternedData
_F_ID_FUNCTION_NAMES = 5
_F_ID_FRAMES = 6
_F_ID_CALLSTACKS = 7
_F_ID_SOURCE_PATHS = 18

# InternedString and Frame
_F_IS_IID, _F_IS_STR = 1, 2
_F_FR_IID, _F_FR_FUNC = 1, 2
_F_FR_SOURCE_PATH, _F_FR_LINE = 5, 6
_F_FR_KIND = 7
_FRAME_KIND_NATIVE = 1
_FRAME_KIND_INTERPRETED = 3
_FRAME_KIND_GC = 5

# Callstack, StackSample, and TaskContext
_F_CS_IID, _F_CS_FRAMES = 1, 2
_F_SS_TASK_CONTEXT = 1
_F_SS_CALLSTACK_IID = 6
_F_SS_PRIMARY_WEIGHT = 11
_F_TC_PID, _F_TC_TID = 2, 3

# TracePacketDefaults, StackSampleDefaults, and CounterDescriptor
_F_TPD_STACK_SAMPLE_DEFAULTS = 100
_F_SSD_SOURCE, _F_SSD_PRIMARY = 1, 2
_F_CD_NAME, _F_CD_UNIT = 2, 3
_COUNTER_UNIT_NANOSECONDS = 1

# TrackDescriptor, ThreadDescriptor, and TrackEvent
_F_TD_UUID, _F_TD_NAME, _F_TD_THREAD, _F_TD_PARENT_UUID = 1, 2, 4, 5
_F_THD_PID, _F_THD_TID = 1, 2
_F_TE_TYPE, _F_TE_NAME, _F_TE_TRACK_UUID = 9, 23, 11
_TRACK_EVENT_TYPE_SLICE_BEGIN = 1
_TRACK_EVENT_TYPE_SLICE_END = 2

# Per-thread interval categories -> track name shown in the UI.
_CATEGORY_TRACK_NAME = {
    "code": "code type",
    "gil": "gil",
    "exception": "exception",
    "gc": "gc",
    "opcode": "python opcodes",
}


class PerfettoCollector(Collector):
    """StackSample callstacks + per-thread state slices on one trace sequence."""

    aggregating = False

    def __init__(self, sample_interval_usec, *, skip_idle=False, opcodes=False,
                 pid=None):
        self.sample_interval_usec = sample_interval_usec
        self.skip_idle = skip_idle
        self.opcodes_enabled = opcodes
        self.pid = os.getpid() if pid is None else pid
        self._start_ns = time.monotonic_ns()
        self._replay_base_us = None
        self._max_ts_ns = 0
        # Callstack interning.
        self._func_ids = {}
        self._source_path_ids = {}
        self._frame_ids = {}
        self._callstack_ids = {}
        self._pending_interned = bytearray()
        self._task_contexts = {}
        self._trace = bytearray()
        self._packet = bytearray()
        self._sample = bytearray()
        self._started = False
        # Per-thread interval state.
        self._thread_track_uuids = {}  # tid -> thread track uuid
        self._track_uuids = {}   # (tid, category) -> child track uuid
        self._open = {}          # (tid, category) -> state_key of the open slice
        self._next_uuid = 1

    # ------------------------- callstack interning -------------------------
    def _fn_id(self, name):
        iid = self._func_ids.get(name)
        if iid is None:
            iid = len(self._func_ids) + 1
            self._func_ids[name] = iid
            interned_string = bytearray()
            _append_varint_field(interned_string, _F_IS_IID, iid)
            _append_string_field(interned_string, _F_IS_STR, name)
            _append_bytes_field(
                self._pending_interned,
                _F_ID_FUNCTION_NAMES,
                interned_string,
            )
        return iid

    def _source_path_id(self, path):
        iid = self._source_path_ids.get(path)
        if iid is None:
            iid = len(self._source_path_ids) + 1
            self._source_path_ids[path] = iid
            interned_string = bytearray()
            _append_varint_field(interned_string, _F_IS_IID, iid)
            _append_string_field(interned_string, _F_IS_STR, path)
            _append_bytes_field(
                self._pending_interned,
                _F_ID_SOURCE_PATHS,
                interned_string,
            )
        return iid

    def _frame_id(self, frame):
        filename, location, funcname = frame[0], frame[1], frame[2]
        lineno = extract_lineno(location)
        kind = self._frame_kind(frame)
        key = (funcname, filename, lineno, kind)
        iid = self._frame_ids.get(key)
        if iid is None:
            iid = len(self._frame_ids) + 1
            self._frame_ids[key] = iid
            encoded_frame = bytearray()
            _append_varint_field(encoded_frame, _F_FR_IID, iid)
            _append_varint_field(
                encoded_frame, _F_FR_FUNC, self._fn_id(funcname))
            # <native> and <GC> are synthetic markers emitted with "~" as
            # their filename. They carry no source location or native address.
            synthetic_marker = (
                filename == "~" and lineno == 0
                and funcname in ("<native>", "<GC>")
            )
            if filename and not synthetic_marker:
                _append_varint_field(
                    encoded_frame,
                    _F_FR_SOURCE_PATH,
                    self._source_path_id(filename),
                )
            if lineno > 0 and not synthetic_marker:
                _append_varint_field(encoded_frame, _F_FR_LINE, lineno)
            _append_varint_field(encoded_frame, _F_FR_KIND, kind)
            _append_bytes_field(
                self._pending_interned, _F_ID_FRAMES, encoded_frame)
        return iid

    @staticmethod
    def _frame_kind(frame):
        # The native and GC entries produced by the sampler are synthetic
        # markers rather than symbolized machine-code frames.
        synthetic_marker = frame[0] == "~" and extract_lineno(frame[1]) == 0
        if synthetic_marker and frame[2] == "<native>":
            return _FRAME_KIND_NATIVE
        if synthetic_marker and frame[2] == "<GC>":
            return _FRAME_KIND_GC
        return _FRAME_KIND_INTERPRETED

    def _callstack_id(self, frames_leaf_first):
        # The profiler gives frames leaf-first; StackSample wants bottom
        # (root) frame first.
        key = tuple(self._frame_id(f) for f in reversed(frames_leaf_first))
        iid = self._callstack_ids.get(key)
        if iid is None:
            iid = len(self._callstack_ids) + 1
            self._callstack_ids[key] = iid
            callstack = bytearray()
            _append_varint_field(callstack, _F_CS_IID, iid)
            for frame_iid in key:
                _append_varint_field(callstack, _F_CS_FRAMES, frame_iid)
            _append_bytes_field(
                self._pending_interned, _F_ID_CALLSTACKS, callstack)
        return iid

    # ------------------------- emit -------------------------
    def _start_packet(self, timestamp=None):
        self._packet.clear()
        _append_varint_field(self._packet, _F_TP_SEQ_ID, 1)
        if timestamp is not None:
            _append_varint_field(self._packet, _F_TP_TIMESTAMP, timestamp)
        return self._packet

    def _finish_packet(self):
        _append_bytes_field(self._trace, _F_TRACE_PACKET, self._packet)

    def _task_context(self, tid):
        context = self._task_contexts.get(tid)
        if context is None:
            context = bytearray()
            _append_varint_field(context, _F_TC_PID, self.pid)
            _append_varint_field(context, _F_TC_TID, tid)
            self._task_contexts[tid] = context
        return context

    def _emit_sample(self, ts_ns, tid, cs_iid, weight_ns):
        packet = self._start_packet(ts_ns)
        if not self._started:
            self._started = True
            _append_varint_field(packet, _F_TP_INCREMENTAL_STATE_CLEARED, 1)

            counter = bytearray()
            _append_string_field(counter, _F_CD_NAME, "wall-time")
            _append_varint_field(
                counter, _F_CD_UNIT, _COUNTER_UNIT_NANOSECONDS)

            sample_defaults = bytearray()
            _append_string_field(sample_defaults, _F_SSD_SOURCE, "python.wall")
            _append_bytes_field(sample_defaults, _F_SSD_PRIMARY, counter)

            packet_defaults = bytearray()
            _append_bytes_field(
                packet_defaults,
                _F_TPD_STACK_SAMPLE_DEFAULTS,
                sample_defaults,
            )
            _append_bytes_field(packet, _F_TP_DEFAULTS, packet_defaults)
        if self._pending_interned:
            _append_bytes_field(packet, _F_TP_INTERNED,
                                self._pending_interned)
            self._pending_interned.clear()

        self._sample.clear()
        _append_bytes_field(
            self._sample, _F_SS_TASK_CONTEXT, self._task_context(tid))
        _append_varint_field(self._sample, _F_SS_CALLSTACK_IID, cs_iid)
        _append_varint_field(self._sample, _F_SS_PRIMARY_WEIGHT, weight_ns)
        _append_bytes_field(packet, _F_TP_STACK_SAMPLE, self._sample)
        self._finish_packet()

    def _thread_track(self, tid):
        uuid = self._thread_track_uuids.get(tid)
        if uuid is None:
            uuid = self._next_uuid
            self._next_uuid += 1
            self._thread_track_uuids[tid] = uuid

            thread = bytearray()
            _append_varint_field(thread, _F_THD_PID, self.pid)
            _append_varint_field(thread, _F_THD_TID, tid)

            descriptor = bytearray()
            _append_varint_field(descriptor, _F_TD_UUID, uuid)
            _append_bytes_field(descriptor, _F_TD_THREAD, thread)

            packet = self._start_packet()
            _append_bytes_field(packet, _F_TP_TRACK_DESCRIPTOR, descriptor)
            self._finish_packet()
        return uuid

    def _track_uuid(self, tid, category):
        # Each category is its own child track under the thread track, so their
        # slices don't interleave on a single timeline.
        key = (tid, category)
        uuid = self._track_uuids.get(key)
        if uuid is None:
            parent = self._thread_track(tid)
            uuid = self._next_uuid
            self._next_uuid += 1
            self._track_uuids[key] = uuid

            descriptor = bytearray()
            _append_varint_field(descriptor, _F_TD_UUID, uuid)
            _append_string_field(
                descriptor, _F_TD_NAME, _CATEGORY_TRACK_NAME[category])
            _append_varint_field(descriptor, _F_TD_PARENT_UUID, parent)

            packet = self._start_packet()
            _append_bytes_field(packet, _F_TP_TRACK_DESCRIPTOR, descriptor)
            self._finish_packet()
        return uuid

    def _emit_track_event(self, ts_ns, uuid, ev_type, name=None):
        event = bytearray()
        _append_varint_field(event, _F_TE_TYPE, ev_type)
        _append_varint_field(event, _F_TE_TRACK_UUID, uuid)
        if name is not None:
            _append_string_field(event, _F_TE_NAME, name)

        packet = self._start_packet(ts_ns)
        _append_varint_field(
            packet,
            _F_TP_SEQUENCE_FLAGS,
            _SEQUENCE_NEEDS_INCREMENTAL_STATE,
        )
        _append_bytes_field(packet, _F_TP_TRACK_EVENT, event)
        self._finish_packet()

    def _transition(self, ts_ns, tid, category, label, state_key=None):
        """Open/close an interval slice for (tid, category). `label` is the slice
        name (None = the state is inactive); `state_key` distinguishes spans that
        share a name (defaults to `label`)."""
        if state_key is None:
            state_key = label
        key = (tid, category)
        prev = self._open.get(key)
        if prev == state_key:
            return
        uuid = self._track_uuid(tid, category)
        if prev is not None:
            self._emit_track_event(
                ts_ns, uuid, _TRACK_EVENT_TYPE_SLICE_END)
        if label is not None:
            self._emit_track_event(
                ts_ns, uuid, _TRACK_EVENT_TYPE_SLICE_BEGIN, label)
            self._open[key] = state_key
        else:
            self._open.pop(key, None)

    def _emit_thread_states(self, ts_ns, tid, frames, status):
        has_gil = bool(status & THREAD_STATUS_HAS_GIL)
        on_cpu = bool(status & THREAD_STATUS_ON_CPU)
        # Code type: Python while it holds the GIL, Native while on-CPU without
        # it, nothing when idle.
        code = "Python Code" if has_gil else ("Native Code" if on_cpu else None)
        self._transition(ts_ns, tid, "code", code)
        self._transition(ts_ns, tid, "gil",
                         "Waiting for GIL"
                         if status & THREAD_STATUS_GIL_REQUESTED else None)
        self._transition(ts_ns, tid, "exception",
                         "Has Exception"
                         if status & THREAD_STATUS_HAS_EXCEPTION else None)
        self._transition(ts_ns, tid, "gc",
                         "GC Collecting"
                         if any(f[2] == "<GC>" for f in frames) else None)
        if self.opcodes_enabled and frames:
            leaf = frames[0]
            opcode = leaf[3] if len(leaf) > 3 else None
            if opcode is not None:
                self._transition(ts_ns, tid, "opcode", format_opcode(opcode),
                                 (opcode, extract_lineno(leaf[1]), leaf[2],
                                  leaf[0]))

    def collect(self, stack_frames, timestamps_us=None):
        if timestamps_us is None:
            times_ns = [time.monotonic_ns() - self._start_ns]
        else:
            if not timestamps_us:
                return
            if self._replay_base_us is None:
                self._replay_base_us = timestamps_us[0]
            base = self._replay_base_us
            times_ns = [int((ts - base) * 1000) for ts in timestamps_us]
        self._max_ts_ns = max(self._max_ts_ns, times_ns[-1])
        weight_ns = self.sample_interval_usec * 1000
        t0 = times_ns[0]

        # Async tasks carry no per-thread status; emit callstacks only.
        if stack_frames and hasattr(stack_frames[0], "awaited_by"):
            for frames, thread_id, _ in self._iter_async_frames(stack_frames):
                frames = filter_internal_frames(frames)
                if not frames:
                    continue
                cs_iid = self._callstack_id(frames)
                for ts_ns in times_ns:
                    self._emit_sample(ts_ns, thread_id, cs_iid, weight_ns)
            return

        for interpreter_info in stack_frames:
            for thread_info in interpreter_info.threads:
                tid = thread_info.thread_id
                status = getattr(thread_info, "status", 0)
                frames = filter_internal_frames(thread_info.frame_info or [])
                if frames:
                    cs_iid = self._callstack_id(frames)
                    for ts_ns in times_ns:
                        self._emit_sample(ts_ns, tid, cs_iid, weight_ns)
                self._emit_thread_states(t0, tid, frames, status)

    def export(self, filename):
        if filename is None and sys.stdout.isatty():
            raise ValueError(
                "Perfetto output is binary and cannot be written to a terminal; "
                "specify an output filename"
            )

        # Close any still-open interval slices at the last observed timestamp.
        for (tid, category) in list(self._open):
            self._emit_track_event(self._max_ts_ns,
                                   self._track_uuids[(tid, category)],
                                   _TRACK_EVENT_TYPE_SLICE_END)
        self._open.clear()
        if not self._trace:
            return False
        if filename is None:
            sys.stdout.buffer.write(self._trace)
        else:
            with open(filename, "wb") as f:
                f.write(self._trace)
        return True
