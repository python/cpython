#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["pytest"]
# ///
"""
Tests for pyconf.py — run with:  uv run Tools/configure/test_pyconf.py
"""

from __future__ import annotations

import sys
import os
import pytest

# Ensure the module is importable regardless of cwd
sys.path.insert(0, os.path.dirname(__file__))
import pyconf


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _reset():
    """Reset all mutable module-level state between tests."""
    pyconf._options.clear()
    pyconf._env_vars.clear()
    pyconf._original_config_args.clear()
    pyconf.defines.clear()
    pyconf.substs.clear()
    pyconf.env.clear()
    pyconf.cache.clear()


@pytest.fixture(autouse=True)
def reset_state():
    _reset()
    yield
    _reset()


# ---------------------------------------------------------------------------
# arg_enable() / arg_with() + OptionSpec
# ---------------------------------------------------------------------------


class TestArgEnable:
    def test_enable_forms(self, monkeypatch):
        monkeypatch.setattr(
            sys,
            "argv",
            [
                "configure",
                "--enable-foo",
                "--disable-foo",
                "--enable-bar=xyz",
                "--disable-baz=custom",
            ],
        )
        foo = pyconf.arg_enable("foo")
        bar = pyconf.arg_enable("bar")
        baz = pyconf.arg_enable("baz")
        assert foo._raw == "no"
        assert bar._raw == "xyz"
        assert baz._raw == "custom"

    def test_last_occurrence_wins(self, monkeypatch):
        monkeypatch.setattr(
            sys,
            "argv",
            ["configure", "--enable-foo", "--disable-foo", "--enable-foo"],
        )
        opt = pyconf.arg_enable("foo")
        assert opt._raw == "yes"

    def test_hyphen_underscore_equivalent(self, monkeypatch):
        monkeypatch.setattr(sys, "argv", ["configure", "--enable-trace_refs"])
        opt = pyconf.arg_enable("trace-refs")
        assert opt._raw == "yes"

    def test_subtle_disable_style_case_enable_gil_custom(self, monkeypatch):
        monkeypatch.setattr(sys, "argv", ["configure", "--enable-gil=foo"])
        gil = pyconf.arg_enable("gil", display="--disable-gil", false_is="yes")
        assert gil._raw == "foo"
        assert gil.process_bool() is True


class TestArgWith:
    def test_with_without_forms(self, monkeypatch):
        monkeypatch.setattr(
            sys,
            "argv",
            [
                "configure",
                "--with-ssl",
                "--without-zlib",
                "--with-hash-algorithm=siphash24",
                "--without-openssl=forced",
            ],
        )
        ssl = pyconf.arg_with("ssl")
        zlib = pyconf.arg_with("zlib")
        hash_algo = pyconf.arg_with("hash-algorithm")
        openssl = pyconf.arg_with("openssl")
        assert ssl._raw == "yes"
        assert zlib._raw == "no"
        assert hash_algo._raw == "siphash24"
        assert openssl._raw == "forced"


class TestOptionSpec:
    def test_helpers(self, monkeypatch):
        monkeypatch.setattr(sys, "argv", ["configure", "--with-foo=value"])
        opt = pyconf.arg_with("foo", false_is="no")
        assert opt.given is True
        assert opt.is_yes() is False
        assert opt.is_no() is False
        assert opt.value_or("fallback") == "value"

    def test_value_property_given(self, monkeypatch):
        monkeypatch.setattr(sys, "argv", ["configure", "--with-foo=bar"])
        opt = pyconf.arg_with("foo", default="fallback")
        assert opt.value == "bar"

    def test_value_property_not_given(self, monkeypatch):
        monkeypatch.setattr(sys, "argv", ["configure"])
        opt = pyconf.arg_with("foo", default="fallback")
        assert opt.value == "fallback"

    def test_value_property_no_default(self, monkeypatch):
        monkeypatch.setattr(sys, "argv", ["configure"])
        opt = pyconf.arg_with("foo")
        assert opt.value is None

    def test_report(self, capsys):
        opt = pyconf.OptionSpec(
            kind="enable", name="foo", display="--enable-foo", _raw="yes"
        )
        value = opt.report("for --enable-foo", "yes")
        out, _ = capsys.readouterr()
        assert value == "yes"
        assert out == "checking for --enable-foo... yes\n"

    def test_process_value_emits_checking_result(self, capsys):
        opt = pyconf.OptionSpec(
            kind="with", name="foo", display="--with-foo", _raw="yes"
        )
        result = opt.process_value(None)
        out, _ = capsys.readouterr()
        assert result == "yes"
        assert out == "checking for --with-foo... yes\n"

    def test_process_value_uses_default_when_raw_is_none(self, capsys):
        opt = pyconf.OptionSpec(
            kind="with",
            name="foo",
            display="--with-foo",
            default="no",
        )
        result = opt.process_value(None)
        out, _ = capsys.readouterr()
        assert result == "no"
        assert out == "checking for --with-foo... no\n"

    def test_process_value_sets_var_on_v(self):
        class V:
            pass

        v = V()
        opt = pyconf.OptionSpec(
            kind="with",
            name="trace-refs",
            display="--with-trace-refs",
            default="no",
            var="with_trace_refs",
        )
        opt.process_value(v)
        assert v.with_trace_refs == "no"

    def test_process_value_sets_var_with_raw_value(self):
        class V:
            pass

        v = V()
        opt = pyconf.OptionSpec(
            kind="enable",
            name="pystats",
            display="--enable-pystats",
            default="no",
            _raw="yes",
            var="enable_pystats",
        )
        result = opt.process_value(v)
        assert result == "yes"
        assert v.enable_pystats == "yes"

    def test_process_value_no_var_skips_setattr(self):
        class V:
            pass

        v = V()
        opt = pyconf.OptionSpec(
            kind="with",
            name="foo",
            display="--with-foo",
            default="no",
        )
        opt.process_value(v)
        assert not hasattr(v, "with_foo")

    def test_process_value_returns_resolved_value(self):
        opt = pyconf.OptionSpec(
            kind="with",
            name="foo",
            display="--with-foo",
            default="no",
            _raw="custom-value",
        )
        assert opt.process_value(None) == "custom-value"

    def test_process_bool_enable_style(self, capsys):
        opt = pyconf.OptionSpec(
            kind="enable",
            name="foo",
            display="--enable-foo",
            _raw="yes",
            false_is="no",
        )
        assert opt.process_bool() is True
        out, _ = capsys.readouterr()
        assert out == "checking for --enable-foo... yes\n"

    def test_process_bool_enable_style_no(self, capsys):
        opt = pyconf.OptionSpec(
            kind="enable",
            name="foo",
            display="--enable-foo",
            _raw="no",
            false_is="no",
        )
        assert opt.process_bool() is False
        out, _ = capsys.readouterr()
        assert out == "checking for --enable-foo... no\n"

    def test_process_bool_disable_style(self, capsys):
        opt = pyconf.OptionSpec(
            kind="enable",
            name="gil",
            display="--disable-gil",
            _raw="no",
            false_is="yes",
        )
        assert opt.process_bool() is True
        out, _ = capsys.readouterr()
        assert out == "checking for --disable-gil... yes\n"

    def test_process_bool_disable_style_yes_is_false(self, capsys):
        opt = pyconf.OptionSpec(
            kind="enable",
            name="gil",
            display="--disable-gil",
            _raw="yes",
            false_is="yes",
        )
        assert opt.process_bool() is False
        out, _ = capsys.readouterr()
        assert out == "checking for --disable-gil... no\n"

    def test_process_bool_none_raw_returns_false(self, capsys):
        opt = pyconf.OptionSpec(
            kind="enable",
            name="foo",
            display="--enable-foo",
            false_is="no",
        )
        assert opt.process_bool() is False
        out, _ = capsys.readouterr()
        assert out == "checking for --enable-foo... no\n"

    def test_process_bool_none_raw_with_default(self, capsys):
        opt = pyconf.OptionSpec(
            kind="enable",
            name="foo",
            display="--enable-foo",
            default="yes",
            false_is="no",
        )
        assert opt.process_bool() is True
        out, _ = capsys.readouterr()
        assert out == "checking for --enable-foo... yes\n"

    def test_process_bool_sets_var(self):
        class V:
            pass

        v = V()
        opt = pyconf.OptionSpec(
            kind="with",
            name="pydebug",
            display="--with-pydebug",
            _raw="yes",
            var="Py_DEBUG",
            false_is="no",
        )
        result = opt.process_bool(v)
        assert result is True
        assert v.Py_DEBUG is True

    def test_process_bool_no_v_skips_setattr(self):
        opt = pyconf.OptionSpec(
            kind="with",
            name="pydebug",
            display="--with-pydebug",
            _raw="yes",
            var="Py_DEBUG",
            false_is="no",
        )
        result = opt.process_bool()
        assert result is True


