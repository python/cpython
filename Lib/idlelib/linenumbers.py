"""Line numbering implementation for IDLE as an extension.
Includes BaseSideBar which can be extended for other sidebar based extensions
"""
import itertools

import tkinter as tk
from idlelib.config import idleConf
from idlelib.delegator import Delegator

DISABLED = False
ENABLED = True

get_end = lambda text: int(float(text.index('end-1c')))


class BaseSideBar:
    """
    The base class for extensions which require a sidebar.
    """
    def __init__(self, editwin):
        self.editwin = editwin
        self.text = editwin.text
        self.text.bind('<<font-changed>>', self.update_sidebar_text_font)
        self.parent = self.text.nametowidget(self.text.winfo_parent())
        self.sidebar_text = tk.Text(self.parent, width=1, wrap=tk.NONE)
        self.sidebar_text.config(state=tk.DISABLED)
        self.text['yscrollcommand'] = self.vbar_set
        self.sidebar_text['yscrollcommand'] = self.vbar_set

    def update_sidebar_text_font(self, event=''):
        """
        Implement in subclass to update font config values of sidebar_text
        when font config values of editwin.text changes
        """

    def show_sidebar(self, side=tk.LEFT):
        """
        side - Valid values are tk.LEFT, tk.RIGHT, tk.TOP, tk.BOTTOM
        """
        try:
            self.sidebar_text.pack(side=tk.LEFT, fill=tk.Y, before=self.text)
        except tk.TclError:
            self.sidebar_text.pack(side=tk.LEFT, fill=tk.Y)
        self.state = ENABLED

    def hide_sidebar(self):
        self.sidebar_text.pack_forget()
        self.state = DISABLED

    def vbar_set(self, *args, **kwargs):
        """Redirect scrollbar's set command to editwin.text and sidebar_text
        """
        self.editwin.vbar.set(*args)
        self.sidebar_text.yview_moveto(args[0])
        self.text.yview_moveto(args[0])

    def redirect_event(self, event, event_name):
        """Set focus to editwin.text and redirect 'event' to editwin.text.
        """
        self.text.focus_set()
        kwargs = dict(x=event.x, y=event.y)
        if event_name == '<MouseWheel>':
            kwargs.update(delta=event.delta)
        self.text.event_generate(event_name, **kwargs)


class EndLineDelegator(Delegator):
    """Generate callbacks with the current end line number after
       insert or delete operations"""
    def __init__(self, changed_callback):
        """
        changed_callback - Callable, will be called after insert
                           or delete operations with the current
                           end line number.
        """
        Delegator.__init__(self)
        self.changed_callback = changed_callback

    def insert(self, index, chars, tags=None):
        self.delegate.insert(index, chars, tags)
        self.changed_callback(get_end(self.delegate))

    def delete(self, index1, index2=None):
        self.delegate.delete(index1, index2)
        self.changed_callback(get_end(self.delegate))


class LineNumbers(BaseSideBar):
    """Line numbers support for editor windows."""
    def __init__(self, editwin):
        BaseSideBar.__init__(self, editwin)
        self.prev_end = 1
        self.update_sidebar_text_font()
        self._sidebar_width_type = type(self.sidebar_text['width'])
        self.sidebar_text.config(state=tk.NORMAL)
        self.sidebar_text.insert('insert', '1', 'linenumber')
        self.sidebar_text.config(state=tk.DISABLED)
        for event_name in ('<Button-2>', '<Button-3>', '<Button-4>',
                           '<Button-5>', '<B2-Motion>', '<B3-Motion>',
                           '<ButtonRelease-2>', '<ButtonRelease-3>',
                           '<Double-Button-1>', '<Double-Button-2>',
                           '<Double-Button-3>', '<Enter>', '<Leave>',
                           '<2>', '<3>', '<MouseWheel>',
                           '<FocusIn>'):
            self.sidebar_text.bind(event_name,
                                   lambda event, event_name=event_name:
                                   self.redirect_event(event, event_name))
        self.end_line_delegator = EndLineDelegator(self.update_sidebar_text)
        end = get_end(self.text)
        self.update_sidebar_text(end)
        self.editwin.per.insertfilter(self.end_line_delegator)
        # self.state = idleConf.GetOption('extensions', 'LineNumber', 'visible',
        #                                 type='bool')
        self.state = True  # TODO: Read config
        # Note : We invert state here, and call toggle_line_numbers_event
        # to get our desired state
        self.state = not self.state
        self.toggle_line_numbers_event('')

    def update_sidebar_text_font(self, event=''):
        """Update the font when the editor window's font changes."""
        colors = idleConf.GetHighlight(idleConf.CurrentTheme(), 'context')
        bg = colors['background']
        fg = colors['foreground']
        self.sidebar_text.tag_config('linenumber', justify=tk.RIGHT)
        config = {'fg': fg, 'bg': bg, 'font': self.text['font'],
                  'relief': tk.FLAT, 'selectforeground': fg,
                  'selectbackground': bg}
        if tk.TkVersion >= 8.5:
            config['inactiveselectbackground'] = bg
        self.sidebar_text.config(**config)
        # The below lines below are required to allow tk to "catch up" with
        # changes in font to the main text widget
        #
        # TODO: validate the assertion above
        sidebar_text = self.sidebar_text.get('1.0', 'end')
        self.sidebar_text.delete('1.0', 'end')
        self.sidebar_text.insert('1.0', sidebar_text)
        self.text.update_idletasks()
        self.sidebar_text.update_idletasks()

    def toggle_line_numbers_event(self, event):
        self.show_sidebar() if self.state == DISABLED else self.hide_sidebar()
        self.editwin.setvar('<<toggle-line-numbers>>', self.state)
        # idleConf.SetOption('extensions', 'LineNumber', 'visible',
        #                    str(self.state))
        # idleConf.SaveUserCfgFiles()

    def update_sidebar_text(self, end):
        """
        Perform the following action:
        Each line sidebar_text contains the linenumber for that line
        Synchronize with editwin.text so that both sidebar_text and
        editwin.text contain the same number of lines"""
        if end == self.prev_end:
            return

        width_difference = len(str(end)) - len(str(self.prev_end))
        if width_difference:
            cur_width = int(float(self.sidebar_text['width']))
            new_width = cur_width + width_difference
            self.sidebar_text['width'] = self._sidebar_width_type(new_width)

        self.sidebar_text.config(state=tk.NORMAL)
        if end > self.prev_end:
            new_text = '\n'.join(itertools.chain(
                [''],
                map(str, range(self.prev_end + 1, end + 1)),
            ))
            self.sidebar_text.insert(f'{end+1:d}.0', new_text, 'linenumber')
        else:
            self.sidebar_text.delete(f'{end+1:d}.0', 'end')
        self.sidebar_text.config(state=tk.DISABLED)

        self.prev_end = end


if __name__ == '__main__':
    from unittest import main
    main('idlelib.idle_test.test_linenumbers', verbosity=2)
