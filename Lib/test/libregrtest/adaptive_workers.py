"""Load-average-dependent worker admission using the existing load tracker."""

import math
import os
import threading


class AdaptiveWorkerGate:
    # Heuristic: avoid rapid adjustments against a smoothed load measurement.
    update_interval = 5.0
    # Heuristic, not empirically tuned: hold concurrency within 3% of the
    # usable CPU count on either side of the target to reduce oscillation.
    target_tolerance = 0.03

    def __init__(self, target, maximum, logger):
        self.target = target
        self.maximum = maximum
        self.get_load_avg = logger.get_load_avg
        self.cpu_count = os.process_cpu_count() or 1
        # Load is system-wide, so this only approximates affinity restrictions.
        self.load_target = target * self.cpu_count
        self.initial_limit = min(maximum, max(1, math.ceil(self.load_target)))
        self.limit = self.initial_limit
        self.active = 0
        self.stopped = False
        self.condition = threading.Condition()

    def acquire(self):
        with self.condition:
            self.condition.wait_for(lambda: self.stopped or self.active < self.limit)
            if self.stopped:
                return False
            self.active += 1
            return True

    def release(self):
        with self.condition:
            self.active -= 1
            self.condition.notify_all()

    def get_active_workers(self):
        with self.condition:
            return self.active

    def update(self):
        try:
            load = self.get_load_avg()
        except (OSError, ValueError):
            load = None
        with self.condition:
            if load is None or not math.isfinite(load) or load < 0:
                self.limit = self.initial_limit
                self.condition.notify_all()
                return
            tolerance = self.target_tolerance * self.cpu_count
            if load < self.load_target - tolerance:
                self.limit = min(self.maximum, self.limit + 1)
            elif load > self.load_target + tolerance:
                self.limit = max(1, self.limit - 1)
            self.condition.notify_all()

    def stop(self):
        with self.condition:
            self.stopped = True
            self.condition.notify_all()
