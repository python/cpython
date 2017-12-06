# Tests the attempted automatic coercion of the C locale to a UTF-8 locale

import unittest
import locale
import os
import sys
import sysconfig
import shutil
from collections import namedtuple

import test.support
from test.support.script_helper import (
    run_python_until_end,
    interpreter_requires_environment,
)

# Set our expectation for the default encoding used in the C locale
# for the filesystem encoding and the standard streams

# While most *nix platforms default to ASCII in the C locale, some use a
# different encoding.
if sys.platform.startswith("aix"):
    C_LOCALE_STREAM_ENCODING = "iso8859-1"
elif test.support.is_android:
    C_LOCALE_STREAM_ENCODING = "utf-8"
else:
    C_LOCALE_STREAM_ENCODING = "ascii"

# FS encoding is UTF-8 on macOS, other *nix platforms use the locale encoding
if sys.platform == "darwin":
    C_LOCALE_FS_ENCODING = "utf-8"
else:
    C_LOCALE_FS_ENCODING = C_LOCALE_STREAM_ENCODING

# Note that the above is probably still wrong in some cases, such as:
# * Windows when PYTHONLEGACYWINDOWSFSENCODING is set
# * AIX and any other platforms that use latin-1 in the C locale
#
# Options for dealing with this:
# * Don't set PYTHON_COERCE_C_LOCALE on such platforms (e.g. Windows doesn't)
# * Fix the test expectations to match the actual platform behaviour

# In order to get the warning messages to match up as expected, the candidate
# order here must much the target locale order in Python/pylifecycle.c
_C_UTF8_LOCALES = ("C.UTF-8", "C.utf8", "UTF-8")

# There's no reliable cross-platform way of checking locale alias
# lists, so the only way of knowing which of these locales will work
# is to try them with locale.setlocale(). We do that in a subprocess
# to avoid altering the locale of the test runner.
#
# If the relevant locale module attributes exist, and we're not on a platform
# where we expect it to always succeed, we also check that
# `locale.nl_langinfo(locale.CODESET)` works, as if it fails, the interpreter
# will skip locale coercion for that particular target locale
_check_nl_langinfo_CODESET = bool(
    sys.platform not in ("darwin", "linux") and
    hasattr(locale, "nl_langinfo") and
    hasattr(locale, "CODESET")
)

def _set_locale_in_subprocess(locale_name):
    cmd_fmt = "import locale; print(locale.setlocale(locale.LC_CTYPE, '{}'))"
    if _check_nl_langinfo_CODESET:
        # If there's no valid CODESET, we expect coercion to be skipped
        cmd_fmt += "; import sys; sys.exit(not locale.nl_langinfo(locale.CODESET))"
    cmd = cmd_fmt.format(locale_name)
    result, py_cmd = run_python_until_end("-c", cmd, __isolated=True)
    return result.rc == 0



_fields = "fsencoding stdin_info stdout_info stderr_info lang lc_ctype lc_all"
_EncodingDetails = namedtuple("EncodingDetails", _fields)

class EncodingDetails(_EncodingDetails):
    # XXX (ncoghlan): Using JSON for child state reporting may be less fragile
    CHILD_PROCESS_SCRIPT = ";".join([
        "import sys, os",
        "print(sys.getfilesystemencoding())",
        "print(sys.stdin.encoding + ':' + sys.stdin.errors)",
        "print(sys.stdout.encoding + ':' + sys.stdout.errors)",
        "print(sys.stderr.encoding + ':' + sys.stderr.errors)",
        "print(os.environ.get('LANG', 'not set'))",
        "print(os.environ.get('LC_CTYPE', 'not set'))",
        "print(os.environ.get('LC_ALL', 'not set'))",
    ])

    @classmethod
    def get_expected_details(cls, coercion_expected, fs_encoding, stream_encoding, env_vars):
        """Returns expected child process details for a given encoding"""
        _stream = stream_encoding + ":{}"
        # stdin and stdout should use surrogateescape either because the
        # coercion triggered, or because the C locale was detected
        stream_info = 2*[_stream.format("surrogateescape")]
        # stderr should always use backslashreplace
        stream_info.append(_stream.format("backslashreplace"))
        expected_lang = env_vars.get("LANG", "not set").lower()
        if coercion_expected:
            expected_lc_ctype = CLI_COERCION_TARGET.lower()
        else:
            expected_lc_ctype = env_vars.get("LC_CTYPE", "not set").lower()
        expected_lc_all = env_vars.get("LC_ALL", "not set").lower()
        env_info = expected_lang, expected_lc_ctype, expected_lc_all
        return dict(cls(fs_encoding, *stream_info, *env_info)._asdict())

    @staticmethod
    def _handle_output_variations(data):
        """Adjust the output to handle platform specific idiosyncrasies

        * Some platforms report ASCII as ANSI_X3.4-1968
        * Some platforms report ASCII as US-ASCII
        * Some platforms report UTF-8 instead of utf-8
        """
        data = data.replace(b"ANSI_X3.4-1968", b"ascii")
        data = data.replace(b"US-ASCII", b"ascii")
        data = data.lower()
        return data

    @classmethod
    def get_child_details(cls, env_vars):
        """Retrieves fsencoding and standard stream details from a child process

        Returns (encoding_details, stderr_lines):

        - encoding_details: EncodingDetails for eager decoding
        - stderr_lines: result of calling splitlines() on the stderr output

        The child is run in isolated mode if the current interpreter supports
        that.
        """
        result, py_cmd = run_python_until_end(
            "-c", cls.CHILD_PROCESS_SCRIPT,
            __isolated=True,
            **env_vars
        )
        if not result.rc == 0:
            result.fail(py_cmd)
        # All subprocess outputs in this test case should be pure ASCII
        adjusted_output = cls._handle_output_variations(result.out)
        stdout_lines = adjusted_output.decode("ascii").splitlines()
        child_encoding_details = dict(cls(*stdout_lines)._asdict())
        stderr_lines = result.err.decode("ascii").rstrip().splitlines()
        return child_encoding_details, stderr_lines


