"""
Parens and Ticks Closing Extension

When you hit left paren or tick,
automatically creates the closing paren or tick.

Author: Charles M. Wohlganger
        charles.wohlganger@gmail.com

Last Updated: 12-Aug-2017 by Charles Wohlganger

Add to the end of config-extensions.def :

    [ParenClose]
    enable = True
    paren_close = True
    tick_close = True
    skip_closures = True
    comment_space = True
    [ParenClose_cfgBindings]
    p-open = <Key-parenleft> <Key-bracketleft> <Key-braceleft>
    t-open = <Key-'> <Key-">
    p-close = <Key-parenright> <Key-bracketright> <Key-braceright>

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
    '''
    def __init__(self, editwin=None): #setting default to none makes testing easier
        if editwin:
            self.text = editwin.text
        else:
            self.text=None
        self.paren_close = idleConf.GetOption(
            'extensions', 'ParenClose', 'paren_close', default=True)
        self.tick_close = idleConf.GetOption(
            'extensions', 'ParenClose', 'tick_close', default=True)
        self.skip_closures = idleConf.GetOption(
            'extensions', 'ParenClose', 'skip_closures', default=True)
        #sometimes idleConf loads boolean False as string "False"
        if self.paren_close == 'False':
            self.paren_close = False
        if self.tick_close == 'False':
            self.tick_close = False
        if self.skip_closures == 'False':
            self.skip_closures = False

    def p_open_event(self, event):
        if self.paren_close:
            closer = {'(': ')', '[': ']', '{': '}'}[event.char]
            pos = self.text.index('insert')
            self.text.insert(pos, closer)
            self.text.mark_set('insert', pos)

    def p_close_event(self, event):
        pos = self.text.index('insert')
        if self.skip_closures \
           and self.text.get(pos, pos + ' +1c') == event.char:
            self.text.delete(pos, pos + '+1c')

    def t_open_event(self, event):
        if self.tick_close:
            pos = self.text.index('insert')
            if self.text.get(pos + ' -2c', pos) == event.char * 2 \
               and self.text.get(pos, pos + ' +1c') != event.char:
                # Instead of one tick, add two ticks if there are two;
                # user wants to make docstring or multiline.
                self.text.insert(pos, event.char * 3)
                self.text.mark_set('insert', pos)
            else:
                if self.skip_closures \
                   and self.text.get(pos, pos + ' +1c') == event.char:
                    self.text.delete(pos, pos + ' +1c')
                else:
                    self.text.insert(pos, event.char)
                    self.text.mark_set('insert', pos)

if __name__ == '__main__':
    import unittest
    unittest.main('idlelib.idle_test.test_parenclose', verbosity=2)
