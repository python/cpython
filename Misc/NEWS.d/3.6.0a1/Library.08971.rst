Issue #23804: Fix SSL recv(0) and read(0) methods to return zero bytes
instead of up to 1024.