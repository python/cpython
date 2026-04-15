"""Generic telemetry plugin framework for profiling.sampling."""

from .chunks import TelemetryChunkReader, TelemetryChunkWriter, sidecar_path_for_binary
from .manager import TelemetryHelperManager
from .plugin_registry import create_live_plugin, resolve_helper_config
from .interfaces import LiveTelemetryPlugin
from .replay import replay_sidecar_to_sink

__all__ = (
    "TelemetryChunkReader",
    "TelemetryChunkWriter",
    "TelemetryHelperManager",
    "sidecar_path_for_binary",
    "LiveTelemetryPlugin",
    "create_live_plugin",
    "resolve_helper_config",
    "replay_sidecar_to_sink",
)