# Details of the shared library warning emitted at runtime
LEGACY_LOCALE_WARNING = (
    "Python runtime initialized with LC_CTYPE=C (a locale with default ASCII "
    "encoding), which may cause Unicode compatibility problems. Using C.UTF-8, "
    "C.utf8, or UTF-8 (if available) as alternative Unicode-compatible "
    "locales is recommended."
)

# Details of the CLI locale coercion warning emitted at runtime
CLI_COERCION_WARNING_FMT = (
    "Python detected LC_CTYPE=C: LC_CTYPE coerced to {} (set another locale "
    "or PYTHONCOERCECLOCALE=0 to disable this locale coercion behavior)."
)


AVAILABLE_TARGETS = None
CLI_COERCION_TARGET = None
CLI_COERCION_WARNING = None

def setUpModule():
    global AVAILABLE_TARGETS
    global CLI_COERCION_TARGET
    global CLI_COERCION_WARNING

    if AVAILABLE_TARGETS is not None:
        # initialization already done
        return
    AVAILABLE_TARGETS = []

    # Find the target locales available in the current system
    for target_locale in _C_UTF8_LOCALES:
        if _set_locale_in_subprocess(target_locale):
            AVAILABLE_TARGETS.append(target_locale)

    if AVAILABLE_TARGETS:
        # Coercion is expected to use the first available target locale
        CLI_COERCION_TARGET = AVAILABLE_TARGETS[0]
        CLI_COERCION_WARNING = CLI_COERCION_WARNING_FMT.format(CLI_COERCION_TARGET)


class _LocaleHandlingTestCase(unittest.TestCase):
    # Base class to check expected locale handling behaviour

    def _check_child_encoding_details(self,
                                      env_vars,
                                      expected_fs_encoding,
                                      expected_stream_encoding,
                                      expected_warnings,
                                      coercion_expected):
        """Check the C locale handling for the given process environment

        Parameters:
            expected_fs_encoding: expected sys.getfilesystemencoding() result
            expected_stream_encoding: expected encoding for standard streams
            expected_warning: stderr output to expect (if any)
        """
        result = EncodingDetails.get_child_details(env_vars)
        encoding_details, stderr_lines = result
        expected_details = EncodingDetails.get_expected_details(
            coercion_expected,
            expected_fs_encoding,
            expected_stream_encoding,
            env_vars
        )
        self.assertEqual(encoding_details, expected_details)
        if expected_warnings is None:
            expected_warnings = []
        self.assertEqual(stderr_lines, expected_warnings)


class LocaleConfigurationTests(_LocaleHandlingTestCase):
    # Test explicit external configuration via the process environment

    def setUpClass():
        # This relies on setupModule() having been run, so it can't be
        # handled via the @unittest.skipUnless decorator
        if not AVAILABLE_TARGETS:
            raise unittest.SkipTest("No C-with-UTF-8 locale available")

    def test_external_target_locale_configuration(self):

        # Explicitly setting a target locale should give the same behaviour as
        # is seen when implicitly coercing to that target locale
        self.maxDiff = None

        expected_fs_encoding = "utf-8"
        expected_stream_encoding = "utf-8"

        base_var_dict = {
            "LANG": "",
            "LC_CTYPE": "",
            "LC_ALL": "",
        }
        for env_var in ("LANG", "LC_CTYPE"):
            for locale_to_set in AVAILABLE_TARGETS:
                # XXX (ncoghlan): LANG=UTF-8 doesn't appear to work as
                #                 expected, so skip that combination for now
                # See https://bugs.python.org/issue30672 for discussion
                if env_var == "LANG" and locale_to_set == "UTF-8":
                    continue

                with self.subTest(env_var=env_var,
                                  configured_locale=locale_to_set):
                    var_dict = base_var_dict.copy()
                    var_dict[env_var] = locale_to_set
                    self._check_child_encoding_details(var_dict,
                                                       expected_fs_encoding,
                                                       expected_stream_encoding,
                                                       expected_warnings=None,
                                                       coercion_expected=False)



@test.support.cpython_only
@unittest.skipUnless(sysconfig.get_config_var("PY_COERCE_C_LOCALE"),
                     "C locale coercion disabled at build time")
