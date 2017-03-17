Issue #26735: Fix :func:`os.urandom` on Solaris 11.3 and newer when reading
more than 1,024 bytes: call ``getrandom()`` multiple times with a limit of
1024 bytes per call.