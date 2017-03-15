# Tests the attempted automatic coercion of the C locale to a UTF-8 locale

import unittest
import os
import sys
import sysconfig
import shutil
import subprocess
import test.support
from test.support.script_helper import (
    run_python_until_end,
    interpreter_requires_environment,
)

# In order to get the warning messages to match up as expected, the candidate
# order here must much the target locale order in Programs/python.c
_C_UTF8_LOCALES = (
    # Entries: (Target locale, target category, expected env var updates)
    ("C.UTF-8", "LC_ALL", "LC_ALL & LANG"),
    ("C.utf8", "LC_ALL", "LC_ALL & LANG"),
    ("UTF-8", "LC_CTYPE", "LC_CTYPE"),
)

# There's no reliable cross-platform way of checking locale alias
# lists, so the only way of knowing which of these locales will work
# is to try them with locale.setlocale(). We do that in a subprocess
# to avoid altering the locale of the test runner.
def _set_locale_in_subprocess(locale_name, category):
    cmd_fmt = "import locale; print(locale.setlocale(locale.{}, '{}'))"
    cmd = cmd_fmt.format(category, locale_name)
    result, py_cmd = run_python_until_end("-c", cmd, __isolated=True)
    return result.rc == 0

# Details of the CLI warning emitted at runtime
CLI_COERCION_WARNING_FMT = (
    "Python detected LC_CTYPE=C: {} coerced to {} (set another locale "
    "or PYTHONCOERCECLOCALE=0 to disable this locale coercion behaviour)."
)

@test.support.cpython_only
@unittest.skipUnless(sysconfig.get_config_var("PY_COERCE_C_LOCALE"),
                     "C locale coercion disabled at build time")
