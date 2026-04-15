"""Telemetry plugin interfaces."""

from __future__ import annotations


class LiveTelemetryPlugin:
    """Base interface for telemetry plugins that render in live view."""

    plugin_id = "base"
    panel_modes = ("default",)

    def ingest(self, event_type, payload):
        raise NotImplementedError

    def render_lines(self, mode, width):
        raise NotImplementedError
