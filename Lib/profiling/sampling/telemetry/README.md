# Telemetry Plugin Architecture (`profiling.sampling`)

This document describes the telemetry plugin system used by `profiling.sampling`,
including live rendering, helper-process collection, binary sidecar persistence,
and replay.

It is intended for contributors adding or modifying telemetry plugins (for
example GPU, CPU, runtime counters, system metrics).

---

## Goals

- Keep core sampling (`SampleProfiler`) independent from vendor/provider logic.
- Collect telemetry out-of-process via helper subprocesses.
- Persist plugin events alongside binary profile output.
- Replay telemetry into collectors in a plugin-agnostic way.
- Support multiple plugins in a single profiling run.
- Allow adding new plugins by dropping modules into `telemetry/plugins/`.

---

## High-Level Flow

1. CLI enables one or more plugins via `--plugin`.
2. CLI builds telemetry plugin entries and resolves helper config.
3. `SampleProfiler` creates one `TelemetryHelperManager` per plugin.
4. Each manager launches `python -m profiling.sampling.telemetry --plugin ...`.
5. Helper emits JSON lines (`plugin`, `event_type`, `payload`) over stdout.
6. Profiler polls all managers and forwards events to collector via:
   - `collect_plugin_event(plugin_id, event_type, payload)`
7. In binary mode, `BinaryCollector` writes telemetry events to
   `<binary>.telemetrychunks`.
8. During replay, `replay_sidecar_to_sink()` reads sidecar records and routes
   events back into the sink collector.
9. In live mode, `LiveStatsCollector` instantiates live plugin adapters and
   renders plugin panels in the TUI.

---

## Key Modules

- `telemetry/plugin_registry.py`
  - Dynamic plugin discovery from `telemetry/plugins/`.
  - Dispatches to plugin module hooks for:
    - live plugin creation
    - helper config resolution
    - helper execution

- `telemetry/helper.py`
  - Generic helper-process entrypoint.
  - Parses args, invokes registry dispatch, and emits JSON events.

- `telemetry/manager.py`
  - One manager per plugin helper process.
  - Starts subprocess, polls non-blocking stdout lines, stops process.

- `telemetry/chunks.py`
  - Sidecar writer/reader (`.telemetrychunks`).
  - Line-delimited JSON records with magic header.

- `telemetry/replay.py`
  - Replays sidecar data into sinks that implement plugin event API.

- `telemetry/interfaces.py`
  - Defines `LiveTelemetryPlugin` interface for TUI rendering adapters.

- `telemetry/plugins/`
  - Plugin modules (auto-discovered).
  - Current example: `nvidia_gpu.py` (scaffold).

---

## Dynamic Plugin Discovery Contract

`plugin_registry` scans `telemetry/plugins/` modules and registers those that
declare `PLUGIN_ID`.

A plugin module should define:

- `PLUGIN_ID: str`
- `create_live_plugin() -> LiveTelemetryPlugin | None`
- `resolve_helper_config(config: dict | None) -> dict`
- `run_helper(config: dict, emit: callable) -> None`

Only `PLUGIN_ID` is required for discovery; missing hooks degrade gracefully:

- Missing `create_live_plugin`: plugin not available in live panel.
- Missing `resolve_helper_config`: config passes through unchanged.
- Missing `run_helper`: helper emits metadata error note.

---

## Event Format

Helper emits line-delimited JSON objects:

```json
{
  "plugin": "<plugin_id>",
  "event_type": "<string>",
  "payload": { "...": "plugin-defined" }
}
```

Notes:
- `event_type` semantics are plugin-specific.
- `payload` schema is plugin-defined and versioned by plugin behavior.

---

## Multi-Plugin Behavior

Multi-plugin runs are supported end-to-end:

- One helper subprocess per plugin.
- One shared sidecar writer that multiplexes all plugin records.
- Records are demultiplexed by `plugin` field during replay.
- Live UI keeps plugin runtimes in a map and can cycle plugin panels (`n` key).

