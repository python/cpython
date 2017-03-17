Issue #23804: Fix SSL zero-length recv() calls to not block and not raise
an error about unclean EOF.