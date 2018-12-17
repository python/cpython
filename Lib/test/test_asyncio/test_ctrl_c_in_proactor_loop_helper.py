import sys


def do_in_child_process():
    import asyncio
    asyncio.set_event_loop_policy(
            asyncio.WindowsProactorEventLoopPolicy())
    l = asyncio.get_event_loop()
    try:
        print("start")
        sys.stdout.flush()
        l.run_forever()
    except KeyboardInterrupt:
        # ok
        sys.exit(255)
    except:
        # error - use default exit code
        pass


def do_in_main_process():
    import os
    import signal
    import subprocess
    from test.support.script_helper import spawn_python
    
    ok = False
    with spawn_python(__file__, "--child") as p:
        try:
            ready = p.stdout.readline()
            if ready != b"start\r\n":
                raise Exception(f"Unexpected line: got {ready}, expected 'start'")
            # ignore ctrl-c in current process
            signal.signal(signal.SIGINT, signal.SIG_IGN)
            os.kill(p.pid, signal.CTRL_C_EVENT)
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