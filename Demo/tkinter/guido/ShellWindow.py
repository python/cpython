import os
import sys
import string
from Tkinter import *
from ScrolledText import ScrolledText
from Dialog import Dialog
import signal

BUFSIZE = 512

class ShellWindow(ScrolledText):

    def __init__(self, master=None, shell=None, **cnf):
        if not shell:
            try:
                shell = os.environ['SHELL']
            except KeyError:
                shell = '/bin/sh'
            shell = shell + ' -i'
        args = string.split(shell)
        shell = args[0]

        ScrolledText.__init__(self, master, **cnf)
        self.pos = '1.0'
        self.bind('<Return>', self.inputhandler)
        self.bind('<Control-c>', self.sigint)
        self.bind('<Control-t>', self.sigterm)
        self.bind('<Control-k>', self.sigkill)
        self.bind('<Control-d>', self.sendeof)

        self.pid, self.fromchild, self.tochild = spawn(shell, args)
        self.tk.createfilehandler(self.fromchild, READABLE,
                                  self.outputhandler)

    def outputhandler(self, file, mask):
        data = os.read(file, BUFSIZE)
        if not data:
            self.tk.deletefilehandler(file)
            pid, sts = os.waitpid(self.pid, 0)
            print('pid', pid, 'status', sts)
            self.pid = None
            detail = sts>>8
            cause = sts & 0xff
            if cause == 0:
                msg = "exit status %d" % detail
            else:
                msg = "killed by signal %d" % (cause & 0x7f)
                if cause & 0x80:
                    msg = msg + " -- core dumped"
            Dialog(self.master,
                   text=msg,
                   title="Exit status",
                   bitmap='warning',
                   default=0,
                   strings=('OK',))
            return
        self.insert(END, data)
        self.pos = self.index("end - 1 char")
        self.yview_pickplace(END)

    def inputhandler(self, *args):
        if not self.pid:
            self.no_process()
            return "break"
        self.insert(END, "\n")
        line = self.get(self.pos, "end - 1 char")
        self.pos = self.index(END)
        os.write(self.tochild, line)
        return "break"

    def sendeof(self, *args):
        if not self.pid:
            self.no_process()
            return "break"
        os.close(self.tochild)
        return "break"

    def sendsig(self, sig):
        if not self.pid:
            self.no_process()
            return "break"
        os.kill(self.pid, sig)
        return "break"

    def sigint(self, *args):
        return self.sendsig(signal.SIGINT)

    def sigquit(self, *args):
        return self.sendsig(signal.SIGQUIT)

    def sigterm(self, *args):
        return self.sendsig(signal.SIGTERM)

    def sigkill(self, *args):
        return self.sendsig(signal.SIGKILL)

    def no_process(self):
        Dialog(self.master,
               text="No active process",
               title="No process",
               bitmap='error',
               default=0,
               strings=('OK',))

MAXFD = 100     # Max number of file descriptors (os.getdtablesize()???)

def spawn(prog, args):
    p2cread, p2cwrite = os.pipe()
    c2pread, c2pwrite = os.pipe()
    pid = os.fork()
    if pid == 0:
        # Child
        for i in 0, 1, 2:
            try:
                os.close(i)
            except os.error:
                pass
        if os.dup(p2cread) != 0:
            sys.stderr.write('popen2: bad read dup\n')
        if os.dup(c2pwrite) != 1:
            sys.stderr.write('popen2: bad write dup\n')
        if os.dup(c2pwrite) != 2:
            sys.stderr.write('popen2: bad write dup\n')
        os.closerange(3, MAXFD)
        try:
            os.execvp(prog, args)
        finally:
            sys.stderr.write('execvp failed\n')
            os._exit(1)
    os.close(p2cread)
    os.close(c2pwrite)
    return pid, c2pread, p2cwrite

def test():
    shell = string.join(sys.argv[1:])
    root = Tk()
    root.minsize(1, 1)
    if shell:
        w = ShellWindow(root, shell=shell)
    else:
        w = ShellWindow(root)
    w.pack(expand=1, fill=BOTH)
    w.focus_set()
    w.tk.mainloop()

if __name__ == '__main__':
    test()
