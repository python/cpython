Issue #24137: Run IDLE, test_idle, and htest with tkinter default root
disabled.  Fix code and tests that fail with this restriction.  Fix htests to
not create a second and redundant root and mainloop.