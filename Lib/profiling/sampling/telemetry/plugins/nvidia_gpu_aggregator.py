"""NVIDIA GPU aggregation utilities for telemetry plugin."""

from __future__ import annotations

from collections import Counter, defaultdict, deque


class NvidiaGpuAggregator:
    """In-memory aggregator for NVIDIA PM/PC telemetry."""

    def __init__(self, *, window_size=1024):
        self.window_size = window_size
        self.pm_samples = deque(maxlen=window_size)
        self.pc_samples = deque(maxlen=window_size)
        self.metadata = {}

        self._pm_values = defaultdict(list)
        self._kernel_counter = Counter()
        self._stall_counter = Counter()

    def set_metadata(self, metadata):
        if metadata:
            self.metadata.update(metadata)

    def add_pm(self, sample):
        self.pm_samples.append(sample)
        metrics = sample.get("metrics", {})
        for name, value in metrics.items():
            try:
                v = float(value)
            except (TypeError, ValueError):
                continue
            self._pm_values[name].append(v)

    def add_pc(self, sample):
        self.pc_samples.append(sample)
        count = int(sample.get("sample_count", 1) or 1)
        kernel = sample.get("kernel_name")
        stall = sample.get("stall_reason")
        if kernel:
            self._kernel_counter[kernel] += count
        if stall:
            self._stall_counter[stall] += count

    def snapshot(self):
        pm_summary = {}
        for name, values in self._pm_values.items():
            if not values:
                continue
            pm_summary[name] = {
                "avg": sum(values) / len(values),
                "min": min(values),
                "max": max(values),
            }

        return {
            "metadata": dict(self.metadata),
            "pm_count": len(self.pm_samples),
            "pc_count": len(self.pc_samples),
            "pm_summary": pm_summary,
            "top_kernels": self._kernel_counter.most_common(6),
            "top_stalls": self._stall_counter.most_common(6),
        }
