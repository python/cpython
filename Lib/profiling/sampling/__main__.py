"""Run the sampling profiler from the command line."""

import sys

MACOS_PERMISSION_ERROR = """\
🔒 Permission Error: Unable to access process memory on macOS

Tachyon needs elevated permissions to profile processes. Try one of these solutions:

1. Try running again with elevated permissions by running 'sudo -E !!'.

2. If targeting system Python processes:
   Note: Apple's System Integrity Protection (SIP) may block access to system
   Python binaries. Consider using a user-installed Python instead.
"""

LINUX_PERMISSION_ERROR = """
🔒 Tachyon was unable to access process memory. This could be because tachyon
has insufficient privileges (the required capability is CAP_SYS_PTRACE).
Unprivileged processes cannot trace processes that they cannot send signals
to or those running set-user-ID/set-group-ID programs, for security reasons.

If your uid matches the uid of the target process you want to analyze, you
can do one of the following to get 'ptrace' scope permissions:

* If you are running inside a Docker container, you need to make sure you
  start the container using the '--cap-add=SYS_PTRACE' or '--privileged'
  command line arguments. Notice that this may not be enough if you are not
  running as 'root' inside the Docker container as you may need to disable
  hardening (see next points).

* Try running again with elevated permissions by running 'sudo -E !!'.

* You can disable kernel hardening for the current session temporarily (until
  a reboot happens) by running 'echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope'.
"""

WINDOWS_PERMISSION_ERROR = """
🔒 Tachyon requires administrator rights to access process memory on Windows.
Please run your command prompt as Administrator and try again.
"""

GENERIC_PERMISSION_ERROR = """
🔒 Tachyon was unable to access the target process due to operating
system restrictions or missing privileges.
"""

from .cli import main

def handle_permission_error():
    """Handle PermissionError by displaying appropriate error message."""
    if sys.platform == "darwin":
        print(MACOS_PERMISSION_ERROR, file=sys.stderr)
    elif sys.platform.startswith("linux"):
        print(LINUX_PERMISSION_ERROR, file=sys.stderr)
    elif sys.platform.startswith("win"):
        print(WINDOWS_PERMISSION_ERROR, file=sys.stderr)
    else:
        print(GENERIC_PERMISSION_ERROR, file=sys.stderr)
    sys.exit(1)

if __name__ == '__main__':
    try:
        main()
    except PermissionError:
        handle_permission_error()
