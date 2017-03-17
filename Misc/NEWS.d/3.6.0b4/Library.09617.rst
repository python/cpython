Issue #28613: Fix get_event_loop() return the current loop if
called from coroutines/callbacks.