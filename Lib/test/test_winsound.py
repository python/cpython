# Ridiculously simple test of the winsound module for Windows.

import unittest
from test import test_support
test_support.requires('audio')
import winsound, time
import os
import subprocess


class BeepTest(unittest.TestCase):
    # As with PlaySoundTest, incorporate the _have_soundcard() check
    # into our test methods.  If there's no audio device present,
    # winsound.Beep returns 0 and GetLastError() returns 127, which
    # is: ERROR_PROC_NOT_FOUND ("The specified procedure could not
    # be found").  (FWIW, virtual/Hyper-V systems fall under this
    # scenario as they have no sound devices whatsoever  (not even
    # a legacy Beep device).)

    def test_errors(self):
        self.assertRaises(TypeError, winsound.Beep)
        self.assertRaises(ValueError, winsound.Beep, 36, 75)
        self.assertRaises(ValueError, winsound.Beep, 32768, 75)

    def test_extremes(self):
        if _have_soundcard():
            winsound.Beep(37, 75)
            winsound.Beep(32767, 75)
        else:
            # The behaviour of winsound.Beep() seems to differ between
            # different versions of Windows when there's either a) no
            # sound card entirely, b) legacy beep driver has been disabled,
            # or c) the legacy beep driver has been uninstalled.  Sometimes
            # RuntimeErrors are raised, sometimes they're not.  Meh.
            try:
                winsound.Beep(37, 75)
                winsound.Beep(32767, 75)
            except RuntimeError:
                pass

    def test_increasingfrequency(self):
        if _have_soundcard():
            for i in range(100, 2000, 100):
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
        if _have_soundcard():
            winsound.PlaySound('SystemAsterisk', winsound.SND_ALIAS)
        else:
            self.assertRaises(
                RuntimeError,
                winsound.PlaySound,
                'SystemAsterisk', winsound.SND_ALIAS
            )

    def test_alias_exclamation(self):
        if _have_soundcard():
            winsound.PlaySound('SystemExclamation', winsound.SND_ALIAS)
        else:
            self.assertRaises(
                RuntimeError,
                winsound.PlaySound,
                'SystemExclamation', winsound.SND_ALIAS
            )

    def test_alias_exit(self):
        if _have_soundcard():
            winsound.PlaySound('SystemExit', winsound.SND_ALIAS)
        else:
            self.assertRaises(
                RuntimeError,
                winsound.PlaySound,
                'SystemExit', winsound.SND_ALIAS
            )

    def test_alias_hand(self):
        if _have_soundcard():
            winsound.PlaySound('SystemHand', winsound.SND_ALIAS)
        else:
            self.assertRaises(
                RuntimeError,
                winsound.PlaySound,
                'SystemHand', winsound.SND_ALIAS
            )

    def test_alias_question(self):
        if _have_soundcard():
            winsound.PlaySound('SystemQuestion', winsound.SND_ALIAS)
        else:
            self.assertRaises(
                RuntimeError,
                winsound.PlaySound,
                'SystemQuestion', winsound.SND_ALIAS
            )

    def test_alias_fallback(self):
        # This test can't be expected to work on all systems.  The MS
        # PlaySound() docs say:
        #
        #     If it cannot find the specified sound, PlaySound uses the
        #     default system event sound entry instead.  If the function
        #     can find neither the system default entry nor the default
        #     sound, it makes no sound and returns FALSE.
        #
        # It's known to return FALSE on some real systems.

        # winsound.PlaySound('!"$%&/(#+*', winsound.SND_ALIAS)
        return

    def test_alias_nofallback(self):
        if _have_soundcard():
            # Note that this is not the same as asserting RuntimeError
            # will get raised:  you cannot convert this to
            # self.assertRaises(...) form.  The attempt may or may not
            # raise RuntimeError, but it shouldn't raise anything other
            # than RuntimeError, and that's all we're trying to test
            # here.  The MS docs aren't clear about whether the SDK
            # PlaySound() with SND_ALIAS and SND_NODEFAULT will return
            # True or False when the alias is unknown.  On Tim's WinXP
            # box today, it returns True (no exception is raised).  What
            # we'd really like to test is that no sound is played, but
            # that requires first wiring an eardrum class into unittest
            # <wink>.
            try:
                winsound.PlaySound(
                    '!"$%&/(#+*',
                    winsound.SND_ALIAS | winsound.SND_NODEFAULT
                )
            except RuntimeError:
                pass
        else:
            self.assertRaises(
                RuntimeError,
                winsound.PlaySound,
                '!"$%&/(#+*', winsound.SND_ALIAS | winsound.SND_NODEFAULT
            )

    def test_stopasync(self):
        if _have_soundcard():
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
        else:
            self.assertRaises(
                RuntimeError,
                winsound.PlaySound,
                None, winsound.SND_PURGE
            )


def _get_cscript_path():
    """Return the full path to cscript.exe or None."""
    for dir in os.environ.get("PATH", "").split(os.pathsep):
        cscript_path = os.path.join(dir, "cscript.exe")
        if os.path.exists(cscript_path):
            return cscript_path

__have_soundcard_cache = None
def _have_soundcard():
    """Return True iff this computer has a soundcard."""
    global __have_soundcard_cache
    if __have_soundcard_cache is None:
        cscript_path = _get_cscript_path()
        if cscript_path is None:
            # Could not find cscript.exe to run our VBScript helper. Default
            # to True: most computers these days *do* have a soundcard.
            return True

        check_script = os.path.join(os.path.dirname(__file__),
                                    "check_soundcard.vbs")
        p = subprocess.Popen([cscript_path, check_script],
                             stdout=subprocess.PIPE)
        __have_soundcard_cache = not p.wait()
    return __have_soundcard_cache


def test_main():
    test_support.run_unittest(BeepTest, MessageBeepTest, PlaySoundTest)

if __name__=="__main__":
    test_main()
