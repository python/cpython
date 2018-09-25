"""
Parens and Ticks Closing Extension

When you hit left paren or tick,
automatically creates the closing paren or tick.

Add to the end of config-extensions.def :

    [ParenClose]
    enable = True
    paren_close = True
    tick_close = True
    skip_closures = True
    mutual_delete = True
    [ParenClose_cfgBindings]
    p-open = <Key-parenleft> <Key-bracketleft> <Key-braceleft>
    t-open = <Key-quotedbl> <Key-quoteright>
    p-close = <Key-parenright> <Key-bracketright> <Key-braceright>
    back = <Key-BackSpace>
    delete = <Key-Delete>
"""

from idlelib.config import idleConf


class ParenClose:
    '''
    When loaded as an extension to IDLE, and paren_close is True, the symbols
    ([{ will have their closures printed after them and the insertion cursor
    moved between the two.  The same is true for tick closures and the symbols
    ' and ".  When \'\'\' or """ are typed and tick_close is True, it will also
    produce the closing symbols. If skip_closures is True, then when a closure
    symbol is typed and the same one is to the right of it, that symbols is
    deleted before the new one is typed, effectively skipping over the closure.
    If the cursor is between two closures and mutual_delete or mutual_backspace
    is True, when the respective command is given, both closures are deleted.
    '''

    closers = {'(': ')', '[': ']', '{': '}', "'": "'", '"': '"'}

    def __init__(self, editwin=None):
        # Setting default to none makes testing easier.
        if editwin:
            self.text = editwin.text
        else:
            self.text = None
        self.paren_close = idleConf.GetOption(
            'extensions', 'ParenClose', 'paren_close',
            type='bool', default=True)
        self.tick_close = idleConf.GetOption(
            'extensions', 'ParenClose', 'tick_close',
            type='bool', default=True)
        self.skip_closures = idleConf.GetOption(
            'extensions', 'ParenClose', 'skip_closures',
            type='bool', default=True)
        self.mutual_delete = idleConf.GetOption(
            'extensions', 'ParenClose', 'mutual_delete',
            type='bool', default=True)
        self.mutual_backspace = idleConf.GetOption(
            'extensions', 'ParenClose', 'mutual_backspace',
            type='bool', default=True)

    def delcheck(self, pos):
        symbol1 = self.text.get(pos + '-1c', pos)
        symbol2 = self.text.get(pos, pos + '+1c')
        return (symbol1 in self.closers
                and self.closers[symbol1] == symbol2)

    def back_event(self, event):
        if self.mutual_backspace:
            pos = self.text.index('insert')
            if self.delcheck(pos):
                self.text.delete(pos, pos + '+1c')

    def delete_event(self, event):
        if self.mutual_delete:
            pos = self.text.index('insert')
            if self.delcheck(pos):
                self.text.delete(pos + '-1c', pos)

    def p_open_event(self, event):
        if self.paren_close:
            pos = self.text.index('insert')
            self.text.insert(pos, self.closers[event.char])
            self.text.mark_set('insert', pos)

    def p_close_event(self, event):
        pos = self.text.index('insert')
        if self.skip_closures \
           and self.text.get(pos, pos + ' +1c') == event.char:
            self.text.delete(pos, pos + '+1c')
            return True
        return False

    def t_open_event(self, event):
        if self.tick_close:
            pos = self.text.index('insert')
            if self.text.get(pos + ' -2c', pos) == event.char * 2 \
               and self.text.get(pos, pos + ' +1c') != event.char:
                # Instead of one tick, add three ticks if there are two;
                # user wants to make docstring or multiline.
                self.text.insert(pos, event.char * 3)
                self.text.mark_set('insert', pos)
            elif not self.p_close_event(event):
                self.p_open_event(event)


if __name__ == '__main__':
    import unittest
    unittest.main('idlelib.idle_test.test_parenclose', verbosity=2)