# ---------------------------------------------------------------------------
# env_var()
# ---------------------------------------------------------------------------


class TestEnvVar:
    def test_registers_description(self):
        pyconf.env_var("CC", "C compiler")
        assert pyconf._env_vars["CC"] == "C compiler"

    def test_seeds_from_os_environ(self, monkeypatch):
        monkeypatch.setenv("PROFILE_TASK", "-m test --pgo")
        pyconf.env_var("PROFILE_TASK", "Python args for PGO")
        assert pyconf.env["PROFILE_TASK"] == "-m test --pgo"

    def test_absent_from_environ_not_seeded(self, monkeypatch):
        monkeypatch.delenv("MY_VAR", raising=False)
        pyconf.env_var("MY_VAR", "some var")
        assert "MY_VAR" not in pyconf.env

    def test_precious_appended_to_config_args(self, monkeypatch):
        monkeypatch.setenv("GDBM_CFLAGS", "-I/usr/include/gdbm")
        pyconf.env_var("GDBM_CFLAGS", "C compiler flags for gdbm")
        assert (
            "GDBM_CFLAGS=-I/usr/include/gdbm" in pyconf._original_config_args
        )

    def test_precious_not_duplicated(self, monkeypatch):
        monkeypatch.setenv("GDBM_CFLAGS", "-I/usr/include/gdbm")
        pyconf._original_config_args.append("GDBM_CFLAGS=-I/usr/include/gdbm")
        pyconf.env_var("GDBM_CFLAGS", "C compiler flags for gdbm")
        count = sum(
            1
            for a in pyconf._original_config_args
            if a.startswith("GDBM_CFLAGS=")
        )
        assert count == 1

    def test_precious_not_added_when_absent(self, monkeypatch):
        monkeypatch.delenv("GDBM_CFLAGS", raising=False)
        pyconf.env_var("GDBM_CFLAGS", "C compiler flags for gdbm")
        assert not any(
            a.startswith("GDBM_CFLAGS=") for a in pyconf._original_config_args
        )


# ---------------------------------------------------------------------------
# define() / define_unquoted() / define_template()
# ---------------------------------------------------------------------------


class TestDefine:
    def test_define_int(self):
        pyconf.define("HAVE_UUID_H", 1, "Define if uuid.h present")
        assert pyconf.defines["HAVE_UUID_H"] == (
            1,
            "Define if uuid.h present",
            True,
        )

    def test_define_str_value(self):
        pyconf.define("RETSIGTYPE", "void", "assume C89 semantics")
        assert pyconf.defines["RETSIGTYPE"] == (
            "void",
            "assume C89 semantics",
            True,
        )

    def test_define_default_value(self):
        pyconf.define("HAVE_FOO")
        assert pyconf.defines["HAVE_FOO"][0] == 1

    def test_define_quoted_flag(self):
        pyconf.define("HAVE_FOO", 1)
        assert pyconf.defines["HAVE_FOO"][2] is True

    def test_define_unquoted(self):
        pyconf.define_unquoted("PYLONG_BITS_IN_DIGIT", 30, "bits per digit")
        assert pyconf.defines["PYLONG_BITS_IN_DIGIT"] == (
            30,
            "bits per digit",
            False,
        )

    def test_define_template_no_value(self):
        pyconf.define_template(
            "HAVE_ZLIB_COPY", "Define if zlib has inflateCopy"
        )
        value, desc, quoted = pyconf.defines["HAVE_ZLIB_COPY"]
        assert value is None
        assert desc == "Define if zlib has inflateCopy"

    def test_define_template_does_not_overwrite(self):
        pyconf.define("HAVE_ZLIB_COPY", 1, "already set")
        pyconf.define_template("HAVE_ZLIB_COPY", "template description")
        assert pyconf.defines["HAVE_ZLIB_COPY"][0] == 1  # not overwritten

    def test_define_overwrites(self):
        pyconf.define("HAVE_FOO", 0)
        pyconf.define("HAVE_FOO", 1)
        assert pyconf.defines["HAVE_FOO"][0] == 1


# ---------------------------------------------------------------------------
# export()
# ---------------------------------------------------------------------------


