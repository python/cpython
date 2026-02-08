"""Tests comparing PyREPL's pure Python curses implementation with the standard curses module."""

import json
import os
import subprocess
import sys
import unittest
from test.support import requires, has_subprocess_support
from textwrap import dedent

# Only run these tests if curses is available
requires("curses")

try:
    import _curses
except ImportError:
    try:
        import curses as _curses
    except ImportError:
        _curses = None

from _pyrepl import terminfo


ABSENT_STRING = terminfo.ABSENT_STRING
CANCELLED_STRING = terminfo.CANCELLED_STRING


class TestCursesCompatibility(unittest.TestCase):
    """Test that PyREPL's curses implementation matches the standard curses behavior.

    Python's `curses` doesn't allow calling `setupterm()` again with a different
    $TERM in the same process, so we subprocess all `curses` tests to get correctly
    set up terminfo."""

    @classmethod
    def setUpClass(cls):
        if _curses is None:
            raise unittest.SkipTest(
                "`curses` capability provided to regrtest but `_curses` not importable"
            )

        if not has_subprocess_support:
            raise unittest.SkipTest("test module requires subprocess")

        # we need to ensure there's a terminfo database on the system and that
        # `infocmp` works
        cls.infocmp("dumb")

    def setUp(self):
        self.original_term = os.environ.get("TERM", None)

    def tearDown(self):
        if self.original_term is not None:
            os.environ["TERM"] = self.original_term
        elif "TERM" in os.environ:
            del os.environ["TERM"]

    @classmethod
    def infocmp(cls, term) -> list[str]:
        all_caps = []
        try:
            result = subprocess.run(
                ["infocmp", "-l1", term],
                capture_output=True,
                text=True,
                check=True,
            )
        except Exception:
            raise unittest.SkipTest("calling `infocmp` failed on the system")

        for line in result.stdout.splitlines():
            line = line.strip()
            if line.startswith("#"):
                if "terminfo" not in line and "termcap" in line:
                    # PyREPL terminfo doesn't parse termcap databases
                    raise unittest.SkipTest(
                        "curses using termcap.db: no terminfo database on"
                        " the system"
                    )
            elif "=" in line:
                cap_name = line.split("=")[0]
                all_caps.append(cap_name)

        return all_caps

    def test_setupterm_basic(self):
        """Test basic setupterm functionality."""
        # Test with explicit terminal type
        test_terms = ["xterm", "xterm-256color", "vt100", "ansi"]

        for term in test_terms:
            with self.subTest(term=term):
                ncurses_code = dedent(
                    f"""
                    import _curses
                    import json
                    try:
                        _curses.setupterm({repr(term)}, 1)
                        print(json.dumps({{"success": True}}))
                    except Exception as e:
                        print(json.dumps({{"success": False, "error": str(e)}}))
                    """
                )

                result = subprocess.run(
                    [sys.executable, "-c", ncurses_code],
                    capture_output=True,
                    text=True,
                )
                ncurses_data = json.loads(result.stdout)
                std_success = ncurses_data["success"]

                # Set up with PyREPL curses
                try:
                    terminfo.TermInfo(term, fallback=False)
                    pyrepl_success = True
                except Exception as e:
                    pyrepl_success = False
                    pyrepl_error = e

                # Both should succeed or both should fail
                if std_success:
                    self.assertTrue(
                        pyrepl_success,
                        f"Standard curses succeeded but PyREPL failed for {term}",
                    )
                else:
                    # If standard curses failed, PyREPL might still succeed with fallback
                    # This is acceptable as PyREPL has hardcoded fallbacks
                    pass

    def test_setupterm_none(self):
        """Test setupterm with None (uses TERM from environment)."""
        # Test with current TERM
        ncurses_code = dedent(
            """
            import _curses
            import json
            try:
                _curses.setupterm(None, 1)
                print(json.dumps({"success": True}))
            except Exception as e:
                print(json.dumps({"success": False, "error": str(e)}))
            """
        )

        result = subprocess.run(
            [sys.executable, "-c", ncurses_code],
            capture_output=True,
            text=True,
        )
        ncurses_data = json.loads(result.stdout)
        std_success = ncurses_data["success"]

        try:
            terminfo.TermInfo(None, fallback=False)
            pyrepl_success = True
        except Exception:
            pyrepl_success = False

        # Both should have same result
        if std_success:
            self.assertTrue(
                pyrepl_success,
                "Standard curses succeeded but PyREPL failed for None",
            )

    def test_tigetstr_common_capabilities(self):
        """Test tigetstr for common terminal capabilities."""
        # Test with a known terminal type
        term = "xterm"

        # Get ALL capabilities from infocmp
        all_caps = self.infocmp(term)

        ncurses_code = dedent(
            f"""
            import _curses
            import json
            _curses.setupterm({repr(term)}, 1)
            results = {{}}
            for cap in {repr(all_caps)}:
                try:
                    val = _curses.tigetstr(cap)
                    if val is None:
                        results[cap] = None
                    elif val == -1:
                        results[cap] = -1
                    else:
                        results[cap] = list(val)
                except BaseException:
                    results[cap] = "error"
            print(json.dumps(results))
            """
        )

        result = subprocess.run(
            [sys.executable, "-c", ncurses_code],
            capture_output=True,
            text=True,
        )
        self.assertEqual(
            result.returncode, 0, f"Failed to run ncurses: {result.stderr}"
        )

        ncurses_data = json.loads(result.stdout)

        ti = terminfo.TermInfo(term, fallback=False)

        # Test every single capability
        for cap in all_caps:
            if cap not in ncurses_data or ncurses_data[cap] == "error":
                continue

            with self.subTest(capability=cap):
                ncurses_val = ncurses_data[cap]
                if isinstance(ncurses_val, list):
                    ncurses_val = bytes(ncurses_val)

                pyrepl_val = ti.get(cap)

                self.assertEqual(
                    pyrepl_val,
                    ncurses_val,
                    f"Capability {cap}: ncurses={repr(ncurses_val)}, "
                    f"pyrepl={repr(pyrepl_val)}",
                )

    def test_tigetstr_input_types(self):
        """Test tigetstr with different input types."""
        term = "xterm"
        cap = "cup"

        # Test standard curses behavior with string in subprocess
        ncurses_code = dedent(
            f"""
            import _curses
            import json
            _curses.setupterm({repr(term)}, 1)

            # Test with string input
            try:
                std_str_result = _curses.tigetstr({repr(cap)})
                std_accepts_str = True
                if std_str_result is None:
                    std_str_val = None
                elif std_str_result == -1:
                    std_str_val = -1
                else:
                    std_str_val = list(std_str_result)
            except TypeError:
                std_accepts_str = False
                std_str_val = None

            print(json.dumps({{
                "accepts_str": std_accepts_str,
                "str_result": std_str_val
            }}))
            """
        )

        result = subprocess.run(
            [sys.executable, "-c", ncurses_code],
            capture_output=True,
            text=True,
        )
        ncurses_data = json.loads(result.stdout)

        # PyREPL setup
        ti = terminfo.TermInfo(term, fallback=False)

        # PyREPL behavior with string
        try:
            pyrepl_str_result = ti.get(cap)
            pyrepl_accepts_str = True
        except TypeError:
            pyrepl_accepts_str = False

        # PyREPL should also only accept strings for compatibility
        with self.assertRaises(TypeError):
            ti.get(cap.encode("ascii"))

        # Both should accept string input
        self.assertEqual(
            pyrepl_accepts_str,
            ncurses_data["accepts_str"],
            "PyREPL and standard curses should have same string handling",
        )
        self.assertTrue(
            pyrepl_accepts_str, "PyREPL should accept string input"
        )

    def test_tparm_basic(self):
        """Test basic tparm functionality."""
        term = "xterm"
        ti = terminfo.TermInfo(term, fallback=False)

        # Test cursor positioning (cup)
        cup = ti.get("cup")
        if cup and cup not in {ABSENT_STRING, CANCELLED_STRING}:
            # Test various parameter combinations
            test_cases = [
                (0, 0),  # Top-left
                (5, 10),  # Arbitrary position
                (23, 79),  # Bottom-right of standard terminal
                (999, 999),  # Large values
            ]

            # Get ncurses results in subprocess
            ncurses_code = dedent(
                f"""
                import _curses
                import json
                _curses.setupterm({repr(term)}, 1)

                # Get cup capability
                cup = _curses.tigetstr('cup')
                results = {{}}

                for row, col in {repr(test_cases)}:
                    try:
                        result = _curses.tparm(cup, row, col)
                        results[f"{{row}},{{col}}"] = list(result)
                    except Exception as e:
                        results[f"{{row}},{{col}}"] = {{"error": str(e)}}

                print(json.dumps(results))
                """
            )

            result = subprocess.run(
                [sys.executable, "-c", ncurses_code],
                capture_output=True,
                text=True,
            )
            self.assertEqual(
                result.returncode, 0, f"Failed to run ncurses: {result.stderr}"
            )
            ncurses_data = json.loads(result.stdout)

            for row, col in test_cases:
                with self.subTest(row=row, col=col):
                    # Standard curses tparm from subprocess
                    key = f"{row},{col}"
                    if (
                        isinstance(ncurses_data[key], dict)
                        and "error" in ncurses_data[key]
                    ):
                        self.fail(
                            f"ncurses tparm failed: {ncurses_data[key]['error']}"
                        )
                    std_result = bytes(ncurses_data[key])

                    # PyREPL curses tparm
                    pyrepl_result = terminfo.tparm(cup, row, col)

                    # Results should be identical
                    self.assertEqual(
                        pyrepl_result,
                        std_result,
                        f"tparm(cup, {row}, {col}): "
                        f"std={repr(std_result)}, pyrepl={repr(pyrepl_result)}",
                    )
        else:
            raise unittest.SkipTest(
                "test_tparm_basic() requires the `cup` capability"
            )

    def test_tparm_multiple_params(self):
        """Test tparm with capabilities using multiple parameters."""
        term = "xterm"
        ti = terminfo.TermInfo(term, fallback=False)

        # Test capabilities that take parameters
        param_caps = {
            "cub": 1,  # cursor_left with count
            "cuf": 1,  # cursor_right with count
            "cuu": 1,  # cursor_up with count
            "cud": 1,  # cursor_down with count
            "dch": 1,  # delete_character with count
            "ich": 1,  # insert_character with count
        }

        # Get all capabilities from PyREPL first
        pyrepl_caps = {}
        for cap in param_caps:
            cap_value = ti.get(cap)
            if cap_value and cap_value not in {
                ABSENT_STRING,
                CANCELLED_STRING,
            }:
                pyrepl_caps[cap] = cap_value

        if not pyrepl_caps:
            self.skipTest("No parametrized capabilities found")

        # Get ncurses results in subprocess
        ncurses_code = dedent(
            f"""
            import _curses
            import json
            _curses.setupterm({repr(term)}, 1)

            param_caps = {repr(param_caps)}
            test_values = [1, 5, 10, 99]
            results = {{}}

            for cap in param_caps:
                cap_value = _curses.tigetstr(cap)
                if cap_value and cap_value != -1:
                    for value in test_values:
                        try:
                            result = _curses.tparm(cap_value, value)
                            results[f"{{cap}},{{value}}"] = list(result)
                        except Exception as e:
                            results[f"{{cap}},{{value}}"] = {{"error": str(e)}}

            print(json.dumps(results))
            """
        )

        result = subprocess.run(
            [sys.executable, "-c", ncurses_code],
            capture_output=True,
            text=True,
        )
        self.assertEqual(
            result.returncode, 0, f"Failed to run ncurses: {result.stderr}"
        )
        ncurses_data = json.loads(result.stdout)

        for cap, cap_value in pyrepl_caps.items():
            with self.subTest(capability=cap):
                # Test with different parameter values
                for value in [1, 5, 10, 99]:
                    key = f"{cap},{value}"
                    if key in ncurses_data:
                        if (
                            isinstance(ncurses_data[key], dict)
                            and "error" in ncurses_data[key]
                        ):
                            self.fail(
                                f"ncurses tparm failed: {ncurses_data[key]['error']}"
                            )
                        std_result = bytes(ncurses_data[key])

                        pyrepl_result = terminfo.tparm(cap_value, value)
                        self.assertEqual(
                            pyrepl_result,
                            std_result,
                            f"tparm({cap}, {value}): "
                            f"std={repr(std_result)}, pyrepl={repr(pyrepl_result)}",
                        )

    def test_tparm_null_handling(self):
        """Test tparm with None/null input."""
        term = "xterm"

        ncurses_code = dedent(
            f"""
            import _curses
            import json
            _curses.setupterm({repr(term)}, 1)

            # Test with None
            try:
                _curses.tparm(None)
                raises_typeerror = False
            except TypeError:
                raises_typeerror = True
            except Exception as e:
                raises_typeerror = False
                error_type = type(e).__name__

            print(json.dumps({{"raises_typeerror": raises_typeerror}}))
            """
        )

        result = subprocess.run(
            [sys.executable, "-c", ncurses_code],
            capture_output=True,
            text=True,
        )
        ncurses_data = json.loads(result.stdout)

        # PyREPL setup
        ti = terminfo.TermInfo(term, fallback=False)

        # Test with None - both should raise TypeError
        if ncurses_data["raises_typeerror"]:
            with self.assertRaises(TypeError):
                terminfo.tparm(None)
        else:
            # If ncurses doesn't raise TypeError, PyREPL shouldn't either
            try:
                terminfo.tparm(None)
            except TypeError:
                self.fail("PyREPL raised TypeError but ncurses did not")

    def test_special_terminals(self):
        """Test with special terminal types."""
        special_terms = [
            "dumb",  # Minimal terminal
            "unknown",  # Should fall back to defaults
            "linux",  # Linux console
            "screen",  # GNU Screen
            "tmux",  # tmux
        ]

        # Get all string capabilities from ncurses
        for term in special_terms:
            with self.subTest(term=term):
                all_caps = self.infocmp(term)
                ncurses_code = dedent(
                    f"""
                    import _curses
                    import json
                    import sys

                    try:
                        _curses.setupterm({repr(term)}, 1)
                        results = {{}}
                        for cap in {repr(all_caps)}:
                            try:
                                val = _curses.tigetstr(cap)
                                if val is None:
                                    results[cap] = None
                                elif val == -1:
                                    results[cap] = -1
                                else:
                                    # Convert bytes to list of ints for JSON
                                    results[cap] = list(val)
                            except BaseException:
                                results[cap] = "error"
                        print(json.dumps(results))
                    except Exception as e:
                        print(json.dumps({{"error": str(e)}}))
                    """
                )

                # Get ncurses results
                result = subprocess.run(
                    [sys.executable, "-c", ncurses_code],
                    capture_output=True,
                    text=True,
                )
                if result.returncode != 0:
                    self.fail(
                        f"Failed to get ncurses data for {term}: {result.stderr}"
                    )

                try:
                    ncurses_data = json.loads(result.stdout)
                except json.JSONDecodeError:
                    self.fail(
                        f"Failed to parse ncurses output for {term}: {result.stdout}"
                    )

                if "error" in ncurses_data and len(ncurses_data) == 1:
                    # ncurses failed to setup this terminal
                    # PyREPL should still work with fallback
                    ti = terminfo.TermInfo(term, fallback=True)
                    continue

                ti = terminfo.TermInfo(term, fallback=False)

                # Compare all capabilities
                for cap in all_caps:
                    if cap not in ncurses_data:
                        continue

                    with self.subTest(term=term, capability=cap):
                        ncurses_val = ncurses_data[cap]
                        if isinstance(ncurses_val, list):
                            # Convert back to bytes
                            ncurses_val = bytes(ncurses_val)

                        pyrepl_val = ti.get(cap)

                        # Both should return the same value
                        self.assertEqual(
                            pyrepl_val,
                            ncurses_val,
                            f"Capability {cap} for {term}: "
                            f"ncurses={repr(ncurses_val)}, "
                            f"pyrepl={repr(pyrepl_val)}",
                        )

    def test_terminfo_fallback(self):
        """Test that PyREPL falls back gracefully when terminfo is not found."""
        # Use a non-existent terminal type
        fake_term = "nonexistent-terminal-type-12345"

        # Check if standard curses can setup this terminal in subprocess
        ncurses_code = dedent(
            f"""
            import _curses
            import json
            try:
                _curses.setupterm({repr(fake_term)}, 1)
                print(json.dumps({{"success": True}}))
            except _curses.error:
                print(json.dumps({{"success": False, "error": "curses.error"}}))
            except Exception as e:
                print(json.dumps({{"success": False, "error": str(e)}}))
            """
        )

        result = subprocess.run(
            [sys.executable, "-c", ncurses_code],
            capture_output=True,
            text=True,
        )
        ncurses_data = json.loads(result.stdout)

        if ncurses_data["success"]:
            # If it succeeded, skip this test as we can't test fallback
            self.skipTest(
                f"System unexpectedly has terminfo for '{fake_term}'"
            )

        # PyREPL should succeed with fallback
        try:
            ti = terminfo.TermInfo(fake_term, fallback=True)
            pyrepl_ok = True
        except Exception:
            pyrepl_ok = False

        self.assertTrue(
            pyrepl_ok, "PyREPL should fall back for unknown terminals"
        )

        # Should still be able to get basic capabilities
        bel = ti.get("bel")
        self.assertIsNotNone(
            bel, "PyREPL should provide basic capabilities after fallback"
        )

    def test_invalid_terminal_names(self):
        cases = [
            (42, TypeError),
            ("", ValueError),
            ("w\x00t", ValueError),
            (f"..{os.sep}name", ValueError),
        ]

        for term, exc in cases:
            with self.subTest(term=term):
                with self.assertRaises(exc):
                    terminfo._validate_terminal_name_or_raise(term)
