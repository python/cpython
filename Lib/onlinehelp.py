"""
Online help module. 

This module is experimental and could be removed or radically changed
at any time.

It is intended specifically for the standard interpreter command 
line but is intended to be compatible with and useful for other
(e.g. GUI, handheld) environments. Help with those other environments is
appreciated.

Please remember to set PYTHONDOCS to the location of your HTML files:

e.g. 
set PYTHONDOCS=c:\python\docs
PYTHONDOCS=/python/docs

The docs directory should have a lib subdirectory with "index.html" in it.
If it has *.tex then you have the documentation *source* distribution, not
the runtime distribution.

The module exposes one object: "help". "help" has a repr that does something
useful if you just type:

>>> from onlinehelp import help
>>> help

Of course one day the first line will be done automatically by site.py or
something like that. help can be used as a function.

The function takes the following forms of input:

help( "string" ) -- built-in topic or global
help( <ob> ) -- docstring from object or type
help( "doc:filename" ) -- filename from Python documentation

Type help to get the rest of the instructions.
"""
import htmllib # todo: add really basic tr/td support
import formatter
import os, sys
import re

prompt="--more-- (enter for more, q to quit) "

topics={}   # all built-in (non-HTML, non-docstring) topics go in here
commands="" # only used at the top level

def topLevelCommand( name, description, text ):
    """ this function is just for use at the top-level to make sure that
        every advertised top-level topic has a description and every 
        description has text. Maybe we can generalize it later."""

    global commands
    topics[name]=text

    if description[0]=="[":
        placeholder="(dummy)"
    elif description[0]=="<":
        placeholder="link"
    else:
        placeholder=""
    commands=commands+'help( "%s" ) %s - %s\n' % \
                                (name, placeholder, description )

topLevelCommand( 
"intro", 
"What is Python? Read this first!",
"""Welcome to Python, the easy to learn, portable, object oriented 
programming language.

[info on how to use help]

[info on intepreter]"""
)

topLevelCommand( 
"keywords", 
"What are the keywords?", 
"")

topLevelCommand( 
"syntax", 
"What is the overall syntax?", 
"[placeholder]")

topLevelCommand( 
"operators", 
"What operators are available?", 
"<doc:ref/operators.html>" )

topLevelCommand( 
"builtins", 
"What functions, types, etc. are built-in?", 
"<doc:lib/built-in-funcs.html>")

topLevelCommand( "modules", 
"What modules are in the standard library?", 
"<doc:lib/lib.html>")

topLevelCommand( 
"copyright", 
"Who owns Python?", 
"[who knows]")

topLevelCommand( 
"moreinfo", 
"Where is there more information?", 
"[placeholder]")

topLevelCommand( 
"changes", 
"What changed in Python 2.0?",
"[placeholder]"
)

topLevelCommand( 
"extensions", 
"What extensions are installed?",
"[placeholder]")

topLevelCommand( 
"faq", 
"What questions are frequently asked?", 
"[placeholder]")

topLevelCommand( 
"ack", 
"Who has done work on Python lately?", 
"[placeholder for list of people who contributed patches]")


topics[ "prompt" ]="""<doc:tut/node4.html>"""
topics[ "types" ]="""<doc:ref/types.html>"""
topics["everything"]= \
"""<pre>The help function allows you to read help on Python's various 
functions, objects, instructions and modules. You have two options:

1. Use help( obj ) to browse the help attached to some function, module
class or other object. e.g. help( dir )

2. Use help( "somestring" ) to browse help on one of the predefined 
help topics, unassociated with any particular object:

%s</pre>""" % commands

topics[ "keywords" ]=\
"""<pre>"if"       - Conditional execution
"while"    - Loop while a condition is true
"for"      - Loop over a sequence of values (often numbers)
"try"      - Set up an exception handler
"def"      - Define a named function
"class"    - Define a class
"assert"   - Check that some code is working as you expect it to.
"pass"     - Do nothing
"del"      - Delete a data value
"print"    - Print a value
"return"   - Return information from a function
"raise"    - Raise an exception
"break"    - Terminate a loop
"continue" - Skip to the next loop statement
"import"   - Import a module
"global"   - Declare a variable global
"exec"     - Execute some dynamically generated code
"lambda"   - Define an unnamed function

For more information, type e.g. help("assert")</pre>"""

topics[ "if" ]="""<doc:ref/if.html>"""
topics[ "while" ]="""<doc:ref/while.html>"""
topics[ "for" ]="""<doc:ref/for.html>"""
topics[ "try" ]="""<doc:ref/try.html>"""
topics[ "def" ]="""<doc:ref/def.html>"""
topics[ "class" ]="""<doc:ref/class.html>"""
topics[ "assert" ]="""<doc:ref/assert.html>"""
topics[ "pass" ]="""<doc:ref/pass.html>"""
topics[ "del" ]="""<doc:ref/del.html>"""
topics[ "print" ]="""<doc:ref/print.html>"""
topics[ "return" ]="""<doc:ref/return.html>"""
topics[ "raise" ]="""<doc:ref/raise.html>"""
topics[ "break" ]="""<doc:ref/break.html>"""
topics[ "continue" ]="""<doc:ref/continue.html>"""
topics[ "import" ]="""<doc:ref/import.html>"""
topics[ "global" ]="""<doc:ref/global.html>"""
topics[ "exec" ]="""<doc:ref/exec.html>"""
topics[ "lambda" ]="""<doc:ref/lambda.html>"""

