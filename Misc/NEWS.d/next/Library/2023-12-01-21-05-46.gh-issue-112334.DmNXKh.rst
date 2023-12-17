Fixed a performance regression in 3.12's :mod:`subprocess` on Linux where it
would no longer use the fast-path ``vfork()`` system call when it could have
due to a logic bug, instead falling back to the safe but slower ``fork()``.

Also fixed a second 3.12.0 potential security bug.  If a value of
``extra_groups=[]`` was passed to :mod:`subprocess.Popen` or related APIs,
the underlying ``setgroups(0, NULL)`` system call to clear the groups list
would not be made in the child process prior to ``exec()``.

This was identified via code inspection in the process of fixing the first
bug.
