"""Manage helper-process telemetry plugins."""

from __future__ import annotations

import json
import selectors
import subprocess
import sys


class TelemetryHelperManager:
    """Single-plugin helper process manager."""

    def __init__(self, plugin_id, plugin_config=None):
        self.plugin_id = plugin_id
        self.plugin_config = plugin_config or {}
        self.process = None
        self.selector = selectors.DefaultSelector()
        self._started = False

    def start(self):
        if self._started:
            return
        args = [
            sys.executable,
            "-m",
            "profiling.sampling.telemetry",
            "--plugin",
            self.plugin_id,
            "--config-json",
            json.dumps(self.plugin_config, separators=(",", ":")),
        ]
        self.process = subprocess.Popen(
            args,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            stdin=subprocess.DEVNULL,
            text=True,
            bufsize=1,
        )
        if self.process.stdout is not None:
            self.selector.register(self.process.stdout, selectors.EVENT_READ)
        self._started = True

    def poll(self):
        if not self._started or self.process is None:
            return []
        events = []
        for key, _ in self.selector.select(timeout=0):
            line = key.fileobj.readline()
            if not line:
                continue
            line = line.strip()
            if not line:
                continue
            try:
                events.append(json.loads(line))
            except json.JSONDecodeError:
                continue
        return events

    def stop(self):
        if self.process is not None:
            if self.process.poll() is None:
                self.process.terminate()
                try:
                    self.process.wait(timeout=1.0)
                except subprocess.TimeoutExpired:
                    self.process.kill()
            self.process = None
        self._started = False