class TestExport:
    def test_registers_empty_string_when_no_value(self):
        v = pyconf.vars
        v.export("BASECPPFLAGS")
        assert "BASECPPFLAGS" in pyconf.substs
        assert pyconf.substs["BASECPPFLAGS"] == ""

    def test_sets_value_when_assigned(self):
        v = pyconf.vars
        v.LIBPL = "/usr/lib/python3.14"
        v.export("LIBPL")
        assert pyconf.substs["LIBPL"] == "/usr/lib/python3.14"

    def test_does_not_overwrite_existing_when_no_value(self):
        v = pyconf.vars
        pyconf.substs["BASECPPFLAGS"] = "-DFOO"
        v.export("BASECPPFLAGS")
        assert pyconf.substs["BASECPPFLAGS"] == "-DFOO"

    def test_overwrites_when_value_assigned(self):
        v = pyconf.vars
        pyconf.substs["LIBPL"] = "/old"
        v.LIBPL = "/new"
        v.export("LIBPL")
        assert pyconf.substs["LIBPL"] == "/new"

    def test_tracks_exported_names(self):
        v = pyconf.vars
        v.FOO_TEST = "bar"
        v.export("FOO_TEST")
        assert "FOO_TEST" in v._exports


# ---------------------------------------------------------------------------
# config_files()
# ---------------------------------------------------------------------------


class TestConfigFiles:
    def test_registers_files(self):
        pyconf.config_files(["Makefile.pre", "Misc/python.pc"])
        assert "Makefile.pre" in pyconf.substs["_config_files"]
        assert "Misc/python.pc" in pyconf.substs["_config_files"]

    def test_accumulates_across_calls(self):
        pyconf.config_files(["Makefile.pre"])
        pyconf.config_files(["Misc/python.pc"])
        files = pyconf.substs["_config_files"].split()
        assert "Makefile.pre" in files
        assert "Misc/python.pc" in files

    def test_chmod_x_records_separately(self):
        pyconf.config_files(["Modules/ld_so_aix"], chmod_x=True)
        assert "Modules/ld_so_aix" in pyconf.substs["_config_files"]
        assert "Modules/ld_so_aix" in pyconf.substs["_config_files_x"]

    def test_no_chmod_x_by_default(self):
        pyconf.config_files(["Makefile.pre"])
        assert "_config_files_x" not in pyconf.substs


# ---------------------------------------------------------------------------
# Messages: checking() / result() / notice() / warn() / error() / fatal()
# ---------------------------------------------------------------------------


class TestMessages:
    def test_checking_prints_without_newline(self, capsys):
        pyconf.checking("for gcc")
        out, _ = capsys.readouterr()
        assert out == "checking for gcc... "
        assert not out.endswith("\n")

    def test_result_completes_line(self, capsys):
        pyconf.checking("for gcc")
        pyconf.result("yes")
        out, _ = capsys.readouterr()
        assert out == "checking for gcc... yes\n"

    def test_result_converts_to_str(self, capsys):
        pyconf.result(42)
        out, _ = capsys.readouterr()
        assert out == "42\n"

    def test_notice_prints_to_stdout(self, capsys):
        pyconf.notice("hello world")
        out, _ = capsys.readouterr()
        assert "hello world" in out

    def test_warn_prints_to_stderr(self, capsys):
        pyconf.warn("something fishy")
        _, err = capsys.readouterr()
        assert "WARNING" in err
        assert "something fishy" in err

    def test_error_exits_1(self):
        with pytest.raises(SystemExit) as exc:
            pyconf.error("fatal problem")
        assert exc.value.code == 1

    def test_error_prints_to_stderr(self, capsys):
        with pytest.raises(SystemExit):
            pyconf.error("fatal problem")
        _, err = capsys.readouterr()
        assert "fatal problem" in err

    def test_fatal_exits_1(self):
        with pytest.raises(SystemExit) as exc:
            pyconf.fatal("unrecoverable")
        assert exc.value.code == 1

    def test_fatal_prints_to_stderr(self, capsys):
        with pytest.raises(SystemExit):
            pyconf.fatal("unrecoverable")
        _, err = capsys.readouterr()
        assert "unrecoverable" in err


# ---------------------------------------------------------------------------
# use_system_extensions()
# ---------------------------------------------------------------------------


class TestUseSystemExtensions:
    def test_defines_gnu_source(self):
        pyconf.use_system_extensions()
        assert "_GNU_SOURCE" in pyconf.defines

    def test_defines_all_source(self):
        pyconf.use_system_extensions()
        assert "_ALL_SOURCE" in pyconf.defines

    def test_defined_as_unquoted(self):
        pyconf.use_system_extensions()
        _, _, quoted = pyconf.defines["_GNU_SOURCE"]
        assert quoted is False


# ---------------------------------------------------------------------------
# find_install() / find_mkdir_p()
# ---------------------------------------------------------------------------


class TestFindInstall:
    def test_sets_install_in_substs(self):
        pyconf.find_install()
        assert "INSTALL" in pyconf.substs
        assert pyconf.substs["INSTALL"]  # non-empty

    def test_install_is_executable(self):
        pyconf.find_install()
        import shutil

        # INSTALL is "path -c"; check just the executable part
        install_prog = pyconf.substs["INSTALL"].split()[0]
        assert shutil.which(install_prog) is not None or os.path.isfile(
            install_prog
        )


class TestFindMkdirP:
    def test_sets_mkdir_p_in_substs(self):
        pyconf.find_mkdir_p()
        assert "MKDIR_P" in pyconf.substs
        assert "mkdir" in pyconf.substs["MKDIR_P"]

    def test_includes_dash_p_flag(self):
        pyconf.find_mkdir_p()
        assert "-p" in pyconf.substs["MKDIR_P"]


# ---------------------------------------------------------------------------
# check_prog() / check_progs() / check_tools()
# ---------------------------------------------------------------------------


class TestCheckProg:
    def test_finds_existing_program(self):
        result = pyconf.check_prog("sh")
        # check_prog returns the full path (like AC_PATH_TOOL)
        assert result.endswith("/sh")
        assert os.path.isabs(result)

    def test_returns_default_for_missing(self):
        result = pyconf.check_prog(
            "__nonexistent_prog_xyz__", default="NOTFOUND"
        )
        assert result == "NOTFOUND"

    def test_default_empty_string(self):
        result = pyconf.check_prog("__nonexistent_prog_xyz__")
        assert result == ""


class TestCheckProgs:
    def test_returns_first_found(self):
        result = pyconf.check_progs(["__nonexistent__", "sh", "bash"])
        assert result in ("sh", "bash")

    def test_returns_default_when_none_found(self):
        result = pyconf.check_progs(["__a__", "__b__"], default="NONE")
        assert result == "NONE"

    def test_default_is_empty_string(self):
        result = pyconf.check_progs(["__a__", "__b__"])
        assert result == ""


