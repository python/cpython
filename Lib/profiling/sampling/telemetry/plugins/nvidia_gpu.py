"""NVIDIA GPU telemetry plugin scaffold."""

from __future__ import annotations

import time

from ..interfaces import LiveTelemetryPlugin
from .nvidia_gpu_aggregator import NvidiaGpuAggregator

PLUGIN_ID = "nvidia_gpu"


class NvidiaGpuLivePlugin(LiveTelemetryPlugin):
    plugin_id = "nvidia_gpu"
    panel_modes = ("pc", "pm")

    def __init__(self):
        self.agg = NvidiaGpuAggregator()

    def ingest(self, event_type, payload):
        if event_type == "metadata":
            self.agg.set_metadata(payload)
        elif event_type == "pm":
            self.agg.add_pm(payload)
        elif event_type == "pc":
            self.agg.add_pc(payload)

    def render_lines(self, mode, width):
        s = self.agg.snapshot()
        lines = []
        if mode == "pm":
            lines.append("Metric                           avg      min      max")
            pm = s.get("pm_summary", {})
            if not pm:
                lines.append("No PM metrics yet.")
            else:
                for name, st in sorted(pm.items())[:6]:
                    lines.append(
                        f"{name:<30} {st['avg']:>7.1f}  {st['min']:>7.1f}  {st['max']:>7.1f}"
                    )
            return lines

        lines.append("Top kernels:")
        kernels = s.get("top_kernels", [])
        if not kernels:
            lines.append("  No PC samples yet.")
        else:
            for name, count in kernels[:3]:
                lines.append(f"  {name:<28} {count:>8}")
        lines.append("Top stall reasons:")
        stalls = s.get("top_stalls", [])
        if not stalls:
            lines.append("  No stall reason data yet.")
        else:
            for name, count in stalls[:3]:
                lines.append(f"  {name:<28} {count:>8}")
        return lines


def resolve_nvidia_gpu_helper_config(config=None):
    resolved = {
        "provider": "nvidia",
        "device": "0",
        "pm": True,
        "pc": True,
    }
    if config:
        resolved.update(dict(config))
    return resolved


def run_nvidia_gpu_helper(config, emit):
    """Helper-process loop for NVIDIA GPU plugin.

    This is still a scaffold implementation that emits synthetic PM/PC events.
    """
    pm = bool(config.get("pm", False))
    pc = bool(config.get("pc", False))
    device = str(config.get("device", "0"))
    plugin_id = "nvidia_gpu"

    emit(
        plugin_id,
        "metadata",
        {
            "provider": "nvidia",
            "device": device,
            "pm_enabled": pm,
            "pc_enabled": pc,
            "note": "helper-process scaffold",
        },
    )

    tick = 0
    while True:
        now_us = int(time.monotonic() * 1_000_000)
        emit(plugin_id, "heartbeat", {"timestamp_us": now_us})
        if pm:
            emit(
                plugin_id,
                "pm",
                {
                    "timestamp_us": now_us,
                    "metrics": {
                        "sm_throughput_pct": 45 + (tick % 25),
                        "dram_throughput_pct": 35 + (tick % 20),
                    },
                },
            )
        if pc:
            emit(
                plugin_id,
                "pc",
                {
                    "timestamp_us": now_us,
                    "kernel_name": "matmul_kernel",
                    "stall_reason": "Long Scoreboard",
                    "sample_count": 100 + (tick % 40),
                },
            )
        tick += 1
        time.sleep(0.25)


def create_live_plugin():
    return NvidiaGpuLivePlugin()


def resolve_helper_config(config=None):
    return resolve_nvidia_gpu_helper_config(config)


def run_helper(config, emit):
    run_nvidia_gpu_helper(config, emit)
