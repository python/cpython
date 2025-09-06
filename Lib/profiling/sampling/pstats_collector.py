import collections
import marshal

from .collector import Collector


class PstatsCollector(Collector):
    def __init__(self, sample_interval_usec):
        self.result = collections.defaultdict(
            lambda: dict(total_rec_calls=0, direct_calls=0, cumulative_calls=0)
        )
        self.stats = {}
        self.sample_interval_usec = sample_interval_usec
        self.callers = collections.defaultdict(
            lambda: collections.defaultdict(int)
        )

    def collect(self, stack_frames):
        for thread_id, frames in stack_frames:
            if not frames:
                continue

            # Process each frame in the stack to track cumulative calls
            for frame in frames:
                location = (frame.filename, frame.lineno, frame.funcname)
                self.result[location]["cumulative_calls"] += 1

            # The top frame gets counted as an inline call (directly executing)
            top_frame = frames[0]
            top_location = (
                top_frame.filename,
                top_frame.lineno,
                top_frame.funcname,
            )

            self.result[top_location]["direct_calls"] += 1

            # Track caller-callee relationships for call graph
            for i in range(1, len(frames)):
                callee_frame = frames[i - 1]
                caller_frame = frames[i]

                callee = (
                    callee_frame.filename,
                    callee_frame.lineno,
                    callee_frame.funcname,
                )
                caller = (
                    caller_frame.filename,
                    caller_frame.lineno,
                    caller_frame.funcname,
                )

                self.callers[callee][caller] += 1

    def export(self, filename):
        self.create_stats()
        self._dump_stats(filename)

    def _dump_stats(self, file):
        stats_with_marker = dict(self.stats)
        stats_with_marker[("__sampled__",)] = True
        with open(file, "wb") as f:
            marshal.dump(stats_with_marker, f)

    # Needed for compatibility with pstats.Stats
    def create_stats(self):
        sample_interval_sec = self.sample_interval_usec / 1_000_000
        callers = {}
        for fname, call_counts in self.result.items():
            total = call_counts["direct_calls"] * sample_interval_sec
            cumulative_calls = call_counts["cumulative_calls"]
            cumulative = cumulative_calls * sample_interval_sec
            callers = dict(self.callers.get(fname, {}))
            self.stats[fname] = (
                call_counts["direct_calls"],  # cc = direct calls for sample percentage
                cumulative_calls,  # nc = cumulative calls for cumulative percentage
                total,
                cumulative,
                callers,
            )
