"""Plugin telemetry chunk sidecar read/write helpers."""

from __future__ import annotations

import json
import os

_MAGIC = "TCHUNK1"


def sidecar_path_for_binary(binary_filename):
    return f"{binary_filename}.telemetrychunks"


class TelemetryChunkWriter:
    def __init__(self, binary_filename):
        self.sidecar_path = sidecar_path_for_binary(binary_filename)
        self._fp = open(self.sidecar_path, "w", encoding="utf-8")
        self._fp.write(_MAGIC + "\n")

    def write_record(self, plugin_id, event_type, payload):
        record = {
            "plugin": plugin_id,
            "event_type": event_type,
            "payload": payload,
        }
        self._fp.write(json.dumps(record, separators=(",", ":")) + "\n")

    def close(self):
        if self._fp is not None:
            self._fp.close()
            self._fp = None


class TelemetryChunkReader:
    def __init__(self, binary_filename):
        self.sidecar_path = sidecar_path_for_binary(binary_filename)

    def exists(self):
        return os.path.exists(self.sidecar_path)

    def iter_records(self):
        if not self.exists():
            return
        with open(self.sidecar_path, "r", encoding="utf-8") as fp:
            first = fp.readline().rstrip("\n")
            if first != _MAGIC:
                return
            for line in fp:
                line = line.strip()
                if not line:
                    continue
                try:
                    yield json.loads(line)
                except json.JSONDecodeError:
                    continue
