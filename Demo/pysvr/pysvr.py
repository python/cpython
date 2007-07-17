#! /usr/bin/env python

"""A multi-threaded telnet-like server that gives a Python prompt.

This is really a prototype for the same thing in C.

Usage: pysvr.py [port]

For security reasons, it only accepts requests from the current host.
This can still be insecure, but restricts violations from people who
can log in on your machine.  Use with caution!

"""

import sys, os, string, getopt, thread, socket, traceback

PORT = 4000                             # Default port

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "")
        if len(args) > 1:
            raise getopt.error("Too many arguments.")
    except getopt.error as msg:
        usage(msg)
    for o, a in opts:
        pass
    if args:
        try:
            port = string.atoi(args[0])
        except ValueError as msg:
            usage(msg)
    else:
        port = PORT
    main_thread(port)

def usage(msg=None):
    sys.stdout = sys.stderr
    if msg:
        print(msg)
    print("\n", __doc__, end=' ')
    sys.exit(2)

def main_thread(port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(("", port))
    sock.listen(5)
    print("Listening on port", port, "...")
    while 1:
        (conn, addr) = sock.accept()
        if addr[0] != conn.getsockname()[0]:
            conn.close()
            print("Refusing connection from non-local host", addr[0], ".")
            continue
        thread.start_new_thread(service_thread, (conn, addr))
        del conn, addr

def service_thread(conn, addr):
    (caddr, cport) = addr
    print("Thread %s has connection from %s.\n" % (str(thread.get_ident()),
                                                   caddr), end=' ')
    stdin = conn.makefile("r")
    stdout = conn.makefile("w", 0)
    run_interpreter(stdin, stdout)
    print("Thread %s is done.\n" % str(thread.get_ident()), end=' ')

def run_interpreter(stdin, stdout):
    globals = {}
    try:
        str(sys.ps1)
    except:
        sys.ps1 = ">>> "
    source = ""
    while 1:
        stdout.write(sys.ps1)
        line = stdin.readline()
        if line[:2] == '\377\354':
            line = ""
        if not line and not source:
            break
        if line[-2:] == '\r\n':
            line = line[:-2] + '\n'
        source = source + line
        try:
            code = compile_command(source)
        except SyntaxError as err:
            source = ""
            traceback.print_exception(SyntaxError, err, None, file=stdout)
            continue
        if not code:
            continue
        source = ""
        try:
            run_command(code, stdin, stdout, globals)
        except SystemExit as how:
            if how:
                try:
                    how = str(how)
                except:
                    how = ""
                stdout.write("Exit %s\n" % how)
            break
    stdout.write("\nGoodbye.\n")

def run_command(code, stdin, stdout, globals):
    save = sys.stdin, sys.stdout, sys.stderr
    try:
        sys.stdout = sys.stderr = stdout
        sys.stdin = stdin
        try:
            exec(code, globals)
        except SystemExit as how:
            raise SystemExit(how).with_traceback(sys.exc_info()[2])
        except:
            type, value, tb = sys.exc_info()
            if tb: tb = tb.tb_next
            traceback.print_exception(type, value, tb)
            del tb
    finally:
        sys.stdin, sys.stdout, sys.stderr = save

from code import compile_command

main()
