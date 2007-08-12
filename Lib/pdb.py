#! /usr/bin/env python

"""A Python debugger."""

# (See pdb.doc for documentation.)

import sys
import linecache
import cmd
import bdb
from repr import Repr
import os
import re
import pprint
import traceback


class Restart(Exception):
    """Causes a debugger to be restarted for the debugged python program."""
    pass

# Create a custom safe Repr instance and increase its maxstring.
# The default of 30 truncates error messages too easily.
_repr = Repr()
_repr.maxstring = 200
_saferepr = _repr.repr

__all__ = ["run", "pm", "Pdb", "runeval", "runctx", "runcall", "set_trace",
           "post_mortem", "help"]

def find_function(funcname, filename):
    cre = re.compile(r'def\s+%s\s*[(]' % re.escape(funcname))
    try:
        fp = open(filename)
    except IOError:
        return None
    # consumer of this info expects the first line to be 1
    lineno = 1
    answer = None
    while 1:
        line = fp.readline()
        if line == '':
            break
        if cre.match(line):
            answer = funcname, filename, lineno
            break
        lineno = lineno + 1
    fp.close()
    return answer


# Interaction prompt line will separate file and call info from code
# text using value of line_prefix string.  A newline and arrow may
# be to your liking.  You can set it once pdb is imported using the
# command "pdb.line_prefix = '\n% '".
# line_prefix = ': '    # Use this to get the old situation back
line_prefix = '\n-> '   # Probably a better default

