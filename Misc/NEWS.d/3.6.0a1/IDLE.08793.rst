Issue #24455: Prevent IDLE from hanging when a) closing the shell while the
debugger is active (15347); b) closing the debugger with the [X] button
(15348); and c) activating the debugger when already active (24455).
The patch by Mark Roseman does this by making two changes.
1. Suspend and resume the gui.interaction method with the tcl vwait
mechanism intended for this purpose (instead of root.mainloop & .quit).
2. In gui.run, allow any existing interaction to terminate first.