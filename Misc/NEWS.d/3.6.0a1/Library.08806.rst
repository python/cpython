Issue #25319: When threading.Event is reinitialized, the underlying condition
should use a regular lock rather than a recursive lock.