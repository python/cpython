"""
Helper to run a script in a pseudo-terminal.
"""
import os
import selectors
import subprocess
import sys
from contextlib import ExitStack
from errno import EIO

from test.support.import_helper import import_module

def run_pty(script, input=b"dummy input\r", env=None):
    pty = import_module('pty')
    output = bytearray()
    [master, slave] = pty.openpty()
    args = (sys.executable, '-c', script)
    proc = subprocess.Popen(args, stdin=slave, stdout=slave, stderr=slave, env=env)
    os.close(slave)
    with ExitStack() as cleanup:
        cleanup.enter_context(proc)
        def terminate(proc):
            try:
                proc.terminate()
            except ProcessLookupError:
                # Workaround for Open/Net BSD bug (Issue 16762)
                pass
        cleanup.callback(terminate, proc)
        cleanup.callback(os.close, master)
        # Avoid using DefaultSelector and PollSelector. Kqueue() does not
        # work with pseudo-terminals on OS X < 10.9 (Issue 20365) and Open
        # BSD (Issue 20667). Poll() does not work with OS X 10.6 or 10.4
        # either (Issue 20472). Hopefully the file descriptor is low enough
        # to use with select().
        sel = cleanup.enter_context(selectors.SelectSelector())
        sel.register(master, selectors.EVENT_READ | selectors.EVENT_WRITE)
        os.set_blocking(master, False)
        while True:
            for [_, events] in sel.select():
                if events & selectors.EVENT_READ:
                    try:
                        chunk = os.read(master, 0x10000)
                    except OSError as err:
                        # Linux raises EIO when slave is closed (Issue 5380)
                        if err.errno != EIO:
                            raise
                        chunk = b""
                    if not chunk:
                        return output
                    output.extend(chunk)
                if events & selectors.EVENT_WRITE:
                    try:
                        input = input[os.write(master, input):]
                    except OSError as err:
                        # Apparently EIO means the slave was closed
                        if err.errno != EIO:
                            raise
                        input = b""  # Stop writing
                    if not input:
                        sel.modify(master, selectors.EVENT_READ)