class LocaleCoercionTests(_LocaleHandlingTestCase):
    # Test implicit reconfiguration of the environment during CLI startup

    def _check_c_locale_coercion(self,
                                 fs_encoding, stream_encoding,
                                 coerce_c_locale,
                                 expected_warnings=None,
                                 coercion_expected=True,
                                 **extra_vars):
        """Check the C locale handling for various configurations

        Parameters:
            fs_encoding: expected sys.getfilesystemencoding() result
            stream_encoding: expected encoding for standard streams
            coerce_c_locale: setting to use for PYTHONCOERCECLOCALE
              None: don't set the variable at all
              str: the value set in the child's environment
            expected_warnings: expected warning lines on stderr
            extra_vars: additional environment variables to set in subprocess
        """
        self.maxDiff = None

        if not AVAILABLE_TARGETS:
            # Locale coercion is disabled when there aren't any target locales
            fs_encoding = C_LOCALE_FS_ENCODING
            stream_encoding = C_LOCALE_STREAM_ENCODING
            coercion_expected = False
            if expected_warnings:
                expected_warnings = [LEGACY_LOCALE_WARNING]

        base_var_dict = {
            "LANG": "",
            "LC_CTYPE": "",
            "LC_ALL": "",
        }
        base_var_dict.update(extra_vars)
        for env_var in ("LANG", "LC_CTYPE"):
            for locale_to_set in ("", "C", "POSIX", "invalid.ascii"):
                # XXX (ncoghlan): *BSD platforms don't behave as expected in the
                #                 POSIX locale, so we skip that for now
                # See https://bugs.python.org/issue30672 for discussion
                if locale_to_set == "POSIX":
                    continue

                # Platforms using UTF-8 in the C locale do not print
                # CLI_COERCION_WARNING when all the locale envt variables are
                # not set or set to the empty string.
                _expected_warnings = expected_warnings
                for _env_var in base_var_dict:
                    if base_var_dict[_env_var]:
                        break
                else:
                    if (C_LOCALE_STREAM_ENCODING == "utf-8" and
                           locale_to_set == "" and coerce_c_locale == "warn"):
                        _expected_warnings = None

                with self.subTest(env_var=env_var,
                                  nominal_locale=locale_to_set,
                                  PYTHONCOERCECLOCALE=coerce_c_locale):
                    var_dict = base_var_dict.copy()
                    var_dict[env_var] = locale_to_set
                    if coerce_c_locale is not None:
                        var_dict["PYTHONCOERCECLOCALE"] = coerce_c_locale
                    # Check behaviour on successful coercion
                    self._check_child_encoding_details(var_dict,
                                                       fs_encoding,
                                                       stream_encoding,
                                                       _expected_warnings,
                                                       coercion_expected)

    def test_test_PYTHONCOERCECLOCALE_not_set(self):
        # This should coerce to the first available target locale by default
        self._check_c_locale_coercion("utf-8", "utf-8", coerce_c_locale=None)

    def test_PYTHONCOERCECLOCALE_not_zero(self):
        # *Any* string other than "0" is considered "set" for our purposes
        # and hence should result in the locale coercion being enabled
        for setting in ("", "1", "true", "false"):
            self._check_c_locale_coercion("utf-8", "utf-8", coerce_c_locale=setting)

    def test_PYTHONCOERCECLOCALE_set_to_warn(self):
        # PYTHONCOERCECLOCALE=warn enables runtime warnings for legacy locales
        self._check_c_locale_coercion("utf-8", "utf-8",
                                      coerce_c_locale="warn",
                                      expected_warnings=[CLI_COERCION_WARNING])


    def test_PYTHONCOERCECLOCALE_set_to_zero(self):
        # The setting "0" should result in the locale coercion being disabled
        self._check_c_locale_coercion(C_LOCALE_FS_ENCODING,
                                      C_LOCALE_STREAM_ENCODING,
                                      coerce_c_locale="0",
                                      coercion_expected=False)
        # Setting LC_ALL=C shouldn't make any difference to the behaviour
        self._check_c_locale_coercion(C_LOCALE_FS_ENCODING,
                                      C_LOCALE_STREAM_ENCODING,
                                      coerce_c_locale="0",
                                      LC_ALL="C",
                                      coercion_expected=False)

    def test_LC_ALL_set_to_C(self):
        # Setting LC_ALL should render the locale coercion ineffective
        self._check_c_locale_coercion(C_LOCALE_FS_ENCODING,
                                      C_LOCALE_STREAM_ENCODING,
                                      coerce_c_locale=None,
                                      LC_ALL="C",
                                      coercion_expected=False)
        # And result in a warning about a lack of locale compatibility
        self._check_c_locale_coercion(C_LOCALE_FS_ENCODING,
                                      C_LOCALE_STREAM_ENCODING,
                                      coerce_c_locale="warn",
                                      LC_ALL="C",
                                      expected_warnings=[LEGACY_LOCALE_WARNING],
                                      coercion_expected=False)

def test_main():
    test.support.run_unittest(
        LocaleConfigurationTests,
        LocaleCoercionTests
    )
    test.support.reap_children()

if __name__ == "__main__":
    test_main()