class TestCheckTools:
    def test_alias_for_check_progs(self):
        assert pyconf.check_tools(["sh"]) == pyconf.check_progs(["sh"])

    def test_default_propagates(self):
        assert pyconf.check_tools(["__xyz__"], default="AR") == "AR"


# ---------------------------------------------------------------------------
# Compiler / linker probes  (require a real CC — skip if none)
# ---------------------------------------------------------------------------

import shutil as _shutil  # noqa: E402

HAS_CC = bool(
    _shutil.which("gcc") or _shutil.which("cc") or _shutil.which("clang")
)


@pytest.fixture()
def with_cc(monkeypatch):
    """Set pyconf.CC to a real compiler or skip the test."""
    cc = _shutil.which("gcc") or _shutil.which("cc") or _shutil.which("clang")
    if not cc:
        pytest.skip("no C compiler available")
    monkeypatch.setattr(pyconf, "CC", cc)
    return cc


@pytest.mark.skipif(not HAS_CC, reason="no C compiler available")
class TestCompileCheck:
    def test_simple_compile_succeeds(self, with_cc):
        ok = pyconf.compile_check(body="int x = 0; (void)x;")
        assert ok is True

    def test_bad_syntax_fails(self, with_cc):
        ok = pyconf.compile_check(body="this is not C !!!;")
        assert ok is False

    def test_then_called_on_success(self, with_cc):
        called = []
        pyconf.compile_check(
            body="int x=0;(void)x;", then=lambda: called.append(1)
        )
        assert called == [1]

    def test_else_called_on_failure(self, with_cc):
        called = []
        pyconf.compile_check(
            body="bad code!!!;", else_=lambda: called.append(1)
        )
        assert called == [1]

    def test_includes_added_to_source(self, with_cc):
        ok = pyconf.compile_check(
            includes=["stdio.h"],
            body='printf("");',
        )
        assert ok is True


@pytest.mark.skipif(not HAS_CC, reason="no C compiler available")
class TestLinkCheck:
    def test_valid_source_links(self, with_cc):
        ok = pyconf.link_check(body="int x = 0; (void)x;")
        assert ok is True

    def test_try_link_valid(self, with_cc):
        src = "int main(void) { return 0; }\n"
        assert pyconf.try_link(src) is True

    def test_try_link_invalid(self, with_cc):
        src = "this is not valid C\n"
        assert pyconf.try_link(src) is False


@pytest.mark.skipif(not HAS_CC, reason="no C compiler available")
class TestCheckCompileFlag:
    def test_valid_flag(self, with_cc):
        # -O0 is universally supported
        assert pyconf.check_compile_flag("-O0") is True

    def test_bogus_flag(self, with_cc):
        assert pyconf.check_compile_flag("--bogus-flag-xyz-12345") is False


# ---------------------------------------------------------------------------
# Header probes
# ---------------------------------------------------------------------------


@pytest.mark.skipif(not HAS_CC, reason="no C compiler available")
class TestCheckHeader:
    def test_stdio_h_exists(self, with_cc):
        assert pyconf.check_header("stdio.h") is True

    def test_defines_have_macro(self, with_cc):
        pyconf.check_header("stdio.h")
        assert "HAVE_STDIO_H" in pyconf.defines

    def test_nonexistent_returns_false(self, with_cc):
        assert pyconf.check_header("__nonexistent_header_xyz__.h") is False

    def test_caches_result(self, with_cc):
        pyconf.check_header("stdio.h")
        assert "ac_cv_header_stdio_h" in pyconf.cache

    def test_cached_result_reused(self, with_cc, monkeypatch):
        pyconf.check_header("stdio.h")
        # Poison CC so any real compile would fail
        monkeypatch.setattr(pyconf, "CC", "")
        # Should still return True from cache
        assert pyconf.check_header("stdio.h") is True


@pytest.mark.skipif(not HAS_CC, reason="no C compiler available")
class TestCheckHeaders:
    def test_checks_multiple(self, with_cc):
        results = {}
        pyconf.check_headers("stdio.h", "string.h", defines=results)
        assert results["stdio.h"] is True
        assert results["string.h"] is True

    def test_nonexistent_marked_false(self, with_cc):
        results = {}
        pyconf.check_headers("__no_such__.h", defines=results)
        assert results["__no_such__.h"] is False


@pytest.mark.skipif(not HAS_CC, reason="no C compiler available")
class TestCheckDefine:
    def test_well_known_macro(self, with_cc):
        # NULL is defined in stdio.h
        assert pyconf.check_define("stdio.h", "NULL") is True

    def test_nonexistent_macro(self, with_cc):
        assert pyconf.check_define("stdio.h", "__NO_SUCH_MACRO_XYZ__") is False


# ---------------------------------------------------------------------------
# Type / sizeof probes
# ---------------------------------------------------------------------------


@pytest.mark.skipif(not HAS_CC, reason="no C compiler available")
class TestCheckSizeof:
    def test_sizeof_char_is_1(self, with_cc):
        size = pyconf.check_sizeof("char")
        assert size == 1

    def test_defines_sizeof_macro(self, with_cc):
        pyconf.check_sizeof("int")
        assert "SIZEOF_INT" in pyconf.defines

    def test_sizeof_int_reasonable(self, with_cc):
        size = pyconf.check_sizeof("int")
        assert size in (2, 4, 8)

    def test_caches_result(self, with_cc):
        pyconf.check_sizeof("long")
        assert "ac_cv_sizeof_long" in pyconf.cache

    def test_sizeof_lookup(self, with_cc):
        pyconf.check_sizeof("double")
        assert pyconf.sizeof("double") == pyconf.check_sizeof("double")


@pytest.mark.skipif(not HAS_CC, reason="no C compiler available")
class TestCheckType:
    def test_size_t_exists(self, with_cc):
        assert pyconf.check_type("size_t", headers=["stddef.h"]) is True

    def test_nonexistent_type_returns_false(self, with_cc):
        assert pyconf.check_type("__no_such_type_xyz_t") is False

    def test_fallback_define_called_when_absent(self, with_cc):
        pyconf.check_type(
            "__no_such_type_xyz_t",
            fallback_define=(
                "__no_such_type_xyz_t",
                "unsigned long",
                "fallback",
            ),
        )
        assert "__no_such_type_xyz_t" in pyconf.defines


# ---------------------------------------------------------------------------
# Function / declaration probes
# ---------------------------------------------------------------------------


@pytest.mark.skipif(not HAS_CC, reason="no C compiler available")
class TestCheckFunc:
    def test_printf_exists(self, with_cc):
        assert pyconf.check_func("printf") is True

    def test_defines_have_macro(self, with_cc):
        pyconf.check_func("printf")
        assert "HAVE_PRINTF" in pyconf.defines

    def test_nonexistent_returns_false(self, with_cc):
        assert pyconf.check_func("__no_such_func_xyz_123__") is False

    def test_result_cached(self, with_cc):
        pyconf.check_func("printf")
        assert "ac_cv_func_printf" in pyconf.cache


