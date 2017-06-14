'''Provide line number sidebar to editor

Inspire by Bryan Oakley, on stackflow: https://stackoverflow.com/a/16375233
'''

import tkinter as tk


class LineNumberSidebar(tk.Canvas):
    def __init__(self, *args, **kwargs):
        tk.Canvas.__init__(self, highlightthickness=0, background='grey93', *args, **kwargs)

    def attach(self, text_widget):
        self.text = text_widget

    def redraw(self, *args):
        '''redraw line numbers'''
        self.delete('all')

        # Get cursor lineno
        curline, column = self.text.index(tk.INSERT).split('.')

        # Redraw from top
        i = self.text.index('@0,0')
        while True:
            dline = self.text.dlineinfo(i)
            if dline is None:
                break

            lineno = i.split('.')[0]
            text = self.create_text(45, dline[1], anchor='ne', text=lineno)

            # Highlight cursor line number with white rectangle
            if lineno == curline:
                n, e, s, w = self.bbox(text)
                r = self.create_rectangle((1, e, s + 3, w), fill='white', outline='white')
                self.tag_lower(r, text)

            # Update index
            i = self.text.index('%s + 1line' % i)


class SidebarText(tk.Text):
    def __init__(self, *args, **kwargs):
        tk.Text.__init__(self, *args, **kwargs)

        self.tk.eval('''
            proc widget_proxy {widget widget_command args} {

                # call the real tk widget command with the real args
                set result [uplevel [linsert $args 0 $widget_command]]

                # generate the event for certain types of commands
                if {([lindex $args 0] in {insert replace delete}) ||
                    ([lrange $args 0 2] == {mark set insert}) ||
                    ([lrange $args 0 1] == {xview moveto}) ||
                    ([lrange $args 0 1] == {xview scroll}) ||
                    ([lrange $args 0 1] == {yview moveto}) ||
                    ([lrange $args 0 1] == {yview scroll})} {

                    event generate  $widget <<Change>> -when tail
                }

                # return the result from the real widget command
                return $result
            }
            ''')
        self.tk.eval('''
            rename {widget} _{widget}
            interp alias {{}} ::{widget} {{}} widget_proxy {widget} _{widget}
        '''.format(widget=str(self)))
