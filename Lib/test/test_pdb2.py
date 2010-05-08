#!/usr/bin/env python
# pdb tests in the Lib/test style
import os
import sys
import time
import re
import subprocess
import signal
from test.test_support import   TESTFN
import test.test_support
import unittest

# allow alt pdb locations, if environment variable is specified then
# the test files will be stored in t/ directory and will not be deleted
# after pdb run
DEBUG_PDB = os.environ.get("_DEBUG_PDB", None)
TMP_DIR = "./t" # dir for tmp files if DEBUG_PDB is set

if DEBUG_PDB:
    if not os.path.exists(TMP_DIR):
        os.mkdir(TMP_DIR)

def _write_test_file(testname, text):
    filename = TESTFN
    if DEBUG_PDB:
        filename = os.path.join(TMP_DIR, testname)
    with open(filename, "wt") as f:
        f.write(text+"\n")
    return filename


class PdbProcess(object):
    def __init__(self, testname, testprg):
        self.testname = testname
        self.filename = _write_test_file(testname, testprg)
        # unbuffer pdb.py output (if it gets any ideas to buffer it)
        # make sure that we use the same interpreter to run tests wrapper and
        # pdb itself
        cmd = [sys.executable, '-u']
        if DEBUG_PDB:
            cmd.append(DEBUG_PDB)
        else:
            cmd.extend(['-m', 'pdb'])
        cmd.append(self.filename)
        self.pdbhandle = subprocess.Popen(cmd, bufsize=0,
                                          stdin=subprocess.PIPE,
                                          stdout=subprocess.PIPE,
                                          stderr=subprocess.STDOUT)
        self.startup_msg = self.wait_for_prompt()
        self.finished=0


    def wait_for_normal_exit(self, timeout=2):
        """wait for pdb subprocess to exit, timeout is in seconds"""
        step = 0.1
        for i in range(int(timeout/step)):
            status=self.pdbhandle.poll()
            if status is not None:
                break
            time.sleep(step)
        if status is -1:
            describe = "pdb has not exited"
        elif status> 0:
            describe = "pdb exited abnormally with status=%d" % status
        assert status == 0, describe

    def wait_for_line(self, stopline, alt_stopline=None):
        output=''
        line=''
        while 1 :
            ch=self.pdbhandle.stdout.read(1)
            # sys.stdout.write(ch)
            line += ch
            if line == stopline or line == alt_stopline:
                return output
            if ch == '\n':
                output += line
                line=''
            if ch == '':#eof
                output += line
                return output

    # note: this can block if issued at the wrong time
    def wait_for_prompt(self):
        """collect any output from pdb session til the prompt is encountered.
        Return this output (exlcuding prompt)"""
        return self.wait_for_line("(Pdb) ", "(com) ")

    def send_cmd(self, cmd):
        """send a command but do not wait for response"""
        #print "sending:", cmd
        self.pdbhandle.stdin.write(cmd+"\n")


    def cmd(self, cmd, response_text=""):
        """send a single command to pdb, collect pdb response (by waiting
        for the next prompt). Verify that response contains specified
        response_text"""

        self.pdbhandle.stdin.write(cmd+"\n")
        response =  self.wait_for_prompt()
        if not response_text:
            return response

        if DEBUG_PDB:
            print "%s: testing response for '%s':" % (self.testname,cmd),
        assert   response.find(response_text) >= 0, (
            "response:\n%s\n does not contain expected substring '%s'" %
            (response, response_text))
        if DEBUG_PDB:
            print "Ok"
        return response

    def send_kbdint(self):
        # os.kill is Posix-specific.  We could have used a X-platform
        # send_signal method of Popen objects, but it still cann't send
        # SIGINT on win32 and it's not present on python2.5
        # self.pdbhandle.send_signal(signal.SIGINT)
        os.kill(self.pdbhandle.pid, signal.SIGINT)

    def __del__(self):
        # if pdb is still running, kill it, leaving it running does not serve
        # any useful purpose
        if self.pdbhandle.poll() is None:
            self.pdbhandle.send_signal(signal.SIGTERM)
        if not DEBUG_PDB:
            os.unlink(self.filename)
        return self.pdbhandle.wait()


class PdbTest(unittest.TestCase):

    def test_00startup(self):
        pdb = PdbProcess("pdb_t_startup", "print 'Hello, world'")
        pdb.cmd("r", "Hello, world")
        pdb.cmd("q")
        pdb.wait_for_normal_exit()

    @unittest.skipIf(sys.platform.startswith("win"),
                     "test_sigint requires a posix system.")
    def test_sigint(self):
        pdb = PdbProcess("pdb_t_loop", """\
for i in xrange(100000000):
     print 'i=%d' %i
"""     )
        # first, test Ctrl-C/kbdint handling while the program is running
        # kbdint should interrupt the program and return to pdb prompt,
        # the program must be resumable
        pdb.send_cmd("c")
        # we could use time.sleep() delays but they are not reliable so you
        # end up with making them much longer than necessary (and still failing
        # from time to time)
        pdb.wait_for_line("i=19")
        pdb.send_kbdint()
        pdb.wait_for_prompt()
        response = pdb.cmd('p "i=%d" % i')
        m = re.search('i=(\d+)', response)
        assert m, "unexpected response %s" % response
        i0 = int(m.group(1))
        pdb.send_cmd("c")
        pdb.wait_for_line("i=%d" % (i0+99))
        pdb.send_kbdint()
        pdb.wait_for_prompt()
        response = pdb.cmd('p "i=%d" % i')
        m = re.search('i=(\d+)', response)
        assert m, "unexpected response %s" % response
        i1 = int(m.group(1))
        assert i1 > i0
        # now test kbd interrupts in interactive mode, they should interrupt
        # the current cmd
        # simple case: just generate kdbint
        pdb.send_kbdint()
        pdb.wait_for_prompt()
        pdb.cmd("p 'hello'", "hello") # check that we are at prompt
        # more complicated case: Ctrl-C while defining bp commands
        # interrupted commands should have no effect
        pdb.cmd("b 2")
        pdb.cmd("commands 1")
        pdb.cmd("p 'marker'")
        pdb.send_kbdint()
        pdb.wait_for_prompt()
        pdb.cmd("p 'hello'", "hello")  # check that we are back at normal prompt
        pdb.send_cmd("c")
        response = pdb.wait_for_prompt()
        assert not re.search("marker", response, re.I), (
            "unexpected response '%s'" % response)
        pdb.cmd("p 'hello'", "hello") #check that we are back  at prompt
        pdb.send_cmd("q")
        pdb.wait_for_normal_exit()


def test_main():
    test.test_support.run_unittest(PdbTest)


if __name__ == "__main__":
    test_main()
