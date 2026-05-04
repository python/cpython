"""JSON Lines (JSONL) collector for the sampling profiler.

Emits a normalized newline-delimited JSON record stream suitable for
programmatic consumption by external tools, scripts, and agents. Each line
is one JSON object; consumers can parse the file incrementally line by
line, but the producer writes the whole file at the end of the run (it is
not a live/streaming producer).

Record schema
=============

Every record is a JSON object with at least ``"type"``, ``"v"`` (record
schema version), and ``"run_id"`` (UUID4 hex tagging the run; allows
demultiplexing concatenated streams). Records appear in this fixed order:

1. ``meta`` (exactly one, first line)::

      {"type":"meta","v":1,"run_id":"<hex>",
       "sample_interval_usec":<int>,"mode":"wall|cpu|gil|all|exception"}

   ``mode`` is omitted when not provided.

2. ``str_def`` (zero or more)::

      {"type":"str_def","v":1,"run_id":"<hex>",
       "defs":[{"str_id":<int>,"value":"<str>"}, ...]}

   Strings (filenames, function names) are interned to keep repeated values
   compact. Each chunk holds up to ``_CHUNK_SIZE`` entries.

3. ``frame_def`` (zero or more)::

      {"type":"frame_def","v":1,"run_id":"<hex>",
       "defs":[{"frame_id":<int>,"path_str_id":<int>,"func_str_id":<int>,
                "line":<int>,"end_line":<int>,"col":<int>,"end_col":<int>,
                "synthetic":true}, ...]}

   ``end_line``/``col``/``end_col`` are *omitted* when source location data
   is unavailable (a missing key means "not available", not zero or null).
   ``synthetic`` is present only on synthetic frames (for example, internal
   marker frames whose source location is None) and absent otherwise.

4. ``agg`` (zero or more)::

      {"type":"agg","v":1,"run_id":"<hex>","kind":"frame","scope":"final",
       "samples_total":<int>,
       "entries":[{"frame_id":<int>,"self":<int>,"cumulative":<int>}, ...]}

   ``self`` counts samples where the frame was the leaf (currently
   executing); ``cumulative`` counts samples where the frame appeared
   anywhere in the stack (deduped per sample so recursion does not
   double-count). ``samples_total`` is the run-wide total, repeated on
   each chunk so a streaming consumer always knows the denominator.

5. ``end`` (exactly one, last line)::

      {"type":"end","v":1,"run_id":"<hex>","samples_total":<int>}

   Presence of ``end`` is the consumer's signal that the file is complete.

Forward compatibility
=====================

Consumers MUST ignore unknown record ``"type"`` values and unknown object
fields. New fields will be added by adding optional keys; an incompatible
schema change will bump the per-record ``"v"``.
"""

from collections import Counter
import json
import uuid
from itertools import batched

from .constants import (
    PROFILING_MODE_ALL,
    PROFILING_MODE_CPU,
    PROFILING_MODE_EXCEPTION,
    PROFILING_MODE_GIL,
    PROFILING_MODE_WALL,
)
from .collector import normalize_location
from .stack_collector import StackTraceCollector


_CHUNK_SIZE = 256

_MODE_NAMES = {
    PROFILING_MODE_WALL: "wall",
    PROFILING_MODE_CPU: "cpu",
    PROFILING_MODE_GIL: "gil",
    PROFILING_MODE_ALL: "all",
    PROFILING_MODE_EXCEPTION: "exception",
}


