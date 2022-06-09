##    Copyright(C) 2011-2012 The Board of Trustees of the University of Illinois.
##    All rights reserved.
##
##    Developed by:   Roger D. Serwy
##                    University of Illinois
##
##    Permission is hereby granted, free of charge, to any person obtaining
##    a copy of this software and associated documentation files (the
##    "Software"), to deal with the Software without restriction, including
##    without limitation the rights to use, copy, modify, merge, publish,
##    distribute, sublicense, and/or sell copies of the Software, and to
##    permit persons to whom the Software is furnished to do so, subject to
##    the following conditions:
##
##    + Redistributions of source code must retain the above copyright
##      notice, this list of conditions and the following disclaimers.
##    + Redistributions in binary form must reproduce the above copyright
##      notice, this list of conditions and the following disclaimers in the
##      documentation and/or other materials provided with the distribution.
##    + Neither the names of Roger D. Serwy, the University of Illinois, nor
##      the names of its contributors may be used to endorse or promote
##      products derived from this Software without specific prior written
##      permission.
##
##    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
##    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
##    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
##    IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
##    ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
##    CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
##    THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.

import re
from idlelib.undo import DeleteCommand

jn = lambda x,y: '%i.%i' % (x,y)        # join integers to text coordinates
sp = lambda x: tuple(map(int, x.split('.')))   # convert tkinter Text coordinate to a line and column tuple
ansi_re = re.compile(r'\x01?\x1b\[(.*?)m\x02?')
def strip_ansi(s):
    return ansi_re.sub("", s)

class clearwindow:
    def __init__(self, editwin):
        self.editwin = editwin

    def clear_window_event(self, ev=None):
        self.clear_window(ev)
        return "break"

    def clear_window(self, event):

        per = self.editwin.per
        text = per.bottom
        
        iomark_orig = text.index('iomark')
        line_io, col_io = sp(iomark_orig)

        # if cursor is at the prompt, preserve the prompt (multiline)
        prompt = strip_ansi(">>> ")
        backlines = prompt.count('\n')
        prompt_start = jn(line_io-backlines, 0)
        maybe_prompt = text.get(prompt_start, prompt_start + '+%ic' % len(prompt))
        at_prompt = maybe_prompt == prompt

        if at_prompt:
            endpos = text.index(prompt_start)
        else:
            endpos = text.index('iomark linestart')

        dump = text.dump('1.0', endpos, all=True)

        # Add a command to the undo delegator
        undo = self.editwin.undo
        if undo:
            dc = clearwindowdeletecommand('1.0', endpos, dump)
            undo.addcmd(dc)

        text.edit_reset() # clear out Tkinter's undo history

        
class clearwindowdeletecommand(DeleteCommand):
    def __init__(self, index1, index2, dump):
        DeleteCommand.__init__(self, index1, index2)
        self.dump = dump

    def do(self, text):
        text.delete(self.index1, self.index2)
        text.see('insert')

    def redo(self, text):
        text.delete(self.index1, self.index2)
        text.see('insert')

    def undo(self, text):
        # inspired by "Serializing a text widget" at http://wiki.tcl.tk/9167
        dump = self.dump
        tag = {} # remember the index where a tag was activated
        for key, value, index in dump:
            if key == 'text':
                text.insert(index, value, '')
            elif key == 'tagon':
                tag[value] = index
            elif key == 'tagoff':
                text.tag_add(value, tag[value], index)
                del tag[value]
        # extend existing tags to the end position
        for value in tag:
            text.tag_add(value, tag[value], self.index2)
        text.see('insert')

