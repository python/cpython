"""JSONL collector."""

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
from .stack_collector import StackTraceCollector


_CHUNK_SIZE = 1000

_MODE_NAMES = {
    PROFILING_MODE_WALL: "wall",
    PROFILING_MODE_CPU: "cpu",
    PROFILING_MODE_GIL: "gil",
    PROFILING_MODE_ALL: "all",
    PROFILING_MODE_EXCEPTION: "exception",
}


class JsonlCollector(StackTraceCollector):
    """Collector that exports finalized profiling data as JSONL."""

    def __init__(self, sample_interval_usec, *, skip_idle=False, mode=None):
        super().__init__(sample_interval_usec, skip_idle=skip_idle)
        self.run_id = uuid.uuid4().hex

        self._string_to_id = {}
        self._strings = []

        self._frame_to_id = {}
        self._frames = []

        self._frame_self = {}
        self._frame_cumulative = {}
        self._samples_total = 0

        self._mode = mode

    def process_frames(self, frames, _thread_id, weight=1):
        if not frames:
            return

        self._samples_total += weight

        frame_ids = [
            self._get_or_create_frame_id(filename, location, funcname)
            for filename, location, funcname, _opcode in frames
        ]
        leaf_frame_id = frame_ids[0]

        self._frame_self[leaf_frame_id] = (
            self._frame_self.get(leaf_frame_id, 0) + weight
        )

        for frame_id in set(frame_ids):
            self._frame_cumulative[frame_id] = (
                self._frame_cumulative.get(frame_id, 0) + weight
            )

    def export(self, filename):
        with open(filename, "w", encoding="utf-8") as output:
            self._write_message(output, self._build_meta_record())
            self._write_chunked_defs(output, "str_def", self._strings)
            self._write_chunked_defs(output, "frame_def", self._frames)
            self._write_chunked_agg(output, self._iter_agg_entries())
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

    def _get_or_create_frame_id(self, filename, location, funcname):
        synthetic = location is None
        location_fields = self._normalize_export_location(location)
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
    def _normalize_export_location(location):
        if location is None:
            return {"line": 0}

        if isinstance(location, int):
            return {"line": max(location, 0)}

        if not isinstance(location, tuple):
            lineno = getattr(location, "lineno", 0)
            location = (
                lineno,
                getattr(location, "end_lineno", lineno),
                getattr(location, "col_offset", -1),
                getattr(location, "end_col_offset", -1),
            )

        lineno, end_lineno, col_offset, end_col_offset = location
        if not isinstance(lineno, int) or lineno <= 0:
            return {"line": 0}

        normalized = {"line": lineno}
        if isinstance(end_lineno, int) and end_lineno > 0:
            normalized["end_line"] = end_lineno
        if isinstance(col_offset, int) and col_offset >= 0:
            normalized["col"] = col_offset
        if isinstance(end_col_offset, int) and end_col_offset >= 0:
            normalized["end_col"] = end_col_offset
        return normalized

    def _iter_agg_entries(self):
        entries = []
        for frame_record in self._frames:
            frame_id = frame_record["frame_id"]
            entries.append(
                {
                    "frame_id": frame_id,
                    "self": self._frame_self.get(frame_id, 0),
                    "cumulative": self._frame_cumulative.get(frame_id, 0),
                }
            )
        return entries

    def _write_chunked_defs(self, output, record_type, entries):
        for chunk in batched(entries, _CHUNK_SIZE):
            self._write_message(
                output,
                {
                    "type": record_type,
                    "v": 1,
                    "run_id": self.run_id,
                    "defs": chunk,
                },
            )

    def _write_chunked_agg(self, output, entries):
        for chunk in batched(entries, _CHUNK_SIZE):
            self._write_message(
                output,
                {
                    "type": "agg",
                    "v": 1,
                    "run_id": self.run_id,
                    "kind": "frame",
                    "scope": "final",
                    "samples_total": self._samples_total,
                    "entries": chunk,
                },
            )

    @staticmethod
    def _write_message(output, record):
        output.write(json.dumps(record, separators=(",", ":")))
        output.write("\n")
