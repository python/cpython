import collections
import os

from .collector import Collector


class StackTraceCollector(Collector):
    def __init__(self):
        self.call_trees = []
        self.function_samples = collections.defaultdict(int)

    def collect(self, stack_frames):
        for thread_id, frames in stack_frames:
            if frames:
                # Store the complete call stack (reverse order - root first)
                call_tree = list(reversed(frames))
                self.call_trees.append(call_tree)

                # Count samples per function
                for frame in frames:
                    self.function_samples[frame] += 1


class CollapsedStackCollector(StackTraceCollector):
    def export(self, filename):
        stack_counter = collections.Counter()
        for call_tree in self.call_trees:
            # Call tree is already in root->leaf order
            stack_str = ";".join(
                f"{os.path.basename(f[0])}:{f[2]}:{f[1]}" for f in call_tree
            )
            stack_counter[stack_str] += 1

        with open(filename, "w") as f:
            for stack, count in stack_counter.items():
                f.write(f"{stack} {count}\n")
        print(f"Collapsed stack output written to {filename}")
