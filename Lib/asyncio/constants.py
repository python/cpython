# After the connection is lost, log warnings after this many write()s.
LOG_THRESHOLD_FOR_CONNLOST_WRITES = 5

# Seconds to wait before retrying accept().
ACCEPT_RETRY_DELAY = 1

# Number of stack entries to capture in debug mode.
# The larger the number, the slower the operation in debug mode
# (see extract_stack() in format_helpers.py).
DEBUG_STACK_DEPTH = 10

# Number of seconds to wait for SSL handshake to complete
SSL_HANDSHAKE_TIMEOUT = 10.0
