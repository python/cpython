# Ridiculously simple test of the winsound module for Windows.

from __future__ import print_function

import functools
import os
import subprocess
import time
import unittest

from test import test_support as support

support.requires('audio')
winsound = support.import_module('winsound')

# Unless we actually have an ear in the room, we have no idea whether a sound
# actually plays, and it's incredibly flaky trying to figure out if a sound
# even *should* play.  Instead of guessing, just call the function and assume
# it either passed or raised the RuntimeError we expect in case of failure.
def sound_func(func):
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        try:
            ret = func(*args, **kwargs)
        except RuntimeError as e:
            if support.verbose:
                print(func.__name__, 'failed:', e)
        else:
            if support.verbose:
                print(func.__name__, 'returned')
            return ret
    return wrapper

safe_Beep = sound_func(winsound.Beep)
safe_MessageBeep = sound_func(winsound.MessageBeep)
safe_PlaySound = sound_func(winsound.PlaySound)

class BeepTest(unittest.TestCase):

    def test_errors(self):
        self.assertRaises(TypeError, winsound.Beep)
        self.assertRaises(ValueError, winsound.Beep, 36, 75)
        self.assertRaises(ValueError, winsound.Beep, 32768, 75)

    def test_extremes(self):
        safe_Beep(37, 75)
        safe_Beep(32767, 75)

    def test_increasingfrequency(self):
        for i in xrange(100, 2000, 100):
            safe_Beep(i, 75)


class MessageBeepTest(unittest.TestCase):

    def tearDown(self):
        time.sleep(0.5)

    def test_default(self):
        self.assertRaises(TypeError, winsound.MessageBeep, "bad")
        self.assertRaises(TypeError, winsound.MessageBeep, 42, 42)
        safe_MessageBeep()

    def test_ok(self):
        safe_MessageBeep(winsound.MB_OK)

    def test_asterisk(self):
        safe_MessageBeep(winsound.MB_ICONASTERISK)

    def test_exclamation(self):
        safe_MessageBeep(winsound.MB_ICONEXCLAMATION)

    def test_hand(self):
        safe_MessageBeep(winsound.MB_ICONHAND)

    def test_question(self):
        safe_MessageBeep(winsound.MB_ICONQUESTION)


class PlaySoundTest(unittest.TestCase):

    def test_errors(self):
        self.assertRaises(TypeError, winsound.PlaySound)
        self.assertRaises(TypeError, winsound.PlaySound, "bad", "bad")
        self.assertRaises(
            RuntimeError,
            winsound.PlaySound,
            "none", winsound.SND_ASYNC | winsound.SND_MEMORY
        )

    def test_aliases(self):
        aliases = [
            "SystemAsterisk",
            "SystemExclamation",
            "SystemExit",
            "SystemHand",
            "SystemQuestion",
        ]
        for alias in aliases:
            safe_PlaySound(alias, winsound.SND_ALIAS)

    def test_alias_fallback(self):
        safe_PlaySound('!"$%&/(#+*', winsound.SND_ALIAS)

    def test_alias_nofallback(self):
        safe_PlaySound('!"$%&/(#+*', winsound.SND_ALIAS | winsound.SND_NODEFAULT)

    def test_stopasync(self):
        safe_PlaySound(
            'SystemQuestion',
            winsound.SND_ALIAS | winsound.SND_ASYNC | winsound.SND_LOOP
        )
        time.sleep(0.5)
        safe_PlaySound('SystemQuestion', winsound.SND_ALIAS | winsound.SND_NOSTOP)
        # Issue 8367: PlaySound(None, winsound.SND_PURGE)
        # does not raise on systems without a sound card.
        winsound.PlaySound(None, winsound.SND_PURGE)


def test_main():
    support.run_unittest(BeepTest, MessageBeepTest, PlaySoundTest)


if __name__=="__main__":
    test_main()
