Issue #26590: Implement a safe finalizer for the _socket.socket type. It now
releases the GIL to close the socket.