"""Extension to execute a script in a separate process

David Scherer <dscherer@cmu.edu>

  The ExecBinding module, a replacement for ScriptBinding, executes
  programs in a separate process.  Unlike previous versions, this version
  communicates with the user process via an RPC protocol (see the 'protocol'
  module).  The user program is loaded by the 'loader' and 'Remote'
  modules.  Its standard output and input are directed back to the
  ExecBinding class through the RPC mechanism and implemented here.

  A "stop program" command is provided and bound to control-break.  Closing
  the output window also stops the running program.
"""

import sys
import os
import imp
import OutputWindow
import protocol
import spawn
import traceback
import tempfile

# Find Python and the loader.  This should be done as early in execution
#   as possible, because if the current directory or sys.path is changed
#   it may no longer be possible to get correct paths for these things.

pyth_exe = spawn.hardpath( sys.executable )
load_py  = spawn.hardpath( imp.find_module("loader")[1] )

# The following mechanism matches loaders up with ExecBindings that are
#    trying to load something.

waiting_for_loader = []

def loader_connect(client, addr):
    if waiting_for_loader:
        a = waiting_for_loader.pop(0)
        try:
            return a.connect(client, addr)
        except:
            return loader_connect(client,addr)

protocol.publish('ExecBinding', loader_connect)

class ExecBinding:
    menudefs = [
        ('run', [None,
                  ('Run program', '<<run-complete-script>>'),
                  ('Stop program', '<<stop-execution>>'),
                 ]
        ),
    ]

    delegate = 1

    def __init__(self, editwin):
        self.editwin = editwin
        self.client = None
        self.temp = []

        if not hasattr(editwin, 'source_window'):
            self.delegate = 0
            self.output = OutputWindow.OnDemandOutputWindow(editwin.flist)
            self.output.close_hook = self.stopProgram
            self.output.source_window = editwin
        else:
            if (self.editwin.source_window and
                self.editwin.source_window.extensions.has_key('ExecBinding') and
                not self.editwin.source_window.extensions['ExecBinding'].delegate):
                    delegate = self.editwin.source_window.extensions['ExecBinding']
                    self.run_complete_script_event = delegate.run_complete_script_event
                    self.stop_execution_event = delegate.stop_execution_event

    def __del__(self):
        self.stopProgram()

    def stop_execution_event(self, event):
        if self.client:
            self.stopProgram()
            self.write('\nProgram stopped.\n','stderr')

    def run_complete_script_event(self, event):
        filename = self.getfilename()
        if not filename: return
        filename = os.path.abspath(filename)

        self.stopProgram()

        self.commands = [ ('run', filename) ]
        waiting_for_loader.append(self)
        spawn.spawn( pyth_exe, load_py )

    def connect(self, client, addr):
        # Called by loader_connect() above.  It is remotely possible that
        #   we get connected to two loaders if the user is running the
        #   program repeatedly in a short span of time.  In this case, we
        #   simply return None, refusing to connect and letting the redundant
        #   loader die.
        if self.client: return None

        self.client = client
        client.set_close_hook( self.connect_lost )

        title = self.editwin.short_title()
        if title:
            self.output.set_title(title + " Output")
        else:
            self.output.set_title("Output")
        self.output.write('\n',"stderr")
        self.output.scroll_clear()

        return self

    def connect_lost(self):
        # Called by the client's close hook when the loader closes its
        #   socket.

        # We print a disconnect message only if the output window is already
        #   open.
        if self.output.owin and self.output.owin.text:
            self.output.owin.interrupt()
            self.output.write("\nProgram disconnected.\n","stderr")

        for t in self.temp:
            try:
                os.remove(t)
            except:
                pass
        self.temp = []
        self.client = None

    def get_command(self):
        # Called by Remote to find out what it should be executing.
        # Later this will be used to implement debugging, interactivity, etc.
        if self.commands:
            return self.commands.pop(0)
        return ('finish',)

    def program_exception(self, type, value, tb, first, last):
        if type == SystemExit: return 0

        for i in range(len(tb)):
            filename, lineno, name, line = tb[i]
            if filename in self.temp:
                filename = 'Untitled'
            tb[i] = filename, lineno, name, line

        list = traceback.format_list(tb[first:last])
        exc = traceback.format_exception_only( type, value )

        self.write('Traceback (innermost last)\n', 'stderr')
        for i in (list+exc):
            self.write(i, 'stderr')

        self.commands = []
        return 1

    def write(self, text, tag):
        self.output.write(text,tag)

    def readline(self):
        return self.output.readline()

    def stopProgram(self):
        if self.client:
          self.client.close()
          self.client = None

    def getfilename(self):
        # Save all files which have been named, because they might be modules
        for edit in self.editwin.flist.inversedict.keys():
            if edit.io and edit.io.filename and not edit.get_saved():
                edit.io.save(None)

        # Experimental: execute unnamed buffer
        if not self.editwin.io.filename:
            filename = os.path.normcase(os.path.abspath(tempfile.mktemp()))
            self.temp.append(filename)
            if self.editwin.io.writefile(filename):
                return filename

        # If the file isn't save, we save it.  If it doesn't have a filename,
        #   the user will be prompted.
        if self.editwin.io and not self.editwin.get_saved():
            self.editwin.io.save(None)

        # If the file *still* isn't saved, we give up.
        if not self.editwin.get_saved():
            return

        return self.editwin.io.filename
