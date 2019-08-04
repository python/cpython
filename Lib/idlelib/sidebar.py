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


def get_widget_padding(widget):
    """Get the total padding of a Tk widget, including its border."""
    # TODO: use also in codecontext.py
    manager = widget.winfo_manager()
    if manager == 'pack':
        info = widget.pack_info()
    elif manager == 'grid':
        info = widget.grid_info()
    else:
        raise ValueError(f"Unsupported geometry manager: {manager}")

    # All values are passed through getint(), since some
    # values may be pixel objects, which can't simply be added to ints.
    padx = sum(map(widget.tk.getint, [
        info['padx'],
        widget.cget('padx'),
        widget.cget('border'),
    ]))
    pady = sum(map(widget.tk.getint, [
        info['pady'],
        widget.cget('pady'),
        widget.cget('border'),
    ]))
    return padx, pady


class BaseSideBar:
    """
    The base class for extensions which require a sidebar.
    """
    def __init__(self, editwin):
        self.editwin = editwin
        self.parent = editwin.text_frame
        self.text = editwin.text

        _padx, pady = get_widget_padding(self.text)
        self.sidebar_text = tk.Text(self.parent, width=1, wrap=tk.NONE,
                                    padx=2, pady=pady,
                                    borderwidth=0, highlightthickness=0)
        self.sidebar_text.config(state=tk.DISABLED)
        self.text['yscrollcommand'] = self.redirect_yscroll_event
        self.update_font()
        self.update_colors()

        self.is_shown = False

    def update_font(self):
        """Update the sidebar text font, usually after config changes."""
        font = idleConf.GetFont(self.text, 'main', 'EditorWindow')
        self._update_font(font)

    def _update_font(self, font):
        self.sidebar_text['font'] = font

    def update_colors(self):
        """Update the sidebar text colors, usually after config changes."""
        colors = idleConf.GetHighlight(idleConf.CurrentTheme(), 'normal')
        self._update_colors(foreground=colors['foreground'],
                            background=colors['background'])

    def _update_colors(self, foreground, background):
        self.sidebar_text.config(
            fg=foreground, bg=background,
            selectforeground=foreground, selectbackground=background,
            inactiveselectbackground=background,
        )

    def show_sidebar(self):
        if not self.is_shown:
            self.sidebar_text.grid(row=1, column=0, sticky=tk.NSEW)
            self.is_shown = True

    def hide_sidebar(self):
        if self.is_shown:
            self.sidebar_text.grid_forget()
            self.is_shown = False

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
        self.text.event_generate(event_name, x=0, y=event.y)
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
        self._sidebar_width_type = type(self.sidebar_text['width'])
        self.sidebar_text.config(state=tk.NORMAL)
        self.sidebar_text.insert('insert', '1', 'linenumber')
        self.sidebar_text.config(state=tk.DISABLED)
        self.sidebar_text.config(takefocus=False, exportselection=False)
        self.sidebar_text.tag_config('linenumber', justify=tk.RIGHT)

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

        self.is_shown = False

    def bind_events(self):
        # Ensure focus is always redirected to the main editor text widget.
        self.sidebar_text.bind('<FocusIn>', self.redirect_focusin_event)

        # Redirect mouse scrolling to the main editor text widget.
        #
        # Note that without this, scrolling with the mouse only scrolls
        # the line numbers.
        self.sidebar_text.bind('<MouseWheel>', self.redirect_mousewheel_event)

        # Redirect mouse button events to the main editor text widget,
        # except for the left mouse button (1).
        #
        # Note: X-11 sends Button-4 and Button-5 events for the scroll wheel.
        def bind_mouse_event(event_name, target_event_name):
            handler = functools.partial(self.redirect_mousebutton_event,
                                        event_name=target_event_name)
            self.sidebar_text.bind(event_name, handler)

        for button in [2, 3, 4, 5]:
            for event_name in (f'<Button-{button}>',
                               f'<ButtonRelease-{button}>',
                               f'<B{button}-Motion>',
                               ):
                bind_mouse_event(event_name, target_event_name=event_name)

            # Convert double- and triple-click events to normal click events,
            # since event_generate() doesn't allow generating such events.
            for event_name in (f'<Double-Button-{button}>',
                               f'<Triple-Button-{button}>',
                               ):
                bind_mouse_event(event_name,
                                 target_event_name=f'<Button-{button}>')

        start_line = None
        def b1_mousedown_handler(event):
            # select the entire line
            lineno = self.editwin.getlineno(f"@0,{event.y}")
            self.text.tag_remove("sel", "1.0", "end")
            self.text.tag_add("sel", f"{lineno}.0", f"{lineno+1}.0")
            self.text.mark_set("insert", f"{lineno+1}.0")

            # remember this line in case this is the beginning of dragging
            nonlocal start_line
            start_line = lineno
        self.sidebar_text.bind('<Button-1>', b1_mousedown_handler)

        # These are set by b1_motion_handler() and read by selection_handler();
        # see below.  last_y is passed this way since the mouse Y-coordinate
        # is not available on selection event objects.  last_yview is passed
        # this way to recognize scrolling while the mouse isn't moving.
        last_y = last_yview = None

        def drag_update_selection_and_insert_mark(y_coord):
            """Helper function for drag and selection event handlers."""
            lineno = self.editwin.getlineno(f"@0,{y_coord}")
            a, b = sorted([start_line, lineno])
            self.text.tag_remove("sel", "1.0", "end")
            self.text.tag_add("sel", f"{a}.0", f"{b+1}.0")
            self.text.mark_set("insert",
                               f"{lineno if lineno == a else lineno + 1}.0")

        # Special handling of dragging with mouse button 1.  In "normal" text
        # widgets this selects text, but the line numbers text widget has
        # selection disabled.  Still, dragging triggers some selection-related
        # functionality under the hood.  Specifically, dragging to above or
        # below the text widget triggers scrolling, in a way that bypasses the
        # other scrolling synchronization mechanisms.i
        def b1_drag_handler(event, *args):
            nonlocal last_y
            nonlocal last_yview
            last_y = event.y
            last_yview = self.sidebar_text.yview()
            if not 0 <= last_y <= self.sidebar_text.winfo_height():
                self.text.yview_moveto(last_yview[0])
            drag_update_selection_and_insert_mark(event.y)
        self.sidebar_text.bind('<B1-Motion>', b1_drag_handler)

        # With mouse-drag scrolling fixed by the above, there is still an edge-
        # case we need to handle: When drag-scrolling, scrolling can continue
        # while the mouse isn't moving, leading to the above fix not scrolling
        # properly.
        def selection_handler(event):
            yview = self.sidebar_text.yview()
            if yview != last_yview:
                self.text.yview_moveto(yview[0])
                drag_update_selection_and_insert_mark(last_y)
        self.sidebar_text.bind('<<Selection>>', selection_handler)

    def update_colors(self):
        """Update the sidebar text colors, usually after config changes."""
        colors = idleConf.GetHighlight(idleConf.CurrentTheme(), 'linenumber')
        self._update_colors(foreground=colors['foreground'],
                            background=colors['background'])

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
            self.sidebar_text.insert(f'end -1c', new_text, 'linenumber')
        else:
            self.sidebar_text.delete(f'{end+1}.0 -1c', 'end -1c')
        self.sidebar_text.config(state=tk.DISABLED)

        self.prev_end = end


def _linenumbers_drag_scrolling(parent):  # htest #
    from idlelib.idle_test.test_sidebar import Dummy_editwin

    toplevel = tk.Toplevel(parent)
    text_frame = tk.Frame(toplevel)
    text_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
    text_frame.rowconfigure(1, weight=1)
    text_frame.columnconfigure(1, weight=1)

    font = idleConf.GetFont(toplevel, 'main', 'EditorWindow')
    text = tk.Text(text_frame, width=80, height=24, wrap=tk.NONE, font=font)
    text.grid(row=1, column=1, sticky=tk.NSEW)

    editwin = Dummy_editwin(text)
    editwin.vbar = tk.Scrollbar(text_frame)

    linenumbers = LineNumbers(editwin)
    linenumbers.show_sidebar()

    text.insert('1.0', '\n'.join('a'*i for i in range(1, 101)))


if __name__ == '__main__':
    from unittest import main
    main('idlelib.idle_test.test_sidebar', verbosity=2, exit=False)

    from idlelib.idle_test.htest import run
    run(_linenumbers_drag_scrolling)