class JsonlCollector(StackTraceCollector):
    """Collector that exports finalized profiling data as JSONL.

    See the module docstring for the full record schema. The collector
    accumulates samples in memory and writes the complete file at
    ``export()`` time.
    """

    def __init__(self, sample_interval_usec, *, skip_idle=False, mode=None):
        super().__init__(sample_interval_usec, skip_idle=skip_idle)
        self.run_id = uuid.uuid4().hex

        self._string_to_id = {}
        self._strings = []

        self._frame_to_id = {}
        self._frames = []

        self._frame_self = Counter()
        self._frame_cumulative = Counter()
        self._samples_total = 0
        self._seen_frame_ids = set()

        self._mode = mode

    def process_frames(self, frames, _thread_id, weight=1):
        self._samples_total += weight
        self._seen_frame_ids.clear()

        for i, (filename, location, funcname, _opcode) in enumerate(frames):
            frame_id = self._get_or_create_frame_id(
                filename, location, funcname
            )
            is_leaf = i == 0
            count_cumulative = frame_id not in self._seen_frame_ids

            if count_cumulative:
                self._seen_frame_ids.add(frame_id)

            if is_leaf:
                self._frame_self[frame_id] += weight

            if count_cumulative:
                self._frame_cumulative[frame_id] += weight

    def export(self, filename):
        with open(filename, "w", encoding="utf-8") as output:
            self._write_message(output, self._build_meta_record())
            self._write_chunked_records(
                output,
                {"type": "str_def", "v": 1, "run_id": self.run_id},
                "defs",
                self._strings,
            )
            self._write_chunked_records(
                output,
                {"type": "frame_def", "v": 1, "run_id": self.run_id},
                "defs",
                self._frames,
            )
            self._write_chunked_records(
                output,
                {
                    "type": "agg",
                    "v": 1,
                    "run_id": self.run_id,
                    "kind": "frame",
                    "scope": "final",
                    "samples_total": self._samples_total,
                },
                "entries",
                self._iter_final_agg_entries(),
            )
            self._write_message(output, self._build_end_record())

    def _build_meta_record(self):
        record = {
            "type": "meta",
            "v": 1,
            "run_id": self.run_id,
            "sample_interval_usec": self.sample_interval_usec,
        }

        if self._mode is not None:
            record["mode"] = _MODE_NAMES.get(self._mode, str(self._mode))

        return record

    def _build_end_record(self):
        record = {
            "type": "end",
            "v": 1,
            "run_id": self.run_id,
            "samples_total": self._samples_total,
        }

        return record

    def _iter_final_agg_entries(self):
        for frame_record in self._frames:
            frame_id = frame_record["frame_id"]
            yield {
                "frame_id": frame_id,
                "self": self._frame_self[frame_id],
                "cumulative": self._frame_cumulative[frame_id],
            }

    def _get_or_create_frame_id(self, filename, location, funcname):
        synthetic = location is None
        location_fields = self._location_to_export_fields(location)
        func_str_id = self._intern_string(funcname)
        path_str_id = self._intern_string(filename)

        frame_key = (
            path_str_id,
            func_str_id,
            location_fields["line"],
            location_fields.get("end_line"),
            location_fields.get("col"),
            location_fields.get("end_col"),
            synthetic,
        )

        if (frame_id := self._frame_to_id.get(frame_key)) is not None:
            return frame_id

        frame_id = len(self._frames) + 1
        frame_record = {
            "frame_id": frame_id,
            "path_str_id": path_str_id,
            "func_str_id": func_str_id,
            **location_fields,
        }
        if synthetic:
            frame_record["synthetic"] = True

        self._frame_to_id[frame_key] = frame_id
        self._frames.append(frame_record)
        return frame_id

    def _intern_string(self, value):
        value = str(value)

        if (string_id := self._string_to_id.get(value)) is not None:
            return string_id

        string_id = len(self._strings) + 1
        self._string_to_id[value] = string_id
        self._strings.append({"str_id": string_id, "value": value})
        return string_id

    @staticmethod
    def _location_to_export_fields(location):
        lineno, end_lineno, col_offset, end_col_offset = normalize_location(
            location
        )

        fields = {"line": lineno}
        if end_lineno > 0:
            fields["end_line"] = end_lineno
        if col_offset >= 0:
            fields["col"] = col_offset
        if end_col_offset >= 0:
            fields["end_col"] = end_col_offset
        return fields

    def _write_chunked_records(
        self, output, base_record, chunk_field, entries
    ):
        for chunk in batched(entries, _CHUNK_SIZE):
            self._write_message(output, {**base_record, chunk_field: chunk})

    @staticmethod
    def _write_message(output, record):
        output.write(json.dumps(record, separators=(",", ":")))
        output.write("\n")
