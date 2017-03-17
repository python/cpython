Issue #26032: Optimized globbing in pathlib by using os.scandir(); it is now
about 1.5--4 times faster.