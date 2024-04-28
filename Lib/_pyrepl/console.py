#   Copyright 2000-2004 Michael Hudson-Doyle <micahel@gmail.com>
#
#                        All Rights Reserved
#
#
# Permission to use, copy, modify, and distribute this software and
# its documentation for any purpose is hereby granted without fee,
# provided that the above copyright notice appear in all copies and
# that both that copyright notice and this permission notice appear in
# supporting documentation.
#
# THE AUTHOR MICHAEL HUDSON DISCLAIMS ALL WARRANTIES WITH REGARD TO
# THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS, IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL,
# INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
# RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
# CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

import dataclasses


@dataclasses.dataclass
class Event:
    evt: str
    data: str
    raw: bytes = b""


class Console:
    """Attributes:

    screen,
    height,
    width,
    """

    def refresh(self, screen, xy):
        pass

    def prepare(self):
        pass

    def restore(self):
        pass

    def move_cursor(self, x, y):
        pass

    def set_cursor_vis(self, vis):
        pass

    def getheightwidth(self):
        """Return (height, width) where height and width are the height
        and width of the terminal window in characters."""
        pass

    def get_event(self, block=1):
        """Return an Event instance.  Returns None if |block| is false
        and there is no event pending, otherwise waits for the
        completion of an event."""
        pass

    def beep(self):
        pass

    def clear(self):
        """Wipe the screen"""
        pass

    def finish(self):
        """Move the cursor to the end of the display and otherwise get
        ready for end.  XXX could be merged with restore?  Hmm."""
        pass

    def flushoutput(self):
        """Flush all output to the screen (assuming there's some
        buffering going on somewhere)."""
        pass

    def forgetinput(self):
        """Forget all pending, but not yet processed input."""
        pass

    def getpending(self):
        """Return the characters that have been typed but not yet
        processed."""
        pass

    def wait(self):
        """Wait for an event."""
        pass
