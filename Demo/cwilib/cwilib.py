# Interface to the interactive CWI library catalog.

import sys
import stdwin
from stdwinevents import *
import select
import telnetlib
import vt100win
from form import Form


# Main program

def main():
    vt = vt100win.VT100win()
    #
    host = 'biefstuk.cwi.nl'
    tn = telnetlib.Telnet(host, 0)
    #
    try:
        vt.send(tn.read_until('login: ', 10))
        tn.write('cwilib\r')
        #
        vt.send(tn.read_until('Hit <RETURN> to continue...', 10))
        tn.write('\r')
        #
        vt.send(tn.read_until('QUIT', 20))
    except EOFError:
        sys.stderr.write('Connection closed prematurely\n')
        sys.exit(1)
    #
    define_screens(vt)
    matches = vt.which_screens()
    if 'menu' not in matches:
        sys.stderr.write('Main menu does not appear\n')
        sys.exit(1)
    #
    tn.write('\r\r')
    vt.open('Progress -- CWI Library')
    vt.set_debuglevel(0)
    ui = UserInterface()
    #
    while 1:
        try:
            data = tn.read_very_eager()
        except EOFError:
            stdwin.message('Connection closed--goodbye')
            break
        if data:
            print 'send...'
            vt.send(data)
            print 'send...done'
            continue
        event = stdwin.pollevent()
        if event:
            type, window, detail = event
            if window == None and type == WE_LOST_SEL:
                window = ui.queryform.window
                event = type, window, detail
            if type == WE_CLOSE:
                break
            if window in ui.windows:
                ui.dispatch(type, window, detail)
            elif window == vt.window:
                if type == WE_NULL:
                    pass
                elif type == WE_COMMAND:
                    if detail == WC_RETURN:
                        tn.write('\r')
                    elif detail == WC_BACKSPACE:
                        tn.write('\b')
                    elif detail == WC_TAB:
                        tn.write('\t')
                    elif detail == WC_UP:
                        tn.write('\033[A')
                    elif detail == WC_DOWN:
                        tn.write('\033[B')
                    elif detail == WC_RIGHT:
                        tn.write('\033[C')
                    elif detail == WC_LEFT:
                        tn.write('\033[D')
                    else:
                        print '*** Command:', detail
                elif type == WE_CHAR:
                    tn.write(detail)
                elif type == WE_DRAW:
                    vt.draw(detail)
                elif type in (WE_ACTIVATE, WE_DEACTIVATE):
                    pass
                else:
                    print '*** VT100 event:', type, detail
            else:
                print '*** Alien event:', type, window, detail
            continue
        rfd, wfd, xfd = select.select([tn, stdwin], [], [])


# Subroutine to define our screen recognition patterns

def define_screens(vt):
    vt.define_screen('menu', {
              'title': ('search', 0, 0, 80,
                        ' SEARCH FUNCTIONS  +OTHER FUNCTIONS '),
              })
    vt.define_screen('search', {
              'title': ('search', 0, 0, 80, ' Search '),
              })
    vt.define_screen('shortlist', {'title': ('search', 0, 0, 80,
              ' Short-list')})
    vt.define_screen('showrecord', {
              'title': ('search', 0, 0, 80, ' Show record '),
              })
    vt.define_screen('timelimit', {
              'limit': ('search', 12, 0, 80, ' TIME LIMIT '),
              })
    vt.define_screen('attention', {
              'BASE': ('copy', 0, 0, 0, 'search'),
              'title': ('search', 10, 0, 80, ' ATTENTION ')})
    vt.define_screen('syntaxerror', {
              'BASE': ('copy', 0, 0, 0, 'attention'),
              'message': ('search', 12, 0, 80, ' Syntax error'),
              })
    vt.define_screen('emptyerror', {
              'BASE': ('copy', 0, 0, 0, 'attention'),
              'message': ('search', 12, 0, 80,
                          ' Check your input. Search at least one term'),
              })
    vt.define_screen('unsortedwarning', {
              'BASE': ('copy', 0, 0, 0, 'attention'),
              'message': ('search', 12, 0, 80,
                          ' Number of records exceeds sort limit'),
              })
    vt.define_screen('thereismore', {
              'BASE': ('copy', 0, 0, 0, 'showrecord'),
              'message': ('search', 15, 0, 80,
                 'There is more within this record. Use the arrow keys'),
              })
    vt.define_screen('nofurther', {
              'BASE': ('copy', 0, 0, 0, 'showrecord'),
              'message': ('search', 17, 0, 80, 'You cannot go further\.'),
              })
    vt.define_screen('nofurtherback', {
              'BASE': ('copy', 0, 0, 0, 'showrecord'),
              'message': ('search', 17, 0, 80,
                          'You cannot go further back'),
              })


# Class to implement our user interface.

class UserInterface:

    def __init__(self):
        stdwin.setfont('7x14')
        self.queryform = QueryForm()
        self.listform = ListForm()
        self.recordform = RecordForm()
        self.forms = [self.queryform, self.listform, self.recordform]
        define_query_fields(self.queryform)
        self.windows = []
        for form in self.forms:
            if form.formheight > 0:
                form.open()
                self.windows.append(form.window)

    def __del__(self):
        self.close()

    def close(self):
        for form in self.forms:
            form.close()

    def dispatch(self, type, window, detail):
        for form in self.forms:
            if window == form.window:
                form.dispatch(type, detail)


def define_query_fields(f):
    f.define_field('name', 'Name auth./ed.', 1, 60)
    f.define_field('title',  'Title', 4, 60)
    f.define_field('shelfmark', 'Shelf mark', 1, 60)
    f.define_field('class', 'Prim. classif.', 1, 60)
    f.define_field('series', 'Series', 1, 60)
    f.define_field('congress', 'Congr. pl./year', 1, 60)
    f.define_field('type', 'Type', 1, 60)


class QueryForm(Form):

    def __init__(self):
        Form.__init__(self, 'Query form -- CWI Library')

    def dispatch(self, type, detail):
        if type == WE_COMMAND and detail == WC_RETURN:
            print '*** SUBMIT ***'
        else:
            Form.dispatch(self, type, detail)


class ListForm(Form):

    def __init__(self):
        Form.__init__(self, 'Short list -- CWI Library')


class RecordForm(Form):

    def __init__(self):
        Form.__init__(self, 'Record detail -- CWI Library')


main()
