# Ridiculously simple test of the winsound module for Windows.

import unittest
from test import test_support
import winsound, time

class BeepTest(unittest.TestCase):

    def test_errors(self):
        self.assertRaises(TypeError, winsound.Beep)
        self.assertRaises(ValueError, winsound.Beep, 36, 75)
        self.assertRaises(ValueError, winsound.Beep, 32768, 75)

    def test_extremes(self):
        winsound.Beep(37, 75)
        winsound.Beep(32767, 75)

    def test_increasingfrequency(self):
        for i in xrange(100, 2000, 100):
            winsound.Beep(i, 75)

class MessageBeepTest(unittest.TestCase):

    def tearDown(self):
        time.sleep(0.5)

    def test_default(self):
        self.assertRaises(TypeError, winsound.MessageBeep, "bad")
        self.assertRaises(TypeError, winsound.MessageBeep, 42, 42)
        winsound.MessageBeep()

    def test_ok(self):
        winsound.MessageBeep(winsound.MB_OK)

    def test_asterisk(self):
        winsound.MessageBeep(winsound.MB_ICONASTERISK)

    def test_exclamation(self):
        winsound.MessageBeep(winsound.MB_ICONEXCLAMATION)

    def test_hand(self):
        winsound.MessageBeep(winsound.MB_ICONHAND)

    def test_question(self):
        winsound.MessageBeep(winsound.MB_ICONQUESTION)

class PlaySoundTest(unittest.TestCase):

    def test_errors(self):
        self.assertRaises(TypeError, winsound.PlaySound)
        self.assertRaises(TypeError, winsound.PlaySound, "bad", "bad")
        self.assertRaises(
            RuntimeError,
            winsound.PlaySound,
            "none", winsound.SND_ASYNC | winsound.SND_MEMORY
        )

    def test_alias_asterisk(self):
        winsound.PlaySound('SystemAsterisk', winsound.SND_ALIAS)

    def test_alias_exclamation(self):
        winsound.PlaySound('SystemExclamation', winsound.SND_ALIAS)

    def test_alias_exit(self):
        winsound.PlaySound('SystemExit', winsound.SND_ALIAS)

    def test_alias_hand(self):
        winsound.PlaySound('SystemHand', winsound.SND_ALIAS)

    def test_alias_question(self):
        winsound.PlaySound('SystemQuestion', winsound.SND_ALIAS)

    def test_alias_fallback(self):
        winsound.PlaySound('!"$%&/(#+*', winsound.SND_ALIAS)

    def test_alias_nofallback(self):
        try:
            winsound.PlaySound(
                '!"$%&/(#+*',
                winsound.SND_ALIAS | winsound.SND_NODEFAULT
            )
        except RuntimeError:
            pass

    def test_stopasync(self):
        winsound.PlaySound(
            'SystemQuestion',
            winsound.SND_ALIAS | winsound.SND_ASYNC | winsound.SND_LOOP
        )
        time.sleep(0.5)
        try:
            winsound.PlaySound(
                'SystemQuestion',
                winsound.SND_ALIAS | winsound.SND_NOSTOP
            )
        except RuntimeError:
            pass
        else: # the first sound might already be finished
            pass
        winsound.PlaySound(None, winsound.SND_PURGE)

def test_main():
    test_support.run_unittest(BeepTest, MessageBeepTest, PlaySoundTest)

if __name__=="__main__":
    test_main()
