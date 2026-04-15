"""Telemetry replay routing for sidecar chunks."""

from __future__ import annotations

from .chunks import TelemetryChunkReader


def replay_sidecar_to_sink(binary_filename, sink):
    """Replay telemetry sidecar records into a sink.

    Supported sink contract:
    - `collect_plugin_event(plugin_id, event_type, payload)`
    """
    reader = TelemetryChunkReader(binary_filename)
    if not reader.exists():
        return

    seen_plugins = set()
    for record in reader.iter_records():
        plugin_id = record.get("plugin")
        event_type = record.get("event_type")
        payload = record.get("payload", {})
        if not plugin_id or not event_type:
            continue

        if plugin_id not in seen_plugins and hasattr(sink, "set_plugin_enabled"):
            sink.set_plugin_enabled(plugin_id)
            seen_plugins.add(plugin_id)

        if hasattr(sink, "collect_plugin_event"):
            sink.collect_plugin_event(plugin_id, event_type, payload)
