"""Thin Python wrapper around C binary writer for profiling data."""

import time

from .collector import Collector

# Compression type constants (must match binary_io.h)
COMPRESSION_NONE = 0
COMPRESSION_ZSTD = 1


def _resolve_compression(compression):
    """Resolve compression type from string or int.

    Args:
        compression: 'auto', 'zstd', 'none', or int (0/1)

    Returns:
        int: Compression type constant
    """
    if isinstance(compression, int):
        return compression

    compression = compression.lower()
    if compression == 'none':
        return COMPRESSION_NONE
    elif compression == 'zstd':
        return COMPRESSION_ZSTD
    elif compression == 'auto':
        # Auto: use zstd if available, otherwise none
        import _remote_debugging
        if _remote_debugging.zstd_available():
            return COMPRESSION_ZSTD
        return COMPRESSION_NONE
    else:
        raise ValueError(f"Unknown compression type: {compression}")


class BinaryCollector(Collector):
    """High-performance binary collector using C implementation.

    This collector writes profiling data directly to a binary file format
    with optional zstd compression. All I/O is performed in C for maximum
    throughput.

    The binary format uses string/frame deduplication and varint encoding
    for efficient storage.
    """

    def __init__(self, filename, sample_interval_usec, *, skip_idle=False,
                 compression='auto'):
        """Create a new binary collector.

        Args:
            filename: Path to output binary file
            sample_interval_usec: Sampling interval in microseconds
            skip_idle: If True, skip idle threads (not used in binary format)
            compression: 'auto', 'zstd', 'none', or int (0=none, 1=zstd)
        """
        import _remote_debugging

        self.filename = filename
        self.sample_interval_usec = sample_interval_usec
        self.skip_idle = skip_idle

        compression_type = _resolve_compression(compression)
        start_time_us = int(time.monotonic() * 1_000_000)
        self._writer = _remote_debugging.BinaryWriter(
            filename, sample_interval_usec, start_time_us, compression=compression_type
        )

    def collect(self, stack_frames, timestamp_us=None):
        """Collect profiling data from stack frames.

        This passes stack_frames directly to the C writer which handles
        all encoding and buffering.

        Args:
            stack_frames: List of InterpreterInfo objects from _remote_debugging
            timestamp_us: Optional timestamp in microseconds. If not provided,
                          uses time.monotonic() to generate one.
        """
        if timestamp_us is None:
            timestamp_us = int(time.monotonic() * 1_000_000)
        self._writer.write_sample(stack_frames, timestamp_us)

    def collect_failed_sample(self):
        """Record a failed sample attempt (no-op for binary format)."""
        pass

    def export(self, filename=None):
        """Finalize and close the binary file.

        Args:
            filename: Ignored (binary files are written incrementally)
        """
        self._writer.finalize()

    @property
    def total_samples(self):
        """Total number of samples written."""
        return self._writer.total_samples

    def get_stats(self):
        """Get encoding statistics.

        Returns:
            Dict with encoding statistics including repeat/full/suffix/pop-push
            record counts, frames written/saved, and compression ratio.
        """
        return self._writer.get_stats()

    def __enter__(self):
        """Context manager entry."""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit - finalize unless there was an error."""
        if exc_type is None:
            self._writer.finalize()
        else:
            self._writer.close()
        return False