@pytest.mark.skipif(not HAS_CC, reason="no C compiler available")
class TestCheckFuncs:
    def test_checks_multiple(self, with_cc):
        pyconf.check_funcs(["printf", "strlen"])
        assert "HAVE_PRINTF" in pyconf.defines
        assert "HAVE_STRLEN" in pyconf.defines


@pytest.mark.skipif(not HAS_CC, reason="no C compiler available")
class TestCheckDecl:
    def test_null_is_declared(self, with_cc):
        found = pyconf.check_decl("NULL", includes=["stdio.h"])
        assert found is True

    def test_on_found_callback(self, with_cc):
        called = []
        pyconf.check_decl(
            "NULL", includes=["stdio.h"], on_found=lambda: called.append(1)
        )
        assert called == [1]

    def test_on_notfound_callback(self, with_cc):
        called = []
        pyconf.check_decl(
            "__NO_SUCH_DECL_XYZ__", on_notfound=lambda: called.append(1)
        )
        assert called == [1]


@pytest.mark.skipif(not HAS_CC, reason="no C compiler available")
class TestCheckDecls:
    def test_defines_have_decl_macro(self, with_cc):
        pyconf.check_decls("NULL", includes=["stdio.h"])
        assert "HAVE_DECL_NULL" in pyconf.defines
        assert pyconf.defines["HAVE_DECL_NULL"][0] == 1


# ---------------------------------------------------------------------------
# Library probes
# ---------------------------------------------------------------------------


@pytest.mark.skipif(not HAS_CC, reason="no C compiler available")
class TestCheckLib:
    def test_libc_has_printf(self, with_cc):
        # printf is in libc, but check_lib tests against explicit -l<lib>
        # On Linux, libm has sqrt; test with that
        result = pyconf.check_lib("m", "sqrt")
        assert isinstance(result, bool)

    def test_nonexistent_lib_returns_false(self, with_cc):
        assert pyconf.check_lib("__no_such_lib_xyz__", "printf") is False


# ---------------------------------------------------------------------------
# Stdlib module registration
# ---------------------------------------------------------------------------


class TestStdlibModule:
    def setup_method(self):
        pyconf._stdlib_modules.clear()

    def teardown_method(self):
        pyconf._stdlib_modules.clear()

    def test_registers_module(self):
        pyconf.stdlib_module("_json", supported=True, enabled=True)
        assert "_json" in pyconf._stdlib_modules

    def test_stores_kwargs(self):
        pyconf.stdlib_module("_csv", sources=["Modules/_csv.c"])
        assert pyconf._stdlib_modules["_csv"]["sources"] == ["Modules/_csv.c"]

    def test_supported_enabled_flags(self):
        pyconf.stdlib_module("_testmod", supported=False, enabled=False)
        assert pyconf._stdlib_modules["_testmod"]["supported"] is False
        assert pyconf._stdlib_modules["_testmod"]["enabled"] is False


class TestStdlibModuleSimple:
    def setup_method(self):
        pyconf._stdlib_modules.clear()

    def teardown_method(self):
        pyconf._stdlib_modules.clear()

    def test_registers_with_supported_enabled(self):
        pyconf.stdlib_module_simple("_stat")
        m = pyconf._stdlib_modules["_stat"]
        assert m["supported"] is True
        assert m["enabled"] is True

    def test_stores_cflags_ldflags(self):
        pyconf.stdlib_module_simple(
            "_posixsubprocess", cflags="-DFOO", ldflags="-lbar"
        )
        m = pyconf._stdlib_modules["_posixsubprocess"]
        assert m["cflags"] == "-DFOO"
        assert m["ldflags"] == "-lbar"


class TestStdlibModuleSetNa:
    def setup_method(self):
        pyconf._stdlib_modules.clear()

    def teardown_method(self):
        pyconf._stdlib_modules.clear()
        pyconf._stdlib_modules_pending_na.clear()

    def test_marks_existing_module_unsupported(self):
        pyconf.stdlib_module("_ctypes", supported=True)
        pyconf.stdlib_module_set_na(["_ctypes"])
        assert pyconf._stdlib_modules["_ctypes"]["supported"] is False

    def test_creates_entry_for_unknown_module(self):
        pyconf.stdlib_module_set_na(["_tkinter"])
        # Unknown module is deferred: not yet in _stdlib_modules,
        # but in _stdlib_modules_pending_na (so order follows stdlib_module call).
        assert "_tkinter" not in pyconf._stdlib_modules
        assert "_tkinter" in pyconf._stdlib_modules_pending_na

    def test_multiple_names(self):
        pyconf.stdlib_module_set_na(["_a", "_b", "_c"])
        for name in ("_a", "_b", "_c"):
            assert name not in pyconf._stdlib_modules
            assert name in pyconf._stdlib_modules_pending_na


# ---------------------------------------------------------------------------
# shell()
# ---------------------------------------------------------------------------


class TestShell:
    def test_runs_without_error(self):
        pyconf.shell("echo hello")  # should not raise

    def test_captures_exported_var(self):
        result = pyconf.shell("MYVAR=hello_world", exports=["MYVAR"])
        assert pyconf.vars.MYVAR == "hello_world"
        assert result.MYVAR == "hello_world"

    def test_captures_computed_var(self):
        result = pyconf.shell("X=$(echo computed)", exports=["X"])
        assert pyconf.vars.X == "computed"
        assert result.X == "computed"

    def test_no_exports_no_side_effect(self):
        before = set(vars(pyconf.vars).keys())
        pyconf.shell("TMPVAR=ignored")
        after = set(vars(pyconf.vars).keys())
        assert "TMPVAR" not in after - before


# ---------------------------------------------------------------------------
# sizeof() lookup
# ---------------------------------------------------------------------------


class TestSizeof:
    def test_returns_zero_when_not_computed(self):
        assert pyconf.sizeof("__unknown_type__") == 0

    def test_returns_cached_value(self):
        pyconf.cache["ac_cv_sizeof_int"] = "4"
        assert pyconf.sizeof("int") == 4


# ---------------------------------------------------------------------------
# macro() passthrough
# ---------------------------------------------------------------------------


