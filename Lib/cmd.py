"""A generic class to build line-oriented command interpreters.

Interpreters constructed with this class obey the following conventions:

1. End of file on input is processed as the command 'EOF'.
2. A command is parsed out of each line by collecting the prefix composed
   of characters in the identchars member.
3. A command `foo' is dispatched to a method 'do_foo()'; the do_ method
   is passed a single argument consisting of the remainder of the line.
4. Typing an empty line repeats the last command.  (Actually, it calls the
   method `emptyline', which may be overridden in a subclass.)
5. There is a predefined `help' method.  Given an argument `topic', it
   calls the command `help_topic'.  With no arguments, it lists all topics
   with defined help_ functions, broken into up to three topics; documented
   commands, miscellaneous help topics, and undocumented commands.
6. The command '?' is a synonym for `help'.  The command '!' is a synonym
   for `shell', if a do_shell method exists.

The `default' method may be overridden to intercept commands for which there
is no do_ method.

The data member `self.ruler' sets the character used to draw separator lines
in the help messages.  If empty, no ruler line is drawn.  It defaults to "=".

If the value of `self.intro' is nonempty when the cmdloop method is called,
it is printed out on interpreter startup.  This value may be overridden
via an optional argument to the cmdloop() method.

The data members `self.doc_header', `self.misc_header', and
`self.undoc_header' set the headers used for the help function's
listings of documented functions, miscellaneous topics, and undocumented
functions respectively.

These interpreters use raw_input; thus, if the readline module is loaded,
they automatically support Emacs-like command history and editing features.
"""

import string

PROMPT = '(Cmd) '
IDENTCHARS = string.letters + string.digits + '_'

class Cmd:
    prompt = PROMPT
    identchars = IDENTCHARS
    ruler = '='
    lastcmd = ''
    cmdqueue = []
    intro = None
    doc_leader = ""
    doc_header = "Documented commands (type help <topic>):"
    misc_header = "Miscellaneous help topics:"
    undoc_header = "Undocumented commands:"
    nohelp = "*** No help on %s"

    def __init__(self): pass

    def cmdloop(self, intro=None):
        self.preloop()
        if intro != None:
            self.intro = intro
        if self.intro:
            print self.intro
        stop = None
        while not stop:
            if self.cmdqueue:
                line = self.cmdqueue[0]
                del self.cmdqueue[0]
            else:
                try:
                    line = raw_input(self.prompt)
                except EOFError:
                    line = 'EOF'
            line = self.precmd(line)
            stop = self.onecmd(line)
            stop = self.postcmd(stop, line)
        self.postloop()

    def precmd(self, line):
        return line

    def postcmd(self, stop, line):
        return stop

    def preloop(self):
        pass

    def postloop(self):
        pass

    def onecmd(self, line):
        line = string.strip(line)
        if not line:
            return self.emptyline()
        elif line[0] == '?':
            line = 'help ' + line[1:]
        elif line[0] == '!':
            if hasattr(self, 'do_shell'):
                line = 'shell ' + line[1:]
            else:
                return self.default(line)
        self.lastcmd = line
        i, n = 0, len(line)
        while i < n and line[i] in self.identchars: i = i+1
        cmd, arg = line[:i], string.strip(line[i:])
        if cmd == '':
            return self.default(line)
        else:
            try:
                func = getattr(self, 'do_' + cmd)
            except AttributeError:
                return self.default(line)
            return func(arg)

    def emptyline(self):
        if self.lastcmd:
            return self.onecmd(self.lastcmd)

    def default(self, line):
        print '*** Unknown syntax:', line

    def do_help(self, arg):
        if arg:
            # XXX check arg syntax
            try:
                func = getattr(self, 'help_' + arg)
            except:
                try:
                    doc=getattr(self, 'do_' + arg).__doc__
                    if doc:
                        print doc
                        return
                except:
                    pass
                print self.nohelp % (arg,)
                return
            func()
        else:
            # Inheritance says we have to look in class and
            # base classes; order is not important.
            names = []
            classes = [self.__class__]
            while classes:
                aclass = classes[0]
                if aclass.__bases__:
                    classes = classes + list(aclass.__bases__)
                names = names + dir(aclass)
                del classes[0]
            cmds_doc = []
            cmds_undoc = []
            help = {}
            for name in names:
                if name[:5] == 'help_':
                    help[name[5:]]=1
            names.sort()
            # There can be duplicates if routines overridden
            prevname = ''
            for name in names:
                if name[:3] == 'do_':
                    if name == prevname:
                        continue
                    prevname = name
                    cmd=name[3:]
                    if help.has_key(cmd):
                        cmds_doc.append(cmd)
                        del help[cmd]
                    elif getattr(self, name).__doc__:
                        cmds_doc.append(cmd)
                    else:
                        cmds_undoc.append(cmd)
            print self.doc_leader
            self.print_topics(self.doc_header,   cmds_doc,   15,80)
            self.print_topics(self.misc_header,  help.keys(),15,80)
            self.print_topics(self.undoc_header, cmds_undoc, 15,80)

    def print_topics(self, header, cmds, cmdlen, maxcol):
        if cmds:
            print header;
            if self.ruler:
                print self.ruler * len(header)
            (cmds_per_line,junk)=divmod(maxcol,cmdlen)
            col=cmds_per_line
            for cmd in cmds:
                if col==0: print
                print (("%-"+`cmdlen`+"s") % cmd),
                col = (col+1) % cmds_per_line
            print "\n"