class Pdb(bdb.Bdb, cmd.Cmd):

    def __init__(self, completekey='tab', stdin=None, stdout=None):
        bdb.Bdb.__init__(self)
        cmd.Cmd.__init__(self, completekey, stdin, stdout)
        if stdout:
            self.use_rawinput = 0
        self.prompt = '(Pdb) '
        self.aliases = {}
        self.mainpyfile = ''
        self._wait_for_mainpyfile = 0
        # Try to load readline if it exists
        try:
            import readline
        except ImportError:
            pass

        # Read $HOME/.pdbrc and ./.pdbrc
        self.rcLines = []
        if 'HOME' in os.environ:
            envHome = os.environ['HOME']
            try:
                rcFile = open(os.path.join(envHome, ".pdbrc"))
            except IOError:
                pass
            else:
                for line in rcFile.readlines():
                    self.rcLines.append(line)
                rcFile.close()
        try:
            rcFile = open(".pdbrc")
        except IOError:
            pass
        else:
            for line in rcFile.readlines():
                self.rcLines.append(line)
            rcFile.close()

        self.commands = {} # associates a command list to breakpoint numbers
        self.commands_doprompt = {} # for each bp num, tells if the prompt must be disp. after execing the cmd list
        self.commands_silent = {} # for each bp num, tells if the stack trace must be disp. after execing the cmd list
        self.commands_defining = False # True while in the process of defining a command list
        self.commands_bnum = None # The breakpoint number for which we are defining a list

    def reset(self):
        bdb.Bdb.reset(self)
        self.forget()

    def forget(self):
        self.lineno = None
        self.stack = []
        self.curindex = 0
        self.curframe = None

    def setup(self, f, t):
        self.forget()
        self.stack, self.curindex = self.get_stack(f, t)
        self.curframe = self.stack[self.curindex][0]
        self.execRcLines()

    # Can be executed earlier than 'setup' if desired
    def execRcLines(self):
        if self.rcLines:
            # Make local copy because of recursion
            rcLines = self.rcLines
            # executed only once
            self.rcLines = []
            for line in rcLines:
                line = line[:-1]
                if len(line) > 0 and line[0] != '#':
                    self.onecmd(line)

    # Override Bdb methods

    def user_call(self, frame, argument_list):
        """This method is called when there is the remote possibility
        that we ever need to stop in this function."""
        if self._wait_for_mainpyfile:
            return
        if self.stop_here(frame):
            print('--Call--', file=self.stdout)
            self.interaction(frame, None)

    def user_line(self, frame):
        """This function is called when we stop or break at this line."""
        if self._wait_for_mainpyfile:
            if (self.mainpyfile != self.canonic(frame.f_code.co_filename)
                or frame.f_lineno<= 0):
                return
            self._wait_for_mainpyfile = 0
        if self.bp_commands(frame):
            self.interaction(frame, None)

    def bp_commands(self,frame):
        """ Call every command that was set for the current active breakpoint (if there is one)
        Returns True if the normal interaction function must be called, False otherwise """
        #self.currentbp is set in bdb.py in bdb.break_here if a breakpoint was hit
        if getattr(self,"currentbp",False) and self.currentbp in self.commands:
            currentbp = self.currentbp
            self.currentbp = 0
            lastcmd_back = self.lastcmd
            self.setup(frame, None)
            for line in self.commands[currentbp]:
                self.onecmd(line)
            self.lastcmd = lastcmd_back
            if not self.commands_silent[currentbp]:
                self.print_stack_entry(self.stack[self.curindex])
            if self.commands_doprompt[currentbp]:
                self.cmdloop()
            self.forget()
            return
        return 1

    def user_return(self, frame, return_value):
        """This function is called when a return trap is set here."""
        frame.f_locals['__return__'] = return_value
        print('--Return--', file=self.stdout)
        self.interaction(frame, None)

    def user_exception(self, frame, exc_info):
        """This function is called if an exception occurs,
        but only if we are to stop at or just below this level."""
        exc_type, exc_value, exc_traceback = exc_info
        frame.f_locals['__exception__'] = exc_type, exc_value
        exc_type_name = exc_type.__name__
        print(exc_type_name + ':', _saferepr(exc_value), file=self.stdout)
        self.interaction(frame, exc_traceback)

    # General interaction function

    def interaction(self, frame, traceback):
        self.setup(frame, traceback)
        self.print_stack_entry(self.stack[self.curindex])
        self.cmdloop()
        self.forget()

    def default(self, line):
        if line[:1] == '!': line = line[1:]
        locals = self.curframe.f_locals
        globals = self.curframe.f_globals
        try:
            code = compile(line + '\n', '<stdin>', 'single')
            exec(code, globals, locals)
        except:
            t, v = sys.exc_info()[:2]
            if type(t) == type(''):
                exc_type_name = t
            else: exc_type_name = t.__name__
            print('***', exc_type_name + ':', v, file=self.stdout)

    def precmd(self, line):
        """Handle alias expansion and ';;' separator."""
        if not line.strip():
            return line
        args = line.split()
        while args[0] in self.aliases:
            line = self.aliases[args[0]]
            ii = 1
            for tmpArg in args[1:]:
                line = line.replace("%" + str(ii),
                                      tmpArg)
                ii = ii + 1
            line = line.replace("%*", ' '.join(args[1:]))
            args = line.split()
        # split into ';;' separated commands
        # unless it's an alias command
        if args[0] != 'alias':
            marker = line.find(';;')
            if marker >= 0:
                # queue up everything after marker
                next = line[marker+2:].lstrip()
                self.cmdqueue.append(next)
                line = line[:marker].rstrip()
        return line

    def onecmd(self, line):
        """Interpret the argument as though it had been typed in response
        to the prompt.

        Checks whether this line is typed at the normal prompt or in
        a breakpoint command list definition.
        """
        if not self.commands_defining:
            return cmd.Cmd.onecmd(self, line)
        else:
            return self.handle_command_def(line)

    def handle_command_def(self,line):
        """ Handles one command line during command list definition. """
        cmd, arg, line = self.parseline(line)
        if cmd == 'silent':
            self.commands_silent[self.commands_bnum] = True
            return # continue to handle other cmd def in the cmd list
        elif cmd == 'end':
            self.cmdqueue = []
            return 1 # end of cmd list
        cmdlist = self.commands[self.commands_bnum]
        if (arg):
            cmdlist.append(cmd+' '+arg)
        else:
            cmdlist.append(cmd)
        # Determine if we must stop
        try:
            func = getattr(self, 'do_' + cmd)
        except AttributeError:
            func = self.default
        if func.__name__ in self.commands_resuming : # one of the resuming commands.
            self.commands_doprompt[self.commands_bnum] = False
            self.cmdqueue = []
            return 1
        return

    # Command definitions, called by cmdloop()
    # The argument is the remaining string on the command line
    # Return true to exit from the command loop

    do_h = cmd.Cmd.do_help

    def do_commands(self, arg):
        """Defines a list of commands associated to a breakpoint
        Those commands will be executed whenever the breakpoint causes the program to stop execution."""
        if not arg:
            bnum = len(bdb.Breakpoint.bpbynumber)-1
        else:
            try:
                bnum = int(arg)
            except:
                print("Usage : commands [bnum]\n        ...\n        end", file=self.stdout)
                return
        self.commands_bnum = bnum
        self.commands[bnum] = []
        self.commands_doprompt[bnum] = True
        self.commands_silent[bnum] = False
        prompt_back = self.prompt
        self.prompt = '(com) '
        self.commands_defining = True
        self.cmdloop()
        self.commands_defining = False
        self.prompt = prompt_back

    def do_break(self, arg, temporary = 0):
        # break [ ([filename:]lineno | function) [, "condition"] ]
        if not arg:
            if self.breaks:  # There's at least one
                print("Num Type         Disp Enb   Where", file=self.stdout)
                for bp in bdb.Breakpoint.bpbynumber:
                    if bp:
                        bp.bpprint(self.stdout)
            return
        # parse arguments; comma has lowest precedence
        # and cannot occur in filename
        filename = None
        lineno = None
        cond = None
        comma = arg.find(',')
        if comma > 0:
            # parse stuff after comma: "condition"
            cond = arg[comma+1:].lstrip()
            arg = arg[:comma].rstrip()
        # parse stuff before comma: [filename:]lineno | function
        colon = arg.rfind(':')
        funcname = None
        if colon >= 0:
            filename = arg[:colon].rstrip()
            f = self.lookupmodule(filename)
            if not f:
                print('*** ', repr(filename), end=' ', file=self.stdout)
                print('not found from sys.path', file=self.stdout)
                return
            else:
                filename = f
            arg = arg[colon+1:].lstrip()
            try:
                lineno = int(arg)
            except ValueError as msg:
                print('*** Bad lineno:', arg, file=self.stdout)
                return
        else:
            # no colon; can be lineno or function
            try:
                lineno = int(arg)
            except ValueError:
                try:
                    func = eval(arg,
                                self.curframe.f_globals,
                                self.curframe.f_locals)
                except:
                    func = arg
                try:
                    if hasattr(func, 'im_func'):
                        func = func.im_func
                    code = func.__code__
                    #use co_name to identify the bkpt (function names
                    #could be aliased, but co_name is invariant)
                    funcname = code.co_name
                    lineno = code.co_firstlineno
                    filename = code.co_filename
                except:
                    # last thing to try
                    (ok, filename, ln) = self.lineinfo(arg)
                    if not ok:
                        print('*** The specified object', end=' ', file=self.stdout)
                        print(repr(arg), end=' ', file=self.stdout)
                        print('is not a function', file=self.stdout)
                        print('or was not found along sys.path.', file=self.stdout)
                        return
                    funcname = ok # ok contains a function name
                    lineno = int(ln)
        if not filename:
            filename = self.defaultFile()
        # Check for reasonable breakpoint
        line = self.checkline(filename, lineno)
        if line:
            # now set the break point
            err = self.set_break(filename, line, temporary, cond, funcname)
            if err: print('***', err, file=self.stdout)
            else:
                bp = self.get_breaks(filename, line)[-1]
                print("Breakpoint %d at %s:%d" % (bp.number,
                                                                 bp.file,
                                                                 bp.line), file=self.stdout)

    # To be overridden in derived debuggers
    def defaultFile(self):
        """Produce a reasonable default."""
        filename = self.curframe.f_code.co_filename
        if filename == '<string>' and self.mainpyfile:
            filename = self.mainpyfile
        return filename

    do_b = do_break

    def do_tbreak(self, arg):
        self.do_break(arg, 1)

    def lineinfo(self, identifier):
        failed = (None, None, None)
        # Input is identifier, may be in single quotes
        idstring = identifier.split("'")
        if len(idstring) == 1:
            # not in single quotes
            id = idstring[0].strip()
        elif len(idstring) == 3:
            # quoted
            id = idstring[1].strip()
        else:
            return failed
        if id == '': return failed
        parts = id.split('.')
        # Protection for derived debuggers
        if parts[0] == 'self':
            del parts[0]
            if len(parts) == 0:
                return failed
        # Best first guess at file to look at
        fname = self.defaultFile()
        if len(parts) == 1:
            item = parts[0]
        else:
            # More than one part.
            # First is module, second is method/class
            f = self.lookupmodule(parts[0])
            if f:
                fname = f
            item = parts[1]
        answer = find_function(item, fname)
        return answer or failed

    def checkline(self, filename, lineno):
        """Check whether specified line seems to be executable.

        Return `lineno` if it is, 0 if not (e.g. a docstring, comment, blank
        line or EOF). Warning: testing is not comprehensive.
        """
        line = linecache.getline(filename, lineno)
        if not line:
            print('End of file', file=self.stdout)
            return 0
        line = line.strip()
        # Don't allow setting breakpoint at a blank line
        if (not line or (line[0] == '#') or
             (line[:3] == '"""') or line[:3] == "'''"):
            print('*** Blank or comment', file=self.stdout)
            return 0
        return lineno

    def do_enable(self, arg):
        args = arg.split()
        for i in args:
            try:
                i = int(i)
            except ValueError:
                print('Breakpoint index %r is not a number' % i, file=self.stdout)
                continue

            if not (0 <= i < len(bdb.Breakpoint.bpbynumber)):
                print('No breakpoint numbered', i, file=self.stdout)
                continue

            bp = bdb.Breakpoint.bpbynumber[i]
            if bp:
                bp.enable()

    def do_disable(self, arg):
        args = arg.split()
        for i in args:
            try:
                i = int(i)
            except ValueError:
                print('Breakpoint index %r is not a number' % i, file=self.stdout)
                continue

            if not (0 <= i < len(bdb.Breakpoint.bpbynumber)):
                print('No breakpoint numbered', i, file=self.stdout)
                continue

            bp = bdb.Breakpoint.bpbynumber[i]
            if bp:
                bp.disable()

    def do_condition(self, arg):
        # arg is breakpoint number and condition
        args = arg.split(' ', 1)
        try:
            bpnum = int(args[0].strip())
        except ValueError:
            # something went wrong
            print('Breakpoint index %r is not a number' % args[0], file=self.stdout)
            return
        try:
            cond = args[1]
        except:
            cond = None
        try:
            bp = bdb.Breakpoint.bpbynumber[bpnum]
        except IndexError:
            print >>self.stdout, 'Breakpoint index %r is not valid' % args[0]
            return
        if bp:
            bp.cond = cond
            if not cond:
                print('Breakpoint', bpnum, end=' ', file=self.stdout)
                print('is now unconditional.', file=self.stdout)

    def do_ignore(self,arg):
        """arg is bp number followed by ignore count."""
        args = arg.split()
        try:
            bpnum = int(args[0].strip())
        except ValueError:
            # something went wrong
            print('Breakpoint index %r is not a number' % args[0], file=self.stdout)
            return
        try:
            count = int(args[1].strip())
        except:
            count = 0
        try:
            bp = bdb.Breakpoint.bpbynumber[bpnum]
        except IndexError:
            print >>self.stdout, 'Breakpoint index %r is not valid' % args[0]
            return
        if bp:
            bp.ignore = count
            if count > 0:
                reply = 'Will ignore next '
                if count > 1:
                    reply = reply + '%d crossings' % count
                else:
                    reply = reply + '1 crossing'
                print(reply + ' of breakpoint %d.' % bpnum, file=self.stdout)
            else:
                print('Will stop next time breakpoint', end=' ', file=self.stdout)
                print(bpnum, 'is reached.', file=self.stdout)

    def do_clear(self, arg):
        """Three possibilities, tried in this order:
        clear -> clear all breaks, ask for confirmation
        clear file:lineno -> clear all breaks at file:lineno
        clear bpno bpno ... -> clear breakpoints by number"""
        if not arg:
            try:
                reply = input('Clear all breaks? ')
            except EOFError:
                reply = 'no'
            reply = reply.strip().lower()
            if reply in ('y', 'yes'):
                self.clear_all_breaks()
            return
        if ':' in arg:
            # Make sure it works for "clear C:\foo\bar.py:12"
            i = arg.rfind(':')
            filename = arg[:i]
            arg = arg[i+1:]
            try:
                lineno = int(arg)
            except ValueError:
                err = "Invalid line number (%s)" % arg
            else:
                err = self.clear_break(filename, lineno)
            if err: print('***', err, file=self.stdout)
            return
        numberlist = arg.split()
        for i in numberlist:
            try:
                i = int(i)
            except ValueError:
                print('Breakpoint index %r is not a number' % i, file=self.stdout)
                continue

            if not (0 <= i < len(bdb.Breakpoint.bpbynumber)):
                print('No breakpoint numbered', i, file=self.stdout)
                continue
            err = self.clear_bpbynumber(i)
            if err:
                print('***', err, file=self.stdout)
            else:
                print('Deleted breakpoint', i, file=self.stdout)
    do_cl = do_clear # 'c' is already an abbreviation for 'continue'

    def do_where(self, arg):
        self.print_stack_trace()
    do_w = do_where
    do_bt = do_where

    def do_up(self, arg):
        if self.curindex == 0:
            print('*** Oldest frame', file=self.stdout)
        else:
            self.curindex = self.curindex - 1
            self.curframe = self.stack[self.curindex][0]
            self.print_stack_entry(self.stack[self.curindex])
            self.lineno = None
    do_u = do_up

    def do_down(self, arg):
        if self.curindex + 1 == len(self.stack):
            print('*** Newest frame', file=self.stdout)
        else:
            self.curindex = self.curindex + 1
            self.curframe = self.stack[self.curindex][0]
            self.print_stack_entry(self.stack[self.curindex])
            self.lineno = None
    do_d = do_down

    def do_step(self, arg):
        self.set_step()
        return 1
    do_s = do_step

    def do_next(self, arg):
        self.set_next(self.curframe)
        return 1
    do_n = do_next

    def do_run(self, arg):
        """Restart program by raising an exception to be caught in the main debugger
        loop. If arguments were given, set them in sys.argv."""
        if arg:
            import shlex
            argv0 = sys.argv[0:1]
            sys.argv = shlex.split(arg)
            sys.argv[:0] = argv0
        raise Restart

    do_restart = do_run

    def do_return(self, arg):
        self.set_return(self.curframe)
        return 1
    do_r = do_return

    def do_continue(self, arg):
        self.set_continue()
        return 1
    do_c = do_cont = do_continue

    def do_jump(self, arg):
        if self.curindex + 1 != len(self.stack):
            print("*** You can only jump within the bottom frame", file=self.stdout)
            return
        try:
            arg = int(arg)
        except ValueError:
            print("*** The 'jump' command requires a line number.", file=self.stdout)
        else:
            try:
                # Do the jump, fix up our copy of the stack, and display the
                # new position
                self.curframe.f_lineno = arg
                self.stack[self.curindex] = self.stack[self.curindex][0], arg
                self.print_stack_entry(self.stack[self.curindex])
            except ValueError as e:
                print('*** Jump failed:', e, file=self.stdout)
    do_j = do_jump

    def do_debug(self, arg):
        sys.settrace(None)
        globals = self.curframe.f_globals
        locals = self.curframe.f_locals
        p = Pdb()
        p.prompt = "(%s) " % self.prompt.strip()
        print("ENTERING RECURSIVE DEBUGGER", file=self.stdout)
        sys.call_tracing(p.run, (arg, globals, locals))
        print("LEAVING RECURSIVE DEBUGGER", file=self.stdout)
        sys.settrace(self.trace_dispatch)
        self.lastcmd = p.lastcmd

    def do_quit(self, arg):
        self._user_requested_quit = 1
        self.set_quit()
        return 1

    do_q = do_quit
    do_exit = do_quit

    def do_EOF(self, arg):
        print(file=self.stdout)
        self._user_requested_quit = 1
        self.set_quit()
        return 1

    def do_args(self, arg):
        f = self.curframe
        co = f.f_code
        dict = f.f_locals
        n = co.co_argcount
        if co.co_flags & 4: n = n+1
        if co.co_flags & 8: n = n+1
        for i in range(n):
            name = co.co_varnames[i]
            print(name, '=', end=' ', file=self.stdout)
            if name in dict: print(dict[name], file=self.stdout)
            else: print("*** undefined ***", file=self.stdout)
    do_a = do_args

    def do_retval(self, arg):
        if '__return__' in self.curframe.f_locals:
            print(self.curframe.f_locals['__return__'], file=self.stdout)
        else:
            print('*** Not yet returned!', file=self.stdout)
    do_rv = do_retval

    def _getval(self, arg):
        try:
            return eval(arg, self.curframe.f_globals,
                        self.curframe.f_locals)
        except:
            t, v = sys.exc_info()[:2]
            if isinstance(t, str):
                exc_type_name = t
            else: exc_type_name = t.__name__
            print('***', exc_type_name + ':', repr(v), file=self.stdout)
            raise

    def do_p(self, arg):
        try:
            print(repr(self._getval(arg)), file=self.stdout)
        except:
            pass

    def do_pp(self, arg):
        try:
            pprint.pprint(self._getval(arg), self.stdout)
        except:
            pass

    def do_list(self, arg):
        self.lastcmd = 'list'
        last = None
        if arg:
            try:
                x = eval(arg, {}, {})
                if type(x) == type(()):
                    first, last = x
                    first = int(first)
                    last = int(last)
                    if last < first:
                        # Assume it's a count
                        last = first + last
                else:
                    first = max(1, int(x) - 5)
            except:
                print('*** Error in argument:', repr(arg), file=self.stdout)
                return
        elif self.lineno is None:
            first = max(1, self.curframe.f_lineno - 5)
        else:
            first = self.lineno + 1
        if last is None:
            last = first + 10
        filename = self.curframe.f_code.co_filename
        breaklist = self.get_file_breaks(filename)
        try:
            for lineno in range(first, last+1):
                line = linecache.getline(filename, lineno)
                if not line:
                    print('[EOF]', file=self.stdout)
                    break
                else:
                    s = repr(lineno).rjust(3)
                    if len(s) < 4: s = s + ' '
                    if lineno in breaklist: s = s + 'B'
                    else: s = s + ' '
                    if lineno == self.curframe.f_lineno:
                        s = s + '->'
                    print(s + '\t' + line, end='', file=self.stdout)
                    self.lineno = lineno
        except KeyboardInterrupt:
            pass
    do_l = do_list

    def do_whatis(self, arg):
        try:
            value = eval(arg, self.curframe.f_globals,
                            self.curframe.f_locals)
        except:
            t, v = sys.exc_info()[:2]
            if type(t) == type(''):
                exc_type_name = t
            else: exc_type_name = t.__name__
            print('***', exc_type_name + ':', repr(v), file=self.stdout)
            return
        code = None
        # Is it a function?
        try: code = value.__code__
        except: pass
        if code:
            print('Function', code.co_name, file=self.stdout)
            return
        # Is it an instance method?
        try: code = value.im_func.__code__
        except: pass
        if code:
            print('Method', code.co_name, file=self.stdout)
            return
        # None of the above...
        print(type(value), file=self.stdout)

    def do_alias(self, arg):
        args = arg.split()
        if len(args) == 0:
            keys = self.aliases.keys()
            keys.sort()
            for alias in keys:
                print("%s = %s" % (alias, self.aliases[alias]), file=self.stdout)
            return
        if args[0] in self.aliases and len(args) == 1:
            print("%s = %s" % (args[0], self.aliases[args[0]]), file=self.stdout)
        else:
            self.aliases[args[0]] = ' '.join(args[1:])

    def do_unalias(self, arg):
        args = arg.split()
        if len(args) == 0: return
        if args[0] in self.aliases:
            del self.aliases[args[0]]

    #list of all the commands making the program resume execution.
    commands_resuming = ['do_continue', 'do_step', 'do_next', 'do_return',
                         'do_quit', 'do_jump']

    # Print a traceback starting at the top stack frame.
    # The most recently entered frame is printed last;
    # this is different from dbx and gdb, but consistent with
    # the Python interpreter's stack trace.
    # It is also consistent with the up/down commands (which are
    # compatible with dbx and gdb: up moves towards 'main()'
    # and down moves towards the most recent stack frame).

    def print_stack_trace(self):
        try:
            for frame_lineno in self.stack:
                self.print_stack_entry(frame_lineno)
        except KeyboardInterrupt:
            pass

    def print_stack_entry(self, frame_lineno, prompt_prefix=line_prefix):
        frame, lineno = frame_lineno
        if frame is self.curframe:
            print('>', end=' ', file=self.stdout)
        else:
            print(' ', end=' ', file=self.stdout)
        print(self.format_stack_entry(frame_lineno,
                                                     prompt_prefix), file=self.stdout)


    # Help methods (derived from pdb.doc)

    def help_help(self):
        self.help_h()

    def help_h(self):
        print("""h(elp)
Without argument, print the list of available commands.
With a command name as argument, print help about that command
"help pdb" pipes the full documentation file to the $PAGER
"help exec" gives help on the ! command""", file=self.stdout)

    def help_where(self):
        self.help_w()

    def help_w(self):
        print("""w(here)
Print a stack trace, with the most recent frame at the bottom.
An arrow indicates the "current frame", which determines the
context of most commands.  'bt' is an alias for this command.""", file=self.stdout)

    help_bt = help_w

    def help_down(self):
        self.help_d()

    def help_d(self):
        print("""d(own)
Move the current frame one level down in the stack trace
(to a newer frame).""", file=self.stdout)

    def help_up(self):
        self.help_u()

    def help_u(self):
        print("""u(p)
Move the current frame one level up in the stack trace
(to an older frame).""", file=self.stdout)

    def help_break(self):
        self.help_b()

    def help_b(self):
        print("""b(reak) ([file:]lineno | function) [, condition]
With a line number argument, set a break there in the current
file.  With a function name, set a break at first executable line
of that function.  Without argument, list all breaks.  If a second
argument is present, it is a string specifying an expression
which must evaluate to true before the breakpoint is honored.

The line number may be prefixed with a filename and a colon,
to specify a breakpoint in another file (probably one that
hasn't been loaded yet).  The file is searched for on sys.path;
the .py suffix may be omitted.""", file=self.stdout)

    def help_clear(self):
        self.help_cl()

    def help_cl(self):
        print("cl(ear) filename:lineno", file=self.stdout)
        print("""cl(ear) [bpnumber [bpnumber...]]
With a space separated list of breakpoint numbers, clear
those breakpoints.  Without argument, clear all breaks (but
first ask confirmation).  With a filename:lineno argument,
clear all breaks at that line in that file.

Note that the argument is different from previous versions of
the debugger (in python distributions 1.5.1 and before) where
a linenumber was used instead of either filename:lineno or
breakpoint numbers.""", file=self.stdout)

    def help_tbreak(self):
        print("""tbreak  same arguments as break, but breakpoint is
removed when first hit.""", file=self.stdout)

    def help_enable(self):
        print("""enable bpnumber [bpnumber ...]
Enables the breakpoints given as a space separated list of
bp numbers.""", file=self.stdout)

    def help_disable(self):
        print("""disable bpnumber [bpnumber ...]
Disables the breakpoints given as a space separated list of
bp numbers.""", file=self.stdout)

    def help_ignore(self):
        print("""ignore bpnumber count
Sets the ignore count for the given breakpoint number.  A breakpoint
becomes active when the ignore count is zero.  When non-zero, the
count is decremented each time the breakpoint is reached and the
breakpoint is not disabled and any associated condition evaluates
to true.""", file=self.stdout)

    def help_condition(self):
        print("""condition bpnumber str_condition
str_condition is a string specifying an expression which
must evaluate to true before the breakpoint is honored.
If str_condition is absent, any existing condition is removed;
i.e., the breakpoint is made unconditional.""", file=self.stdout)

    def help_step(self):
        self.help_s()

    def help_s(self):
        print("""s(tep)
Execute the current line, stop at the first possible occasion
(either in a function that is called or in the current function).""", file=self.stdout)

    def help_next(self):
        self.help_n()

    def help_n(self):
        print("""n(ext)
Continue execution until the next line in the current function
is reached or it returns.""", file=self.stdout)

    def help_return(self):
        self.help_r()

    def help_r(self):
        print("""r(eturn)
Continue execution until the current function returns.""", file=self.stdout)

    def help_continue(self):
        self.help_c()

    def help_cont(self):
        self.help_c()

    def help_c(self):
        print("""c(ont(inue))
Continue execution, only stop when a breakpoint is encountered.""", file=self.stdout)

    def help_jump(self):
        self.help_j()

    def help_j(self):
        print("""j(ump) lineno
Set the next line that will be executed.""", file=self.stdout)

    def help_debug(self):
        print("""debug code
Enter a recursive debugger that steps through the code argument
(which is an arbitrary expression or statement to be executed
in the current environment).""", file=self.stdout)

    def help_list(self):
        self.help_l()

    def help_l(self):
        print("""l(ist) [first [,last]]
List source code for the current file.
Without arguments, list 11 lines around the current line
or continue the previous listing.
With one argument, list 11 lines starting at that line.
With two arguments, list the given range;
if the second argument is less than the first, it is a count.""", file=self.stdout)

    def help_args(self):
        self.help_a()

    def help_a(self):
        print("""a(rgs)
Print the arguments of the current function.""", file=self.stdout)

    def help_p(self):
        print("""p expression
Print the value of the expression.""", file=self.stdout)

    def help_pp(self):
        print("""pp expression
Pretty-print the value of the expression.""", file=self.stdout)

    def help_exec(self):
        print("""(!) statement
Execute the (one-line) statement in the context of
the current stack frame.
The exclamation point can be omitted unless the first word
of the statement resembles a debugger command.
To assign to a global variable you must always prefix the
command with a 'global' command, e.g.:
(Pdb) global list_options; list_options = ['-l']
(Pdb)""", file=self.stdout)

    def help_run(self):
        print("""run [args...]
Restart the debugged python program. If a string is supplied, it is
splitted with "shlex" and the result is used as the new sys.argv.
History, breakpoints, actions and debugger options are preserved.
"restart" is an alias for "run".""")

    help_restart = help_run

    def help_quit(self):
        self.help_q()

    def help_q(self):
        print("""q(uit) or exit - Quit from the debugger.
The program being executed is aborted.""", file=self.stdout)

    help_exit = help_q

    def help_whatis(self):
        print("""whatis arg
Prints the type of the argument.""", file=self.stdout)

    def help_EOF(self):
        print("""EOF
Handles the receipt of EOF as a command.""", file=self.stdout)

    def help_alias(self):
        print("""alias [name [command [parameter parameter ...] ]]
Creates an alias called 'name' the executes 'command'.  The command
must *not* be enclosed in quotes.  Replaceable parameters are
indicated by %1, %2, and so on, while %* is replaced by all the
parameters.  If no command is given, the current alias for name
is shown. If no name is given, all aliases are listed.

Aliases may be nested and can contain anything that can be
legally typed at the pdb prompt.  Note!  You *can* override
internal pdb commands with aliases!  Those internal commands
are then hidden until the alias is removed.  Aliasing is recursively
applied to the first word of the command line; all other words
in the line are left alone.

Some useful aliases (especially when placed in the .pdbrc file) are:

#Print instance variables (usage "pi classInst")
alias pi for k in %1.__dict__.keys(): print "%1.",k,"=",%1.__dict__[k]

#Print instance variables in self
alias ps pi self
""", file=self.stdout)

    def help_unalias(self):
        print("""unalias name
Deletes the specified alias.""", file=self.stdout)

    def help_commands(self):
        print("""commands [bpnumber]
(com) ...
(com) end
(Pdb)

Specify a list of commands for breakpoint number bpnumber.  The
commands themselves appear on the following lines.  Type a line
containing just 'end' to terminate the commands.

To remove all commands from a breakpoint, type commands and
follow it immediately with  end; that is, give no commands.

With no bpnumber argument, commands refers to the last
breakpoint set.

You can use breakpoint commands to start your program up again.
Simply use the continue command, or step, or any other
command that resumes execution.

Specifying any command resuming execution (currently continue,
step, next, return, jump, quit and their abbreviations) terminates
the command list (as if that command was immediately followed by end).
This is because any time you resume execution
(even with a simple next or step), you may encounter
another breakpoint--which could have its own command list, leading to
ambiguities about which list to execute.

   If you use the 'silent' command in the command list, the
usual message about stopping at a breakpoint is not printed.  This may
be desirable for breakpoints that are to print a specific message and
then continue.  If none of the other commands print anything, you
see no sign that the breakpoint was reached.
""", file=self.stdout)

    def help_pdb(self):
        help()

    def lookupmodule(self, filename):
        """Helper function for break/clear parsing -- may be overridden.

        lookupmodule() translates (possibly incomplete) file or module name
        into an absolute file name.
        """
        if os.path.isabs(filename) and  os.path.exists(filename):
            return filename
        f = os.path.join(sys.path[0], filename)
        if  os.path.exists(f) and self.canonic(f) == self.mainpyfile:
            return f
        root, ext = os.path.splitext(filename)
        if ext == '':
            filename = filename + '.py'
        if os.path.isabs(filename):
            return filename
        for dirname in sys.path:
            while os.path.islink(dirname):
                dirname = os.readlink(dirname)
            fullname = os.path.join(dirname, filename)
            if os.path.exists(fullname):
                return fullname
        return None

    def _runscript(self, filename):
        # The script has to run in __main__ namespace (or imports from
        # __main__ will break).
        #
        # So we clear up the __main__ and set several special variables
        # (this gets rid of pdb's globals and cleans old variables on restarts).
        import __main__
        __main__.__dict__.clear()
        __main__.__dict__.update({"__name__"    : "__main__",
                                  "__file__"    : filename,
                                  "__builtins__": __builtins__,
                                 })

        # When bdb sets tracing, a number of call and line events happens
        # BEFORE debugger even reaches user's code (and the exact sequence of
        # events depends on python version). So we take special measures to
        # avoid stopping before we reach the main script (see user_line and
        # user_call for details).
        self._wait_for_mainpyfile = 1
        self.mainpyfile = self.canonic(filename)
        self._user_requested_quit = 0
        fp = open(filename)
        try:
            script = fp.read()
        finally:
            fp.close()
        statement = 'exec("%s")' % script
        self.run(statement)

