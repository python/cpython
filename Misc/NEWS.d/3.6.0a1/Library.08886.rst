Issue #25717: Restore the previous behaviour of tolerating most fstat()
errors when opening files.  This was a regression in 3.5a1, and stopped
anonymous temporary files from working in special cases.