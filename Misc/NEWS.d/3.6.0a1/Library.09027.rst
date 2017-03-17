Issue #27057: Fix os.set_inheritable() on Android, ioctl() is blocked by
SELinux and fails with EACCESS. The function now falls back to fcntl().
Patch written by Michał Bednarski.