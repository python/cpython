import sys


def do_in_child_process():
    import asyncio

    asyncio.set_event_loop_policy(asyncio.WindowsProactorEventLoopPolicy())
    l = asyncio.get_event_loop()

    def step(n):
        try:
            print(n)
            sys.stdout.flush()
            l.run_forever()
            sys.exit(100)
        except KeyboardInterrupt:
            # ok
            pass
        except:
            # error - use default exit code
            sys.exit(200)

    step(1)
    step(2)
    sys.exit(255)


def do_in_main_process():
    import os
    import signal
    import subprocess
    import time
    from test.support.script_helper import spawn_python

    ok = False

    def step(p, expected):
        s = p.stdout.readline()
        if s != expected:
            raise Exception(f"Unexpected line: got {s}, expected '{expected}'")
        # ensure that child process gets to run_forever
        time.sleep(0.5)
        os.kill(p.pid, signal.CTRL_C_EVENT)

    with spawn_python(__file__, "--child") as p:
        try:
            # ignore ctrl-c in current process
            signal.signal(signal.SIGINT, signal.SIG_IGN)
            step(p, b"1\r\n")
            step(p, b"2\r\n")
            exit_code = p.wait(timeout=5)
            ok = exit_code = 255
        except Exception as e:
            sys.stderr.write(repr(e))
            p.kill()
    sys.exit(255 if ok else 1)


if __name__ == "__main__":
    if len(sys.argv) == 1:
        do_in_main_process()
    else:
        do_in_child_process()