class TestMacro:
    def test_records_unknown_macro(self):
        pyconf.macro("AC_UNKNOWN_MACRO", ["arg1", "arg2"])
        assert "AC_UNKNOWN_MACRO" in pyconf.substs.get("_unknown_macros", "")

    def test_accumulates_multiple_macros(self):
        pyconf.macro("MACRO_A", [])
        pyconf.macro("MACRO_B", [])
        recorded = pyconf.substs.get("_unknown_macros", "")
        assert "MACRO_A" in recorded
        assert "MACRO_B" in recorded


# ---------------------------------------------------------------------------
# save_env() / restore_env()
# ---------------------------------------------------------------------------


class TestSaveRestoreEnv:
    def test_save_captures_substs(self):
        pyconf.substs["FOO"] = "bar"
        snap = pyconf.save_env()
        assert snap["FOO"] == "bar"

    def test_restore_rolls_back(self):
        pyconf.substs["FOO"] = "original"
        snap = pyconf.save_env()
        pyconf.substs["FOO"] = "modified"
        pyconf.substs["BAR"] = "new"
        pyconf.restore_env(snap)
        assert pyconf.substs["FOO"] == "original"
        assert "BAR" not in pyconf.substs

    def test_snapshot_is_independent(self):
        pyconf.substs["A"] = "1"
        snap = pyconf.save_env()
        pyconf.substs["A"] = "2"
        assert snap["A"] == "1"  # snapshot unaffected


# ---------------------------------------------------------------------------
# output() — _write_pyconfig_h, _subst_vars, _process_config_files
# ---------------------------------------------------------------------------


class TestWritePyconfigH:
    def setup_method(self):
        # Force scratch mode: no pyconfig.h.in template available.
        self._orig_srcdir = pyconf.srcdir
        pyconf.srcdir = "/nonexistent"

    def teardown_method(self):
        pyconf.srcdir = self._orig_srcdir

    def test_creates_file(self, tmp_path):
        pyconf.defines["HAVE_FOO"] = (1, "Has foo", False)
        out = str(tmp_path / "pyconfig.h")
        pyconf._write_pyconfig_h(out)
        text = open(out).read()
        assert "#define HAVE_FOO 1" in text

    def test_header_guard(self, tmp_path):
        out = str(tmp_path / "pyconfig.h")
        pyconf._write_pyconfig_h(out)
        text = open(out).read()
        assert "#ifndef Py_PYCONFIG_H" in text
        assert "#define Py_PYCONFIG_H" in text
        assert "#endif" in text

    def test_quoted_string_value(self, tmp_path):
        pyconf.defines["VERSION"] = ("3.14", "Python version", True)
        out = str(tmp_path / "pyconfig.h")
        pyconf._write_pyconfig_h(out)
        text = open(out).read()
        assert '#define VERSION "3.14"' in text

    def test_unquoted_string_value(self, tmp_path):
        pyconf.defines["CC_NAME"] = ("gcc", "Compiler", False)
        out = str(tmp_path / "pyconfig.h")
        pyconf._write_pyconfig_h(out)
        text = open(out).read()
        assert "#define CC_NAME gcc" in text

    def test_none_value_emits_undef_comment(self, tmp_path):
        pyconf.defines["UNDEF_ME"] = (None, "Not set", False)
        out = str(tmp_path / "pyconfig.h")
        pyconf._write_pyconfig_h(out)
        text = open(out).read()
        assert "/* #undef UNDEF_ME */" in text

    def test_description_as_comment(self, tmp_path):
        pyconf.defines["HAVE_BAR"] = (1, "Define if bar exists", False)
        out = str(tmp_path / "pyconfig.h")
        pyconf._write_pyconfig_h(out)
        text = open(out).read()
        assert "/* Define if bar exists */" in text


class TestSubstVars:
    def test_replaces_known_var(self):
        pyconf.substs["CC"] = "gcc"
        assert pyconf._subst_vars("@CC@") == "gcc"

    def test_leaves_unknown_var(self):
        assert pyconf._subst_vars("@UNKNOWN@") == "@UNKNOWN@"

    def test_multiple_vars(self):
        pyconf.substs["A"] = "hello"
        pyconf.substs["B"] = "world"
        assert pyconf._subst_vars("@A@ @B@") == "hello world"

    def test_no_vars(self):
        assert pyconf._subst_vars("plain text") == "plain text"


class TestProcessConfigFiles:
    def test_substitutes_template(self, tmp_path, monkeypatch):
        monkeypatch.chdir(tmp_path)
        infile = tmp_path / "Makefile.in"
        infile.write_text("CC = @CC@\nVERSION = @VERSION@\n")
        pyconf.srcdir = str(tmp_path)
        pyconf.substs["_config_files"] = "Makefile"
        pyconf.substs["CC"] = "gcc"
        pyconf.substs["VERSION"] = "3.14"
        pyconf._process_config_files()
        result = (tmp_path / "Makefile").read_text()
        assert "CC = gcc" in result
        assert "VERSION = 3.14" in result

    def test_warns_missing_template(self, tmp_path, monkeypatch, capsys):
        monkeypatch.chdir(tmp_path)
        pyconf.srcdir = str(tmp_path)
        pyconf.substs["_config_files"] = "missing_file"
        pyconf._process_config_files()
        err = capsys.readouterr().err
        assert "missing_file" in err

    def test_no_files_is_noop(self, tmp_path, monkeypatch):
        monkeypatch.chdir(tmp_path)
        pyconf.srcdir = str(tmp_path)
        # _config_files not set — should not raise
        pyconf._process_config_files()

    def test_chmod_x(self, tmp_path, monkeypatch):
        monkeypatch.chdir(tmp_path)
        infile = tmp_path / "myscript.in"
        infile.write_text("#!/bin/sh\n")
        pyconf.srcdir = str(tmp_path)
        pyconf.substs["_config_files"] = "myscript"
        pyconf.substs["_config_files_x"] = "myscript"
        pyconf._process_config_files()
        import stat

        mode = os.stat(tmp_path / "myscript").st_mode
        assert mode & stat.S_IXUSR


class TestOutput:
    def test_output_writes_pyconfig_h(self, tmp_path, monkeypatch):
        monkeypatch.chdir(tmp_path)
        pyconf.srcdir = str(tmp_path)
        pyconf.defines["HAVE_STDIO_H"] = (1, "Have stdio.h", False)
        pyconf.output()
        text = (tmp_path / "pyconfig.h").read_text()
        assert "#define HAVE_STDIO_H 1" in text

    def test_output_processes_config_files(self, tmp_path, monkeypatch):
        monkeypatch.chdir(tmp_path)
        (tmp_path / "Makefile.in").write_text("CC=@CC@\n")
        pyconf.srcdir = str(tmp_path)
        pyconf.substs["_config_files"] = "Makefile"
        pyconf.substs["CC"] = "clang"
        pyconf.output()
        assert (tmp_path / "Makefile").read_text() == "CC=clang\n"


