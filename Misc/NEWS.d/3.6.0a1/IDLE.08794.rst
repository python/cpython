Issue #15348: Stop the debugger engine (normally in a user process)
before closing the debugger window (running in the IDLE process).
This prevents the RuntimeErrors that were being caught and ignored.