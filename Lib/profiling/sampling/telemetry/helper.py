"""Generic helper-process telemetry entrypoint."""

from __future__ import annotations

import argparse
import json
import time

from .plugin_registry import run_helper_plugin


def _emit(plugin_id, event_type, payload):
    print(
        json.dumps(
            {
                "plugin": plugin_id,
                "event_type": event_type,
                "payload": payload,
            },
            separators=(",", ":"),
        ),
        flush=True,
    )


def main():
    parser = argparse.ArgumentParser(description="Telemetry helper process")
    parser.add_argument("--plugin", required=True)
    parser.add_argument("--config-json", default="{}")
    args = parser.parse_args()

    try:
        config = json.loads(args.config_json)
    except json.JSONDecodeError:
        config = {}

    run_helper_plugin(args.plugin, config, _emit)
    # Unknown plugins return immediately; keep helper alive with heartbeats.
    while True:
        _emit(args.plugin, "heartbeat", {"timestamp_us": int(time.monotonic() * 1_000_000)})
        time.sleep(1.0)


if __name__ == "__main__":
    main()
