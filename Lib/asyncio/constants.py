"""Constants."""

# After the connection is lost, log warnings after this many write()s.
LOG_THRESHOLD_FOR_CONNLOST_WRITES = 5

# Seconds to wait before retrying accept().
ACCEPT_RETRY_DELAY = 1

# Number of stack entries to capture in debug mode.
# The larger the number, the slower the operation in debug mode
# (see extract_stack() in events.py).
DEBUG_STACK_DEPTH = 10