class LocaleOverrideTests(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        for target_locale, target_category, env_updates in _C_UTF8_LOCALES:
            if _set_locale_in_subprocess(target_locale, target_category):
                break
        else:
            raise unittest.SkipTest("No C-with-UTF-8 locale available")
        cls.EXPECTED_COERCION_WARNING = CLI_COERCION_WARNING_FMT.format(
            env_updates, target_locale
        )

    def _get_child_fsencoding(self, env_vars):
        """Retrieves sys.getfilesystemencoding() from a child process

        Returns (fsencoding, stderr_lines):

        - fsencoding: a lowercase str value with the child's fsencoding
        - stderr_lines: result of calling splitlines() on the stderr output

        The child is run in isolated mode if the current interpreter supports
        that.
        """
        cmd = "import sys; print(sys.getfilesystemencoding().lower())"
        result, py_cmd = run_python_until_end(
            "-c", cmd,
            __isolated=True,
            **env_vars
        )
        if not result.rc == 0:
            result.fail(py_cmd)
        # All subprocess outputs in this test case should be pure ASCII
        child_fsencoding = result.out.decode("ascii").rstrip()
        child_stderr_lines = result.err.decode("ascii").rstrip().splitlines()
        return child_fsencoding, child_stderr_lines


    def test_C_utf8_locale(self):
        # Ensure the C.UTF-8 locale is accepted entirely without complaint
        base_var_dict = {
            "LANG": "",
            "LC_CTYPE": "",
            "LC_ALL": "",
        }
        for env_var in base_var_dict:
            with self.subTest(env_var=env_var):
                var_dict = base_var_dict.copy()
                var_dict[env_var] = "C.UTF-8"
                fsencoding, stderr_lines = self._get_child_fsencoding(var_dict)
                self.assertEqual(fsencoding, "utf-8")
                self.assertFalse(stderr_lines)


    def _check_c_locale_coercion(self, expected_fsencoding, coerce_c_locale):
        """Check the handling of the C locale for various configurations

        Parameters:
            expected_fsencoding: the encoding the child is expected to report
            allow_c_locale: setting to use for PYTHONALLOWCLOCALE
              None: don't set the variable at all
              str: the value set in the child's environment
        """
        expected_warning = []
        if coerce_c_locale != "0":
            # Check C locale is coerced with a warning on stderr
            expected_warning.append(self.EXPECTED_COERCION_WARNING)
        base_var_dict = {
            "LANG": "",
            "LC_CTYPE": "",
            "LC_ALL": "",
        }
        for env_var in base_var_dict:
            for locale_to_set in ("", "C", "POSIX", "invalid.ascii"):
                with self.subTest(env_var=env_var,
                                  nominal_locale=locale_to_set,
                                  PYTHONCOERCECLOCALE=coerce_c_locale):
                    var_dict = base_var_dict.copy()
                    var_dict[env_var] = locale_to_set
                    if coerce_c_locale is not None:
                        var_dict["PYTHONCOERCECLOCALE"] = coerce_c_locale
                    fsencoding, stderr_lines = self._get_child_fsencoding(var_dict)
                    self.assertEqual(fsencoding, expected_fsencoding)
                    self.assertEqual(stderr_lines, expected_warning)


    def test_test_PYTHONCOERCECLOCALE_not_set(self):
        # This should coerce to the C.UTF-8 locale by default
        self._check_c_locale_coercion("utf-8", coerce_c_locale=None)

    def test_PYTHONCOERCECLOCALE_not_zero(self):
        # *Any* string other that "0" is considered "set" for our purposes
        # and hence should result in the locale coercion being enabled
        for setting in ("", "1", "true", "false"):
            self._check_c_locale_coercion("utf-8", coerce_c_locale=setting)

    def test_PYTHONCOERCECLOCALE_set_to_zero(self):
        # The setting "0" should result in the locale coercion being disabled
        self._check_c_locale_coercion("ascii", coerce_c_locale="0")


# Details of the shared library warning emitted at runtime
LIBRARY_C_LOCALE_WARNING = (
    "Python runtime initialized with LC_CTYPE=C (a locale with default ASCII "
    "encoding), which may cause Unicode compatibility problems. Using C.UTF-8, "
    "C.utf8, or UTF-8 (if available) as alternative Unicode-compatible "
    "locales is recommended.\n"
)

@unittest.skipUnless(sysconfig.get_config_var("PY_WARN_ON_C_LOCALE"),
                     "C locale runtime warning disabled at build time")
class EmbeddingTests(unittest.TestCase):
    def setUp(self):
        here = os.path.abspath(__file__)
        basepath = os.path.dirname(os.path.dirname(os.path.dirname(here)))
        self.test_exe = exe = os.path.join(basepath, "Programs", "_testembed")
        if not os.path.exists(exe):
            self.skipTest("%r doesn't exist" % exe)
        # This is needed otherwise we get a fatal error:
        # "Py_Initialize: Unable to get the locale encoding
        # LookupError: no codec search functions registered: can't find encoding"
        self.addCleanup(os.chdir, os.getcwd())
        os.chdir(basepath)

    def run_embedded_interpreter(self, *args):
        """Runs a test in the embedded interpreter"""
        cmd = [self.test_exe]
        cmd.extend(args)
        p = subprocess.Popen(cmd,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE,
                             universal_newlines=True)
        (out, err) = p.communicate()
        self.assertEqual(p.returncode, 0,
                         "bad returncode %d, stderr is %r" %
                         (p.returncode, err))
        return out, err

    def test_library_c_locale_warning(self):
        # Checks forced configuration of embedded interpreter IO streams
        out, err = self.run_embedded_interpreter("c_locale_warning")
        if test.support.verbose > 1:
            print()
            print(out)
            print(err)
        self.assertEqual(out, "")
        self.assertEqual(err, LIBRARY_C_LOCALE_WARNING)

def test_main():
    test.support.run_unittest(LocaleOverrideTests, EmbeddingTests)
    test.support.reap_children()

if __name__ == "__main__":
    test_main()
