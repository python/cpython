"""Line numbering implementation for IDLE as an extension.
Includes BaseSideBar which can be extended for other sidebar based extensions
"""
import functools
import itertools

import tkinter as tk
from idlelib.config import idleConf
from idlelib.delegator import Delegator


def get_end_linenumber(text):
    """Utility to get the last line's number in a Tk text widget."""
    return int(float(text.index('end-1c')))


class BaseSideBar:
    """
    The base class for extensions which require a sidebar.
    """
    def __init__(self, editwin):
        self.editwin = editwin
        self.parent = editwin.text_frame
        self.text = editwin.text

        self.text.bind('<<font-changed>>', self.update_sidebar_text_font)
        self.sidebar_text = tk.Text(self.parent, width=1, wrap=tk.NONE)
        self.sidebar_text.config(state=tk.DISABLED)
        self.text['yscrollcommand'] = self.redirect_yscroll_event

        self.side = None

    def update_sidebar_text_font(self, event=''):
        """
        Implement in subclass to update font config values of sidebar_text
        when font config values of editwin.text changes
        """
        pass

    def show_sidebar(self, side):
        """
        side - Valid values are tk.LEFT, tk.RIGHT
        """
        if side not in {tk.LEFT, tk.RIGHT}:
            raise ValueError(
                'side must be one of: '
                'tk.LEFT = {tk.LEFT!r}; '
                'tk.RIGHT = {tk.RIGHT!r}')
        if side != self.side:
            try:
                self.sidebar_text.pack(side=side, fill=tk.Y, before=self.text)
            except tk.TclError:
                self.sidebar_text.pack(side=side, fill=tk.Y)
            self.side = side

    def hide_sidebar(self):
        if self.side is not None:
            self.sidebar_text.pack_forget()
            self.side = None

    def redirect_yscroll_event(self, *args, **kwargs):
        """Redirect vertical scrolling to the main editor text widget.

        The scroll bar is also updated.
        """
        self.editwin.vbar.set(*args)
        self.sidebar_text.yview_moveto(args[0])
        return 'break'

    def redirect_focusin_event(self, event):
        """Redirect focus-in events to the main editor text widget."""
        self.text.focus_set()
        return 'break'

    def redirect_mousebutton_event(self, event, event_name):
        """Redirect mouse button events to the main editor text widget."""
        self.text.focus_set()
        self.text.event_generate(event_name or event, x=0, y=event.y)
        return 'break'

    def redirect_mousewheel_event(self, event):
        """Redirect mouse wheel events to the editwin text widget."""
        self.text.event_generate('<MouseWheel>',
                                 x=0, y=event.y, delta=event.delta)
        return 'break'


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
        self.changed_callback(get_end_linenumber(self.delegate))

    def delete(self, index1, index2=None):
        self.delegate.delete(index1, index2)
        self.changed_callback(get_end_linenumber(self.delegate))


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
        self.sidebar_text.config(takefocus=False, exportselection=False)

        self.bind_events()

        end = get_end_linenumber(self.text)
        self.update_sidebar_text(end)

        end_line_delegator = EndLineDelegator(self.update_sidebar_text)
        # Insert the delegator after the undo delegator, so that line numbers
        # are properly updated after undo and redo actions.
        end_line_delegator.setdelegate(self.editwin.undo.delegate)
        self.editwin.undo.setdelegate(end_line_delegator)
        # Reset the delegator caches of the delegators "above" the
        # end line delegator we just inserted.
        delegator = self.editwin.per.top
        while delegator is not end_line_delegator:
            delegator.resetcache()
            delegator = delegator.delegate

        self.is_shown = True  # TODO: Read config
        # Note : We invert state here, and call toggle_line_numbers_event
        # to get our desired state
        self.is_shown = not self.is_shown
        self.toggle_line_numbers_event('')

    def bind_events(self):
        # Ensure focus is always redirected to the main editor text widget.
        self.sidebar_text.bind('<FocusIn>', self.redirect_focusin_event)

        # Redirect mouse scrolling to the main editor text widget.
        #
        # Note that without this, scrolling with the mouse only scrolls
        # the line numbers.
        self.sidebar_text.bind('<MouseWheel>', self.redirect_mousewheel_event)

        # Redirect mouse button events to the main editor text widget.
        #
        # Note that double- and triple-clicks must be replaced with normal
        # clicks, since event_generate() doesn't allow generating them
        # directly.
        def bind_mouse_event(event_name, target_event_name=None):
            target_event_name = target_event_name or event_name
            handler = functools.partial(self.redirect_mousebutton_event,
                                        event_name=target_event_name)
            self.sidebar_text.bind(event_name, handler)

        for button in range(1, 5+1):
            for event in (f'<Button-{button}>',
                          f'<Double-Button-{button}>',
                          f'<Triple-Button-{button}>',
                          ):
                bind_mouse_event(event, target_event_name=f'<{button}>')
            for event in (f'<ButtonRelease-{button}>',
                          f'<B{button}-Motion>',
                          ):
                bind_mouse_event(event)

        # These are set by b1_motion_handler() and read by selection_handler();
        # see below.  last_y is passed this way since the mouse Y-coordinate
        # is not available on selection event objects.  last_yview is passed
        # this way to recognize scrolling while the mouse isn't moving.
        last_y = last_yview = None

        # Special handling of dragging with mouse button 1.  In "normal" text
        # widgets this selects text, but the line numbers text widget has
        # selection disabled.  Still, dragging triggers some selection-related
        # functionality under the hood.  Specifically, dragging to above or
        # below the text widget triggers scrolling, in a way that bypasses the
        # other scrolling synchronization mechanisms.i
        def b1_motion_handler(event, *args):
            nonlocal last_y
            nonlocal last_yview
            last_y = event.y
            last_yview = self.sidebar_text.yview()
            if not 0 <= last_y <= self.sidebar_text.winfo_height():
                self.text.yview_moveto(last_yview[0])
            self.redirect_mousebutton_event(event, '<B1-Motion>')
        self.sidebar_text.bind('<B1-Motion>', b1_motion_handler)

        # With mouse-drag scrolling fixed by the above, there is still an edge-
        # case we need to handle: When drag-scrolling, scrolling can continue
        # while the mouse isn't moving, leading to the above fix not scrolling
        # properly.
        def selection_handler(event=None):
            yview = self.sidebar_text.yview()
            if yview != last_yview:
                self.text.yview_moveto(yview[0])
                self.text.event_generate('<B1-Motion>', x=0, y=last_y)
        self.sidebar_text.bind('<<Selection>>', selection_handler)

    @property
    def is_shown(self):
        return self.side is not None

    @is_shown.setter
    def is_shown(self, value):
        if not isinstance(value, bool):
            raise TypeError('is_shown value must be boolean')
        self.side = tk.LEFT if value else None

    def update_sidebar_text_font(self, event=''):
        """Update the font when the editor window's font changes."""
        colors = idleConf.GetHighlight(idleConf.CurrentTheme(), 'linenumber')
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
        self.show_sidebar(tk.LEFT) if not self.is_shown else self.hide_sidebar()
        self.editwin.setvar('<<toggle-line-numbers>>', self.is_shown)
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
