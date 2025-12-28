"""Thin Python wrapper around C binary reader for profiling data."""

import _remote_debugging

from .gecko_collector import GeckoCollector
from .stack_collector import FlamegraphCollector, CollapsedStackCollector
from .pstats_collector import PStatsCollector


class BinaryReader:
    """High-performance binary reader using C implementation.

    This reader uses memory-mapped I/O (on Unix) for fast replay of
    profiling data from binary files.

    Use as a context manager:
        with BinaryReader('profile.bin') as reader:
            info = reader.get_info()
            reader.replay_samples(collector, progress_callback)
    """

    def __init__(self, filename):
        """Create a new binary reader.

        Args:
            filename: Path to input binary file
        """
        self.filename = filename
        self._reader = None

    def __enter__(self):
        self._reader = _remote_debugging.BinaryReader(self.filename)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self._reader is not None:
            self._reader.close()
            self._reader = None
        return False

    def get_info(self):
        """Get metadata about the binary file.

        Returns:
            dict: File metadata including:
                - sample_count: Number of samples in the file
                - sample_interval_us: Sampling interval in microseconds
                - start_time_us: Start timestamp in microseconds
                - string_count: Number of unique strings
                - frame_count: Number of unique frames
                - compression: Compression type used
        """
        if self._reader is None:
            raise RuntimeError("Reader not open. Use as context manager.")
        return self._reader.get_info()

    def replay_samples(self, collector, progress_callback=None):
        """Replay samples from binary file through a collector.

        This allows converting binary profiling data to other formats
        (e.g., flamegraph, pstats) by replaying through the appropriate
        collector.

        Args:
            collector: A Collector instance with a collect() method
            progress_callback: Optional callable(current, total) for progress

        Returns:
            int: Number of samples replayed
        """
        if self._reader is None:
            raise RuntimeError("Reader not open. Use as context manager.")
        return self._reader.replay(collector, progress_callback)

    @property
    def sample_count(self):
        if self._reader is None:
            raise RuntimeError("Reader not open. Use as context manager.")
        return self._reader.get_info()['sample_count']

    def get_stats(self):
        """Get reconstruction statistics from replay.

        Returns:
            dict: Statistics about record types decoded and samples
                  reconstructed during replay.
        """
        if self._reader is None:
            raise RuntimeError("Reader not open. Use as context manager.")
        return self._reader.get_stats()


def convert_binary_to_format(input_file, output_file, output_format,
                             sample_interval_usec=None, progress_callback=None):
    """Convert a binary profiling file to another format.

    Args:
        input_file: Path to input binary file
        output_file: Path to output file
        output_format: Target format ('flamegraph', 'collapsed', 'pstats', etc.)
        sample_interval_usec: Override sample interval (uses file's if None)
        progress_callback: Optional callable(current, total) for progress

    Returns:
        int: Number of samples converted
    """
    with BinaryReader(input_file) as reader:
        info = reader.get_info()
        interval = sample_interval_usec or info['sample_interval_us']

        # Create appropriate collector based on format
        if output_format == 'flamegraph':
            collector = FlamegraphCollector(interval)
        elif output_format == 'collapsed':
            collector = CollapsedStackCollector(interval)
        elif output_format == 'pstats':
            collector = PStatsCollector(interval)
        elif output_format == 'gecko':
            collector = GeckoCollector(interval)
        else:
            raise ValueError(f"Unknown output format: {output_format}")

        # Replay samples through collector
        count = reader.replay_samples(collector, progress_callback)

        # Export to target format
        collector.export(output_file)

        return count