This is expected and intentional; a single sidecar stream stores all plugins.

---

## CLI Integration

- Enable plugins with repeatable `--plugin`:
  - `--plugin nvidia_gpu`
  - `--plugin nvidia_gpu --plugin another_plugin`

- CLI does not hardcode provider details. It asks the registry to resolve helper
  config for each plugin via `resolve_helper_config(plugin_id)`.

### Plugin-specific CLI subflags

Plugins can define their own flags for plugin-specific controls, such as:

- `--nvidia-device`
- `--nvidia-pm` / `--no-nvidia-pm`
- `--nvidia-pc` / `--no-nvidia-pc`

Recommended ownership model:

- Plugin module defines:
  - `register_cli_flags(parser)` to add plugin-specific args.
  - `extract_cli_config(args)` to convert parsed args to plugin config overrides.
- CLI remains generic:
  - discovers plugins
  - asks enabled plugins to register flags
  - merges defaults from `resolve_helper_config(plugin_id)` with plugin overrides

Status: this hook pattern is documented and recommended, but not yet implemented
in the current CLI wiring.

Current validation:
- In non-live mode, telemetry plugins require `--binary` output.

---

## Live UI Integration

`LiveStatsCollector` handles generic plugin rendering:

- `set_plugin_enabled(plugin_id)` creates plugin runtime from registry.
- `collect_plugin_event(...)` routes incoming events to plugin `ingest(...)`.
- `TelemetryPanelWidget` renders current plugin/mode lines.
- Panel controls:
  - `g`: toggle telemetry panel
  - `n`: cycle plugin
  - `m`: cycle plugin mode

---

## Binary Persistence and Replay

### Sidecar path

For `profile.bin`, sidecar is:

- `profile.bin.telemetrychunks`

### Sidecar record

Each line after magic header `TCHUNK1` is a compact JSON record with:
- `plugin`
- `event_type`
- `payload`

### Replay

`replay_sidecar_to_sink(binary_filename, sink)`:
- Reads sidecar records.
- Calls `sink.set_plugin_enabled(plugin_id)` once per seen plugin (if present).
- Calls `sink.collect_plugin_event(plugin_id, event_type, payload)` for each event.

---

## Adding a New Plugin

1. Create module in `telemetry/plugins/`, e.g. `my_plugin.py`.
2. Add:
   - `PLUGIN_ID = "my_plugin"`
   - `create_live_plugin()`
   - `resolve_helper_config(config)`
   - `run_helper(config, emit)`
   - optional: `register_cli_flags(parser)`
   - optional: `extract_cli_config(args)`
3. Implement a live adapter class extending `LiveTelemetryPlugin`:
   - `ingest(event_type, payload)`
   - `render_lines(mode, width)`
4. Enable from CLI:
   - `--plugin my_plugin`

No `plugin_registry.py` edits are required when contract is followed.

---

## Current NVIDIA Plugin Status

`telemetry/plugins/nvidia_gpu.py` is currently a scaffold:

- Emits synthetic PM/PC events.
- Includes helper config defaults (`provider/device/pm/pc`).
- Live panel supports `pc` and `pm` modes.

It is designed as a placeholder for real NVIDIA integration (for example CUPTI /
NVPerf / Nsight-backed collection).

---

## Known Constraints and Follow-Ups

- Helper protocol is JSON lines over stdout (simple, portable, not binary-fast).
- Sidecar payloads are untyped at framework level (plugin-owned schema).
- No plugin capability negotiation yet (beyond graceful missing-hook handling).
- Unknown plugin in helper emits metadata note and heartbeat loop continues.
- Non-live plugin capture currently expects binary format.

Potential future improvements:
- Plugin metadata schema/versioning.
- Optional plugin-specific CLI overrides.
- Plugin capability introspection for UI and validation.
- Robust helper health/failure reporting in collectors/UI.

1q