# ---------------------------------------------------------------------------
# Cache file serialization/deserialization
# ---------------------------------------------------------------------------


class TestCacheSerialize:
    def test_true_becomes_yes(self):
        assert pyconf._cache_serialize(True) == "yes"

    def test_false_becomes_no(self):
        assert pyconf._cache_serialize(False) == "no"

    def test_int_becomes_string(self):
        assert pyconf._cache_serialize(8) == "8"

    def test_string_passthrough(self):
        assert pyconf._cache_serialize("gcc") == "gcc"


class TestCacheDeserialize:
    def test_yes_becomes_true(self):
        assert pyconf._cache_deserialize("yes") is True

    def test_no_becomes_false(self):
        assert pyconf._cache_deserialize("no") is False

    def test_numeric_stays_string(self):
        assert pyconf._cache_deserialize("8") == "8"

    def test_string_passthrough(self):
        assert pyconf._cache_deserialize("gcc") == "gcc"


class TestCacheQuote:
    def test_simple_value_not_quoted(self):
        assert pyconf._cache_quote("yes") == "yes"

    def test_numeric_not_quoted(self):
        assert pyconf._cache_quote("8") == "8"

    def test_path_not_quoted(self):
        assert pyconf._cache_quote("/usr/bin/gcc") == "/usr/bin/gcc"

    def test_value_with_spaces_quoted(self):
        assert pyconf._cache_quote("none needed") == "'none needed'"

    def test_value_with_single_quote_escaped(self):
        assert pyconf._cache_quote("it's") == "'it'\\''s'"

    def test_empty_string_quoted(self):
        assert pyconf._cache_quote("") == "''"


class TestCacheUnquote:
    def test_unquoted_passthrough(self):
        assert pyconf._cache_unquote("yes") == "yes"

    def test_single_quoted(self):
        assert pyconf._cache_unquote("'none needed'") == "none needed"

    def test_escaped_single_quote(self):
        assert pyconf._cache_unquote("'it'\\''s'") == "it's"


class TestCacheRoundTrip:
    """Test that values survive serialize → quote → unquote → deserialize."""

    def test_bool_true(self):
        v = True
        s = pyconf._cache_serialize(v)
        q = pyconf._cache_quote(s)
        assert pyconf._cache_deserialize(pyconf._cache_unquote(q)) is True

    def test_bool_false(self):
        v = False
        s = pyconf._cache_serialize(v)
        q = pyconf._cache_quote(s)
        assert pyconf._cache_deserialize(pyconf._cache_unquote(q)) is False

    def test_int_value(self):
        v = 8
        s = pyconf._cache_serialize(v)
        q = pyconf._cache_quote(s)
        assert pyconf._cache_unquote(q) == "8"

    def test_string_with_spaces(self):
        v = "none needed"
        s = pyconf._cache_serialize(v)
        q = pyconf._cache_quote(s)
        assert pyconf._cache_unquote(q) == "none needed"


class TestSaveLoadCache:
    """Test _save_cache and _load_cache with actual files."""

    def test_save_creates_file(self, tmp_path, monkeypatch):
        cache_path = str(tmp_path / "config.cache")
        monkeypatch.setattr(pyconf, "cache_file", cache_path)
        pyconf.cache["ac_cv_func_printf"] = True
        pyconf.cache["ac_cv_sizeof_long"] = "8"
        pyconf._save_cache()
        assert os.path.exists(cache_path)

    def test_save_format_matches_autoconf(self, tmp_path, monkeypatch):
        cache_path = str(tmp_path / "config.cache")
        monkeypatch.setattr(pyconf, "cache_file", cache_path)
        pyconf.cache["ac_cv_func_printf"] = True
        pyconf.cache["ac_cv_sizeof_long"] = "8"
        pyconf._save_cache()
        content = open(cache_path).read()
        # Autoconf format: key=${key=value}
        assert "ac_cv_func_printf=${ac_cv_func_printf=yes}" in content
        assert "ac_cv_sizeof_long=${ac_cv_sizeof_long=8}" in content

    def test_save_has_header_comment(self, tmp_path, monkeypatch):
        cache_path = str(tmp_path / "config.cache")
        monkeypatch.setattr(pyconf, "cache_file", cache_path)
        pyconf.cache["ac_cv_func_printf"] = True
        pyconf._save_cache()
        content = open(cache_path).read()
        assert content.startswith("# This file is a shell script")

    def test_save_sorted(self, tmp_path, monkeypatch):
        cache_path = str(tmp_path / "config.cache")
        monkeypatch.setattr(pyconf, "cache_file", cache_path)
        pyconf.cache["ac_cv_func_zzz"] = True
        pyconf.cache["ac_cv_func_aaa"] = False
        pyconf._save_cache()
        lines = [
            line
            for line in open(cache_path).readlines()
            if line.strip() and not line.startswith("#")
        ]
        assert lines[0].startswith("ac_cv_func_aaa=")
        assert lines[1].startswith("ac_cv_func_zzz=")

    def test_save_quotes_spaces(self, tmp_path, monkeypatch):
        cache_path = str(tmp_path / "config.cache")
        monkeypatch.setattr(pyconf, "cache_file", cache_path)
        pyconf.cache["ac_cv_prog_CPP"] = "gcc -E"
        pyconf._save_cache()
        content = open(cache_path).read()
        assert "ac_cv_prog_CPP=${ac_cv_prog_CPP='gcc -E'}" in content

    def test_load_autoconf_format(self, tmp_path, monkeypatch):
        cache_path = str(tmp_path / "config.cache")
        with open(cache_path, "w") as f:
            f.write("# cache\n")
            f.write("ac_cv_func_printf=${ac_cv_func_printf=yes}\n")
            f.write("ac_cv_sizeof_long=${ac_cv_sizeof_long=8}\n")
            f.write("ac_cv_func_missing=${ac_cv_func_missing=no}\n")
            f.write("ac_cv_prog_CPP=${ac_cv_prog_CPP='gcc -E'}\n")
        monkeypatch.setattr(pyconf, "cache_file", cache_path)
        pyconf._load_cache()
        assert pyconf.cache["ac_cv_func_printf"] is True
        assert pyconf.cache["ac_cv_sizeof_long"] == "8"
        assert pyconf.cache["ac_cv_func_missing"] is False
        assert pyconf.cache["ac_cv_prog_CPP"] == "gcc -E"

    def test_load_simple_format(self, tmp_path, monkeypatch):
        """Load the simpler key=value format (legacy pyconf)."""
        cache_path = str(tmp_path / "config.cache")
        with open(cache_path, "w") as f:
            f.write("ac_cv_func_printf=yes\n")
            f.write("ac_cv_sizeof_long=8\n")
        monkeypatch.setattr(pyconf, "cache_file", cache_path)
        pyconf._load_cache()
        assert pyconf.cache["ac_cv_func_printf"] is True
        assert pyconf.cache["ac_cv_sizeof_long"] == "8"

    def test_load_test_format(self, tmp_path, monkeypatch):
        """Load the 'test ${key+y} || key=value' format."""
        cache_path = str(tmp_path / "config.cache")
        with open(cache_path, "w") as f:
            f.write("test ${ac_cv_path_PKG+y} || ac_cv_path_PKG='missing'\n")
        monkeypatch.setattr(pyconf, "cache_file", cache_path)
        pyconf._load_cache()
        assert pyconf.cache["ac_cv_path_PKG"] == "missing"

    def test_roundtrip_save_load(self, tmp_path, monkeypatch):
        cache_path = str(tmp_path / "config.cache")
        monkeypatch.setattr(pyconf, "cache_file", cache_path)
        pyconf.cache["ac_cv_func_printf"] = True
        pyconf.cache["ac_cv_func_missing"] = False
        pyconf.cache["ac_cv_sizeof_long"] = "8"
        pyconf.cache["ac_cv_prog_CPP"] = "gcc -E"
        pyconf._save_cache()
        pyconf.cache.clear()
        pyconf._load_cache()
        assert pyconf.cache["ac_cv_func_printf"] is True
        assert pyconf.cache["ac_cv_func_missing"] is False
        assert pyconf.cache["ac_cv_sizeof_long"] == "8"
        assert pyconf.cache["ac_cv_prog_CPP"] == "gcc -E"

    def test_no_save_when_cache_file_empty(self, tmp_path, monkeypatch):
        monkeypatch.setattr(pyconf, "cache_file", "")
        pyconf.cache["ac_cv_func_printf"] = True
        pyconf._save_cache()
        # No file should be created
        assert not (tmp_path / "config.cache").exists()

    def test_load_skips_comments_and_blank_lines(self, tmp_path, monkeypatch):
        cache_path = str(tmp_path / "config.cache")
        with open(cache_path, "w") as f:
            f.write("# comment\n\n")
            f.write("ac_cv_func_x=${ac_cv_func_x=yes}\n")
            f.write("# another comment\n")
        monkeypatch.setattr(pyconf, "cache_file", cache_path)
        pyconf._load_cache()
        assert pyconf.cache == {"ac_cv_func_x": True}


