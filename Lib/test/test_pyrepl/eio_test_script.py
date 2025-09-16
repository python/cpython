import errno
import fcntl
import os
import pty
import signal
import sys
import termios


def handler(sig, f):
    pass


def create_eio_condition():
    # SIGINT handler used to produce an EIO.
    # See https://github.com/python/cpython/issues/135329.
    try:
        master_fd, slave_fd = pty.openpty()
        child_pid = os.fork()
        if child_pid == 0:
            try:
                os.setsid()
                fcntl.ioctl(slave_fd, termios.TIOCSCTTY, 0)
                child_process_group_id = os.getpgrp()
                grandchild_pid = os.fork()
                if grandchild_pid == 0:
                    os.setpgid(0, 0)      # set process group for grandchild
                    os.dup2(slave_fd, 0)  # redirect stdin
                    if slave_fd > 2:
                        os.close(slave_fd)
                    # Fork grandchild for terminal control manipulation
                    if os.fork() == 0:
                        sys.exit(0)  # exit the child process that was just obtained
                    else:
                        try:
                            os.tcsetpgrp(0, child_process_group_id)
                        except OSError:
                            pass
                        sys.exit(0)
                else:
                    # Back to child
                    try:
                        os.setpgid(grandchild_pid, grandchild_pid)
                    except ProcessLookupError:
                        pass
                    os.tcsetpgrp(slave_fd, grandchild_pid)
                    if slave_fd > 2:
                        os.close(slave_fd)
                    os.waitpid(grandchild_pid, 0)
                    # Manipulate terminal control to create EIO condition
                    os.tcsetpgrp(master_fd, child_process_group_id)
                    # Now try to read from master - this might cause EIO
                    try:
                        os.read(master_fd, 1)
                    except OSError as e:
                        if e.errno == errno.EIO:
                            print(f"Setup created EIO condition: {e}", file=sys.stderr)
                    sys.exit(0)
            except Exception as setup_e:
                print(f"Setup error: {setup_e}", file=sys.stderr)
                sys.exit(1)
        else:
            # Parent process
            os.close(slave_fd)
            os.waitpid(child_pid, 0)
            # Now replace stdin with master_fd and try to read
            os.dup2(master_fd, 0)
            os.close(master_fd)
            # This should now trigger EIO
            print(f"Unexpectedly got input: {input()!r}", file=sys.stderr)
            sys.exit(0)
    except OSError as e:
        if e.errno == errno.EIO:
            print(f"Got EIO: {e}", file=sys.stderr)
            sys.exit(1)
        elif e.errno == errno.ENXIO:
            print(f"Got ENXIO (no such device): {e}", file=sys.stderr)
            sys.exit(1)  # Treat ENXIO as success too
        else:
            print(f"Got other OSError: errno={e.errno} {e}", file=sys.stderr)
            sys.exit(2)
    except EOFError as e:
        print(f"Got EOFError: {e}", file=sys.stderr)
        sys.exit(3)
    except Exception as e:
        print(f"Got unexpected error: {type(e).__name__}: {e}", file=sys.stderr)
        sys.exit(4)


if __name__ == "__main__":
    # Set up signal handler for coordination
    signal.signal(signal.SIGUSR1, lambda *a: create_eio_condition())
    print("READY", flush=True)
    signal.pause()
