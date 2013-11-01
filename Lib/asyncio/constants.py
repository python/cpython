"""Constants."""

# After the connection is lost, log warnings after this many write()s.
LOG_THRESHOLD_FOR_CONNLOST_WRITES = 5

# Seconds to wait before retrying accept().
ACCEPT_RETRY_DELAY = 1
