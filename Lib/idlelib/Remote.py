"""Remote
     This module is imported by the loader and serves to control
     the execution of the user program.  It presently executes files
     and reports exceptions to IDLE.  It could be extended to provide
     other services, such as interactive mode and debugging.  To that
     end, it could be a subclass of e.g. InteractiveInterpreter.

     Two other classes, pseudoIn and pseudoOut, are file emulators also
     used by loader.
"""
import sys, os
import traceback

class Remote:    
    def __init__(self, main, master):
        self.main = main
        self.master = master
        self.this_file = self.canonic( self.__init__.im_func.func_code.co_filename )

    def canonic(self, path):
        return os.path.normcase(os.path.abspath(path))

    def mainloop(self):
        while 1:
            args = self.master.get_command()

            try:
                f = getattr(self,args[0])
                apply(f,args[1:])
            except:
                if not self.report_exception(): raise

    def finish(self):
        sys.exit()

    def run(self, *argv):
        sys.argv = argv

        path = self.canonic( argv[0] )
        dir = self.dir = os.path.dirname(path)
        os.chdir(dir)

        sys.path[0] = dir

        usercode = open(path)
        exec usercode in self.main

    def report_exception(self):
        try:
            type, value, tb = sys.exc_info()
            sys.last_type = type
            sys.last_value = value
            sys.last_traceback = tb

            tblist = traceback.extract_tb(tb)

            # Look through the traceback, canonicalizing filenames and
            #   eliminating leading and trailing system modules.
            first = last = 1
            for i in range(len(tblist)):
                filename, lineno, name, line = tblist[i]
                filename = self.canonic(filename)
                tblist[i] = filename, lineno, name, line

                dir = os.path.dirname(filename)
                if filename == self.this_file:
                    first = i+1
                elif dir==self.dir:
                    last = i+1

            # Canonicalize the filename in a syntax error, too:
            if type is SyntaxError:
                try:
                    msg, (filename, lineno, offset, line) = value
                    filename = self.canonic(filename)
                    value = msg, (filename, lineno, offset, line)
                except:
                    pass

            return self.master.program_exception( type, value, tblist, first, last )
        finally:
            # avoid any circular reference through the traceback
            del tb

class pseudoIn:
    def __init__(self, readline):
        self.readline = readline
    def isatty():
        return 1

class pseudoOut:
    def __init__(self, func, **kw):
        self.func = func
        self.kw = kw
    def write(self, *args):
        return apply( self.func, args, self.kw )
    def writelines(self, l):
        map(self.write, l)
    def flush(self):
        pass