class TestPrintCached:
    """Test the (cached) output mechanism."""

    def test_prints_cached_when_checking_pending(self, capsys, monkeypatch):
        monkeypatch.setattr(pyconf, "quiet", False)
        monkeypatch.setattr(pyconf, "_help_requested", False)
        pyconf.checking("for something")
        pyconf._print_cached()
        pyconf.result("yes")
        captured = capsys.readouterr()
        assert "(cached) " in captured.out

    def test_no_output_when_not_checking(self, capsys, monkeypatch):
        monkeypatch.setattr(pyconf, "quiet", False)
        monkeypatch.setattr(pyconf, "_help_requested", False)
        pyconf._print_cached()
        captured = capsys.readouterr()
        assert "(cached)" not in captured.out

    def test_no_output_when_quiet(self, capsys, monkeypatch):
        monkeypatch.setattr(pyconf, "quiet", True)
        monkeypatch.setattr(pyconf, "_help_requested", False)
        pyconf.checking("for something")
        pyconf._print_cached()
        pyconf.result("yes")
        captured = capsys.readouterr()
        assert "(cached)" not in captured.out


class TestInitArgsCache:
    """Test cache-related argument parsing in init_args()."""

    def test_dash_c_sets_cache_file(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["configure", "-C"])
        pyconf.cache_file = ""
        pyconf.init_args()
        assert pyconf.cache_file == "config.cache"

    def test_cache_file_equals(self, monkeypatch):
        monkeypatch.setattr(
            "sys.argv", ["configure", "--cache-file=/tmp/my.cache"]
        )
        pyconf.cache_file = ""
        pyconf.init_args()
        assert pyconf.cache_file == "/tmp/my.cache"

    def test_cache_file_separate_arg(self, monkeypatch):
        monkeypatch.setattr(
            "sys.argv", ["configure", "--cache-file", "/tmp/my.cache"]
        )
        pyconf.cache_file = ""
        pyconf.init_args()
        assert pyconf.cache_file == "/tmp/my.cache"

    def test_creating_cache_message(self, capsys, monkeypatch, tmp_path):
        cache_path = str(tmp_path / "config.cache")
        monkeypatch.setattr(
            "sys.argv", ["configure", f"--cache-file={cache_path}"]
        )
        pyconf.cache_file = ""
        pyconf.init_args()
        captured = capsys.readouterr()
        assert f"creating cache {cache_path}" in captured.out

    def test_loading_cache_message(self, capsys, monkeypatch, tmp_path):
        cache_path = str(tmp_path / "config.cache")
        with open(cache_path, "w") as f:
            f.write("# cache\n")
        monkeypatch.setattr(
            "sys.argv", ["configure", f"--cache-file={cache_path}"]
        )
        pyconf.cache_file = ""
        pyconf.init_args()
        captured = capsys.readouterr()
        assert f"loading cache {cache_path}" in captured.out


# ---------------------------------------------------------------------------
# replace_funcs / LIBOBJS
# ---------------------------------------------------------------------------


@pytest.mark.skipif(not HAS_CC, reason="no C compiler available")
class TestReplaceFuncs:
    def test_missing_func_added_to_libobjs(self, with_cc):
        pyconf.replace_funcs(["__no_such_func_xyz_123__"])
        libobjs = pyconf.substs.get("LIBOBJS", "")
        assert "${LIBOBJDIR}__no_such_func_xyz_123__$U.o" in libobjs

    def test_existing_func_not_in_libobjs(self, with_cc):
        pyconf.replace_funcs(["printf"])
        assert pyconf.substs.get("LIBOBJS", "") == ""

    def test_libobjs_format_matches_autoconf(self, with_cc):
        """LIBOBJS entries must use ${LIBOBJDIR}name$U.o for Makefile.pre.in."""
        pyconf.replace_funcs(["__no_such_a__", "__no_such_b__"])
        entries = pyconf.substs["LIBOBJS"].split()
        for entry in entries:
            assert entry.startswith("${LIBOBJDIR}"), entry
            assert "$U.o" in entry, entry


# ---------------------------------------------------------------------------

if __name__ == "__main__":
    sys.exit(pytest.main([__file__, "-v"]))
