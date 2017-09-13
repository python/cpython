"""
Parens and Ticks Closing Extension

When you hit left paren or tick,
automatically creates the closing paren or tick.

Author: Charles M. Wohlganger
        charles.wohlganger@gmail.com

Last Updated: 13-Aug-2017 by Charles Wohlganger

Add to the end of config-extensions.def :

    [ParenClose]
    enable = True
    paren_close = True
    tick_close = True
    skip_closures = True
    comment_space = True
    [ParenClose_cfgBindings]
    p-open = <Key-parenleft> <Key-bracketleft> <Key-braceleft>
    t-open = <Key-quotedbl> <Key-quoteright>
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
    def __init__(self, editwin=None): # Setting default to none makes testing easier.
        if editwin:
            self.text = editwin.text
        else:
            self.text=None
        self.paren_close = idleConf.GetOption(
            'extensions', 'ParenClose', 'paren_close', type = bool, default=True)
        self.tick_close = idleConf.GetOption(
            'extensions', 'ParenClose', 'tick_close', type = bool, default=True)
        self.skip_closures = idleConf.GetOption(
            'extensions', 'ParenClose', 'skip_closures', type = bool, default=True)

    def p_open_event(self, event):
        if self.paren_close:
            closer = {'(': ')', '[': ']', '{': '}', "'":"'", '"':'"'}[event.char]
            pos = self.text.index('insert')
            self.text.insert(pos, closer)
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
