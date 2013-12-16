# Ridiculously simple test of the winsound module for Windows.

import unittest
from test import support
support.requires('audio')
import time
import os
import subprocess

winsound = support.import_module('winsound')
ctypes = support.import_module('ctypes')
import winreg

def has_sound(sound):
    """Find out if a particular event is configured with a default sound"""
    try:
        # Ask the mixer API for the number of devices it knows about.
        # When there are no devices, PlaySound will fail.
        if ctypes.windll.winmm.mixerGetNumDevs() == 0:
            return False

        key = winreg.OpenKeyEx(winreg.HKEY_CURRENT_USER,
                "AppEvents\Schemes\Apps\.Default\{0}\.Default".format(sound))
        return winreg.EnumValue(key, 0)[1] != ""
    except OSError:
        return False

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
        self._beep(37, 75)
        self._beep(32767, 75)

    def test_increasingfrequency(self):
        for i in range(100, 2000, 100):
            self._beep(i, 75)

    def _beep(self, *args):
        # these tests used to use _have_soundcard(), but it's quite
        # possible to have a soundcard, and yet have the beep driver
        # disabled. So basically, we have no way of knowing whether
        # a beep should be produced or not, so currently if these
        # tests fail we're ignoring them
        #
        # XXX the right fix for this is to define something like
        # _have_enabled_beep_driver() and use that instead of the
        # try/except below
        try:
            winsound.Beep(*args)
        except RuntimeError:
            pass

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

    @unittest.skipUnless(has_sound("SystemAsterisk"),
                         "No default SystemAsterisk")
    def test_alias_asterisk(self):
        if _have_soundcard():
            winsound.PlaySound('SystemAsterisk', winsound.SND_ALIAS)
        else:
            self.assertRaises(
                RuntimeError,
                winsound.PlaySound,
                'SystemAsterisk', winsound.SND_ALIAS
            )

    @unittest.skipUnless(has_sound("SystemExclamation"),
                         "No default SystemExclamation")
    def test_alias_exclamation(self):
        if _have_soundcard():
            winsound.PlaySound('SystemExclamation', winsound.SND_ALIAS)
        else:
            self.assertRaises(
                RuntimeError,
                winsound.PlaySound,
                'SystemExclamation', winsound.SND_ALIAS
            )

    @unittest.skipUnless(has_sound("SystemExit"), "No default SystemExit")
    def test_alias_exit(self):
        if _have_soundcard():
            winsound.PlaySound('SystemExit', winsound.SND_ALIAS)
        else:
            self.assertRaises(
                RuntimeError,
                winsound.PlaySound,
                'SystemExit', winsound.SND_ALIAS
            )

    @unittest.skipUnless(has_sound("SystemHand"), "No default SystemHand")
    def test_alias_hand(self):
        if _have_soundcard():
            winsound.PlaySound('SystemHand', winsound.SND_ALIAS)
        else:
            self.assertRaises(
                RuntimeError,
                winsound.PlaySound,
                'SystemHand', winsound.SND_ALIAS
            )

    @unittest.skipUnless(has_sound("SystemQuestion"),
                         "No default SystemQuestion")
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
        # In the absense of the ability to tell if a sound was actually
        # played, this test has two acceptable outcomes: success (no error,
        # sound was theoretically played; although as issue #19987 shows
        # a box without a soundcard can "succeed") or RuntimeError.  Any
        # other error is a failure.
        try:
            winsound.PlaySound('!"$%&/(#+*', winsound.SND_ALIAS)
        except RuntimeError:
            pass

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
            # Issue 8367: PlaySound(None, winsound.SND_PURGE)
            # does not raise on systems without a sound card.
            pass


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
        p.stdout.close()
    return __have_soundcard_cache


def test_main():
    support.run_unittest(BeepTest, MessageBeepTest, PlaySoundTest)

if __name__=="__main__":
    test_main()