# Simplified interface

def run(statement, globals=None, locals=None):
    Pdb().run(statement, globals, locals)

def runeval(expression, globals=None, locals=None):
    return Pdb().runeval(expression, globals, locals)

def runctx(statement, globals, locals):
    # B/W compatibility
    run(statement, globals, locals)

def runcall(*args, **kwds):
    return Pdb().runcall(*args, **kwds)

def set_trace():
    Pdb().set_trace(sys._getframe().f_back)

# Post-Mortem interface

def post_mortem(t):
    p = Pdb()
    p.reset()
    while t.tb_next is not None:
        t = t.tb_next
    p.interaction(t.tb_frame, t)

def pm():
    post_mortem(sys.last_traceback)


# Main program for testing

TESTCMD = 'import x; x.main()'

def test():
    run(TESTCMD)

# print help
def help():
    for dirname in sys.path:
        fullname = os.path.join(dirname, 'pdb.doc')
        if os.path.exists(fullname):
            sts = os.system('${PAGER-more} '+fullname)
            if sts: print('*** Pager exit status:', sts)
            break
    else:
        print('Sorry, can\'t find the help file "pdb.doc"', end=' ')
        print('along the Python search path')

def main():
    if not sys.argv[1:]:
        print("usage: pdb.py scriptfile [arg] ...")
        sys.exit(2)

    mainpyfile =  sys.argv[1]     # Get script filename
    if not os.path.exists(mainpyfile):
        print('Error:', mainpyfile, 'does not exist')
        sys.exit(1)

    del sys.argv[0]         # Hide "pdb.py" from argument list

    # Replace pdb's dir with script's dir in front of module search path.
    sys.path[0] = os.path.dirname(mainpyfile)

    # Note on saving/restoring sys.argv: it's a good idea when sys.argv was
    # modified by the script being debugged. It's a bad idea when it was
    # changed by the user from the command line. There is a "restart" command which
    # allows explicit specification of command line arguments.
    pdb = Pdb()
    while 1:
        try:
            pdb._runscript(mainpyfile)
            if pdb._user_requested_quit:
                break
            print("The program finished and will be restarted")
        except Restart:
            print("Restarting", mainpyfile, "with arguments:")
            print("\t" + " ".join(sys.argv[1:]))
        except SystemExit:
            # In most cases SystemExit does not warrant a post-mortem session.
            print("The program exited via sys.exit(). Exit status: ", end=' ')
            print(sys.exc_info()[1])
        except:
            traceback.print_exc()
            print("Uncaught exception. Entering post mortem debugging")
            print("Running 'cont' or 'step' will restart the program")
            t = sys.exc_info()[2]
            while t.tb_next is not None:
                t = t.tb_next
            pdb.interaction(t.tb_frame,t)
            print("Post mortem debugger finished. The "+mainpyfile+" will be restarted")


# When invoked as main program, invoke the debugger on a script
if __name__ == '__main__':
    import pdb
    pdb.main()