envir_var="PYTHONDOCS"

class Help:
    def __init__( self, out, line_length, docdir=None ):
        self.out=out
        self.line_length=line_length
        self.Parser=htmllib.HTMLParser
        self.Formatter=formatter.AbstractFormatter
        self.Pager=Pager
        self.Writer=formatter.DumbWriter
        if os.environ.has_key(envir_var):
            self.docdir=os.environ[envir_var]
        else:
            if os.environ.has_key("PYTHONHOME"):
                pyhome=os.environ["PYTHONHOME"]
            else:
                pyhome=os.path.split( sys.executable )[0]
            self.docdir=os.path.join( pyhome, "doc" )

        testfile=os.path.join( 
                os.path.join( self.docdir, "lib" ), "index.html")

        if not os.path.exists( testfile ):
            error = \
"""Cannot find documentation directory %s. 
Set the %s environment variable to point to a "doc" directory.
It should have a subdirectory "Lib" with a file named "index.html".
""" % (self.docdir, envir_var )
            raise EnvironmentError, error

    def __repr__( self ):
        self( "everything" )
        return ""

    def __call__( self, ob, out=None ):
        try:
            self.call( ob, out )
            return 1
        except (KeyboardInterrupt, EOFError):
            return 0

    def call( self, ob, out ):
        self.pager=out or self.Pager( self.out, self.line_length )

        if type( ob ) in (type(""),type(u"")):
            if ob.startswith( "<" ):
                ob=ob[1:]
            if ob.endswith( ">" ):
                ob=ob[:-1]

            self.write( 'Topic: help( "%s" )\n' % ob )

            if ob.startswith("doc:"):
                path=ob[4:]
                fullpath=os.path.join( self.docdir, path )
                data=open( fullpath ).read()
                index=ob.rfind( "/" )
                self.writeHTML( ob[:index], data )
            else:
                try:
                    info=topics[ob]
                    docrlmatch=re.search( "(<doc:[^>]+>)", info.split("\n")[0] )
                    if docrlmatch: # a first-line redirect
                        self( docrlmatch.group(1) )
                    else:
                        self.writeHTML( "", info )
                except KeyError:
                    glo=__builtins__.__dict__.get( ob, 0 )
                    if glo:
                        self( glo )
                    else:
                        sys.stderr.write( "No such topic "+`ob` )
                        return None
        else:
            self.write( 'Topic: help( %s )\n' % ob )
            if hasattr( ob, "__doc__" ):
                self.writeText(ob.__doc__)
            else:
                self.writeText( type( ob ).__doc__ )
    

    def writeHTML( self, base,  str ):
        parser=self.Parser(self.Formatter( self.Writer( self )))
        parser.feed( str ) # calls self.write automatically
        for i in range(  len( parser.anchorlist) ):
            self.pager.write( "[%s] %s/%s\n" %(i+1, base,parser.anchorlist[i] ))
        self.pager.flush()
        self.out.write( "\n" )

    def writeText( self, str ):
        self.pager.write( str )
        self.pager.flush()
        self.out.write( "\n" )

    def write( self, str ):
        self.pager.write( str )

from cStringIO import StringIO

class Pager:
    numlines=1

    def __init__(self, out, pagesize=24, linestart="" ):
        self.out=out
        self.pagesize=pagesize
        self.buf=StringIO()
        self.linestart=linestart

    def close(self ):
        self.flush()

    def flush(self ):
        data=self.buf.getvalue().rstrip() # dump trailing ws
        while data.endswith( "\n|" ): # dump trailing lines
            data=data[:-2]
        self.out.write( data )
        self.buf=StringIO()

    def write(self, str ):
        lines=str.split( "\n" )
        self.buf.write( lines[0] )
        for line in lines[1:]:
            self.buf.write( "\n| " )
            self.buf.write( line )
            if self.numlines and not self.numlines%(self.pagesize):
                dat=self.buf.getvalue().strip()
                self.out.write( "| " )
                self.out.write( dat )
                self.out.write( "\n" )
                j=raw_input(prompt)
                if j and j[0]=="q":
                    raise EOFError
                self.buf=StringIO()
            self.numlines=self.numlines+1

help=Help(sys.stdout,24)

def test():
    rc = 1
    rc = rc and help( "everything" )
    rc = rc and help( "exec" )
    rc = rc and help( "doc:lib/unix.html" )
    rc = rc and help( "doc:lib/module-tty.html" )
    rc = rc and help( "doc:ref/print.html" )
    rc = rc and help( "faq" )
    rc = rc and help( dir )
    repr( help )

if __name__=="__main__":
    if len( sys.argv )!=2:
        print "Usage: %s <topic>   or   %s test" % ( sys.argv[0], sys.argv[0] )
        sys.exit(0)
    elif sys.argv[1]=="test":
        test()
    else:
        help( sys.argv[1] )

