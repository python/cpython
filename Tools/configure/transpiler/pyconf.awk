# pyconf.awk — AWK runtime library for the configure transpiler.
#
# POSIX AWK equivalent of pyconf.sh.  All configure logic runs inside
# a single AWK process, using associative arrays for state.
#
# Data structures:
#   V[]            - configuration variables (replaces flat shell namespace)
#   ENV[]          - shadow environment (init from ENVIRON[])
#   MODIFIED_ENV[] - tracks which env vars were modified
#   DEFINES[]      - pyconfig.h defines: name -> value
#   DEFINE_DESC[]  - pyconfig.h define descriptions
#   DEFINE_QUOTED[]- whether define value needs quoting
#   CACHE[]        - probe cache (ac_cv_*)
#   OPT_VAL[]      - option values
#   OPT_GIVEN[]    - option given flags (0/1)
#   OPT_KIND[]     - option kind ("enable" or "with")
#   OPT_DEFAULT[]  - option defaults
#   OPT_HELP[]     - option help text
#   OPT_METAVAR[]  - option metavar
#   OPT_VAR[]      - option env variable binding
#   OPT_FALSE_IS[] - option false_is value
#   SUBST[]        - Makefile substitution variables
#   CONFIG_FILES[] - list of config files to process
#   CONFIG_FILES_X[] - config files needing chmod +x

# ---------------------------------------------------------------------------
# Initialization
# ---------------------------------------------------------------------------

function pyconf_init(    k) {
        # Shadow ENVIRON into ENV[] and seed V[] so that environment
        # variables are accessible as V["name"], mirroring both shell
        # (where $VAR inherits from the environment) and Python's
        # Vars.__getattr__ fallback to os.environ.
        for (k in ENVIRON) {
                ENV[k] = ENVIRON[k]
                V[k] = ENVIRON[k]
        }
        _pyconf_tmpdir = ""
        _pyconf_verbose = 0
        _pyconf_srcdir = "."
        _pyconf_cross_compiling = "no"
        _pyconf_config_file_count = 0
        _pyconf_config_file_x_count = 0
        SUBST["LIBOBJS"] = ""
        _pyconf_opt_count = 0
        _pyconf_checking_msg = ""
        _pyconf_deferred_result = ""
        _pyconf_defines_order_n = 0
        # Create temp directory
        pyconf_mktmpdir()
}

function pyconf_mktmpdir(    cmd, line) {
        cmd = "mktemp -d /tmp/pyconf.XXXXXX 2>/dev/null || echo /tmp/pyconf.$$"
        if ((cmd | getline line) > 0)
                _pyconf_tmpdir = line
        close(cmd)
        if (_pyconf_tmpdir == "" || substr(_pyconf_tmpdir, 1, 1) != "/") {
                cmd = "echo /tmp/pyconf.$$"
                if ((cmd | getline line) > 0)
                        _pyconf_tmpdir = line
                close(cmd)
        }
        system("mkdir -p " _pyconf_tmpdir)
}

function pyconf_cleanup() {
        if (_pyconf_tmpdir != "")
                system("rm -rf " _pyconf_tmpdir)
}

function pyconf_load_config_site(    config_site, line, eq, name, value) {
        # Load CONFIG_SITE file if set (autoconf compatibility).
        # CONFIG_SITE is a shell script with simple VAR=VALUE assignments that
        # pre-seed cache variables for cross-compilation.  Command-line VAR=VALUE
        # arguments and existing environment variables take precedence.
        config_site = ENVIRON["CONFIG_SITE"]
        if (config_site == "") return
        while ((getline line < config_site) > 0) {
                # Skip comments and blank lines
                sub(/^[ \t]+/, "", line)
                if (line == "" || substr(line, 1, 1) == "#") continue
                # Match VAR=VALUE
                eq = index(line, "=")
                if (eq == 0) continue
                name = substr(line, 1, eq - 1)
                if (!match(name, /^[A-Za-z_][A-Za-z0-9_]*$/)) continue
                value = substr(line, eq + 1)
                # Strip surrounding quotes (single or double)
                if (length(value) >= 2) {
                        if ((substr(value, 1, 1) == "'" && substr(value, length(value), 1) == "'") || \
                            (substr(value, 1, 1) == "\"" && substr(value, length(value), 1) == "\""))
                                value = substr(value, 2, length(value) - 2)
                }
                # Don't override existing env vars or already-set V[] entries
                if (name in ENVIRON) continue
                if (name in V) continue
                V[name] = value
                # Also populate CACHE for ac_cv_* keys so that check_func(),
                # check_header(), etc. honour the pre-seeded value
                if (substr(name, 1, 6) == "ac_cv_")
                        CACHE[name] = value
        }
        close(config_site)
}

# ---------------------------------------------------------------------------
# Logging / messaging
# ---------------------------------------------------------------------------

function pyconf_checking(msg) {
        _pyconf_checking_msg = msg
        printf "checking %s... ", msg
}

function pyconf_result(val) {
        printf "%s\n", val
        _pyconf_checking_msg = ""
}

function _pyconf_flush_result() {
        if (_pyconf_deferred_result != "") {
                pyconf_result(_pyconf_deferred_result)
                _pyconf_deferred_result = ""
        }
}

function _pyconf_print_cached(val) {
        printf "%s (cached)\n", val
}

# AC_CACHE_CHECK — check CACHE before running a probe.
# Returns cached value on hit (prints "(cached) value"), "" on miss.
function pyconf_cache_check(description, cache_var) {
        pyconf_checking(description)
        if (cache_var in CACHE) {
                pyconf_result("(cached) " _pyconf_format_result(CACHE[cache_var]))
                return CACHE[cache_var]
        }
        return ""
}

function pyconf_cache_store(cache_var, value) {
        CACHE[cache_var] = value
}

function _pyconf_format_result(val) {
        if (val == "yes") return "yes"
        if (val == "no") return "no"
        return val
}

function pyconf_warn(msg) {
        printf "WARNING: %s\n", msg > "/dev/stderr"
}

function pyconf_notice(msg) {
        printf "%s\n", msg
}

function pyconf_error(msg) {
        printf "configure: error: %s\n", msg > "/dev/stderr"
        exit 1
}

function pyconf_fatal(msg) {
        printf "configure: error: %s\n", msg > "/dev/stderr"
        exit 1
}

# ---------------------------------------------------------------------------
# Defines (pyconfig.h)
# ---------------------------------------------------------------------------

function _pyconf_init_defines() {
        # Nothing needed — arrays are auto-initialized
}

function pyconf_define(name, value, quoted, desc) {
        if (!(name in DEFINES)) {
                _pyconf_defines_order_n++
                _pyconf_defines_order[_pyconf_defines_order_n] = name
        }
        DEFINES[name] = value
        if (desc != "") DEFINE_DESC[name] = desc
        DEFINE_QUOTED[name] = quoted + 0
}

function pyconf_define_unquoted(name, value, desc) {
        pyconf_define(name, value, 0, desc)
}

function pyconf_define_template(name, desc) {
        if (!(name in DEFINES)) {
                _pyconf_defines_order_n++
                _pyconf_defines_order[_pyconf_defines_order_n] = name
        }
        if (desc != "") DEFINE_DESC[name] = desc
        # Template only — no value set yet
}

function _pyconf_define_remove(name) {
        delete DEFINES[name]
        delete DEFINE_DESC[name]
        delete DEFINE_QUOTED[name]
}

function _pyconf_define_get(name) {
        if (name in DEFINES)
                return DEFINES[name]
        return ""
}

function pyconf_is_defined(name) {
        return (name in DEFINES)
}

# ---------------------------------------------------------------------------
# Substitution variables
# ---------------------------------------------------------------------------

function pyconf_subst_set(name, value) {
        SUBST[name] = value
}

function pyconf_subst_get(name) {
        if (name in SUBST)
                return SUBST[name]
        return ""
}

function pyconf_export(name, value) {
        V[name] = value
        ENV[name] = value
        MODIFIED_ENV[name] = 1
}

function v_export(name) {
        # Fall back to ENVIRON if V[name] is not set, matching
        # autoconf where AC_SUBST reads inherited environment variables.
        if (!(name in V) && name in ENVIRON)
                V[name] = ENVIRON[name]
        ENV[name] = V[name]
        MODIFIED_ENV[name] = 1
}

# ---------------------------------------------------------------------------
# Cache
# ---------------------------------------------------------------------------

function _pyconf_cache_set(key, value) {
        CACHE[key] = value
}

function _pyconf_cache_get(key) {
        if (key in CACHE)
                return CACHE[key]
        return ""
}

function _pyconf_cache_has(key) {
        return (key in CACHE)
}

# ---------------------------------------------------------------------------
# Command execution helpers
# ---------------------------------------------------------------------------

function _arr_join(arr, sep, n, i, result) {
        n = arr[0] + 0
        result = ""
        for (i = 1; i <= n; i++)
                result = result (i > 1 ? sep : "") arr[i]
        return result
}

function _arr_join_quoted(arr, sep, n, i, result) {
        n = arr[0] + 0
        result = ""
        for (i = 1; i <= n; i++)
                result = result (i > 1 ? sep : "") _shell_quote(arr[i])
        return result
}

# Expand $VAR and ${VAR} references in a command string from V[], falling
# back to ENVIRON[].  Mirrors what Python's pyconf.run() does with its
# vars= parameter: the command template contains shell-style variable
# references that must be resolved before execution.
function _expand_cmd_vars(cmd, n, i, c, varname, brace, parts, pc) {
	# Accumulate expanded command in array to avoid mawk string concatenation corruption
	# (character-by-character concatenation in a large loop causes memory issues in mawk).
	n = length(cmd)
	i = 1
	pc = 0  # parts count
	while (i <= n) {
		c = substr(cmd, i, 1)
		if (c == "$" && i < n) {
			# Check for ${VAR} or $VAR
			if (substr(cmd, i + 1, 1) == "{") {
				brace = 1
				i += 2
				varname = ""
				while (i <= n && substr(cmd, i, 1) != "}") {
					varname = varname substr(cmd, i, 1)
					i++
				}
				if (i <= n) i++  # skip closing }
			} else {
				brace = 0
				i++
				varname = ""
				while (i <= n && substr(cmd, i, 1) ~ /[A-Za-z0-9_]/) {
					varname = varname substr(cmd, i, 1)
					i++
				}
			}
			if (varname == "") {
				parts[++pc] = "$"
				if (brace) parts[++pc] = "{"
			} else if (varname in V) {
				parts[++pc] = V[varname]
			} else if (varname in ENVIRON) {
				parts[++pc] = ENVIRON[varname]
			}
			# else: unknown var expands to empty (matches shell behaviour)
		} else {
			parts[++pc] = c
			i++
		}
	}
	# Join all parts once at the end
	return _join_parts(parts, pc)
}

function _join_parts(arr, n,    i, result) {
	# Join array elements into a single string. Called once at end of
	# _expand_cmd_vars to avoid character-by-character concatenation.
	result = ""
	for (i = 1; i <= n; i++)
		result = result arr[i]
	return result
}

function _cmd_output(cmd, line, lines, ln) {
	# Accumulate command output lines in array to avoid mawk string concatenation
	# corruption (large concatenation in loop causes memory issues in mawk).
	ln = 0
	while ((cmd | getline line) > 0)
		lines[++ln] = line
	close(cmd)
	# Join all lines once at the end
	return _join_lines(lines, ln)
}

function _join_lines(arr, n,    i, result) {
	# Join array of lines with newline separators. Called once at end of
	# _cmd_output to avoid repeated string concatenation in loop.
	result = ""
	for (i = 1; i <= n; i++)
		result = result (i > 1 ? "\n" : "") arr[i]
	return result
}

function _cmd_output_oneline(cmd, line) {
        line = ""
        cmd | getline line
        close(cmd)
        return line
}

function _cmd_exit_code(cmd) {
        return system(cmd " >/dev/null 2>&1")
}

function _pyconf_env_prefix(    k, prefix) {
        prefix = ""
        for (k in MODIFIED_ENV) {
                prefix = prefix k "=" _shell_quote(ENV[k]) " "
        }
        return prefix
}

function _shell_quote(s, r) {
        r = s
        gsub(/'/, "'\\''", r)
        return "'" r "'"
}

# ---------------------------------------------------------------------------
# Compiler checks
# ---------------------------------------------------------------------------

# Write confdefs directly to file from the array, avoiding building a
# large concatenated string in AWK memory.  This prevents memory
# corruption in mawk with many repeated compile/link checks.
function _pyconf_write_confdefs(file,    i, name, val) {
        for (i = 1; i <= _pyconf_defines_order_n; i++) {
                name = _pyconf_defines_order[i]
                if (name in DEFINES) {
                        val = DEFINES[name]
                        if (DEFINE_QUOTED[name])
                                printf "#define %s \"%s\"\n", name, val > file
                        else
                                printf "#define %s %s\n", name, val > file
                }
        }
}

function _split_cc_lib_flags(extra_flags, pre, post, n, arr, i) {
        # Split extra_flags: -l/-L/-Wl, flags go into post (after source),
        # everything else goes into pre (before source).
        # This matches GNU ld ordering requirements.
        pre[0] = ""
        post[0] = ""
        n = split(extra_flags, arr, " ")
        for (i = 1; i <= n; i++) {
                if (arr[i] == "") continue
                if (substr(arr[i], 1, 2) == "-l" || substr(arr[i], 1, 2) == "-L" || substr(arr[i], 1, 4) == "-Wl,")
                        post[0] = post[0] (post[0] != "" ? " " : "") arr[i]
                else
                        pre[0] = pre[0] (pre[0] != "" ? " " : "") arr[i]
        }
}

function _pyconf_run_cc(source, extra_flags, mode, with_confdefs, conftest, cmd, rc, pre, post) {
        conftest = _pyconf_tmpdir "/conftest.c"
        if (with_confdefs) {
                _pyconf_write_confdefs(conftest)
                printf "\n%s", source >> conftest
        } else {
                printf "%s", source > conftest
        }
        close(conftest)

        cmd = _pyconf_env_prefix()
        if (mode == "compile")
                cmd = cmd V["CC"] " " V["CPPFLAGS"] " " V["CFLAGS"] " " extra_flags " -c " conftest " -o " _pyconf_tmpdir "/conftest.o 2>/dev/null"
        else {
                _split_cc_lib_flags(extra_flags, pre, post)
                cmd = cmd V["CC"] " " V["CPPFLAGS"] " " V["CFLAGS"] " " V["LDFLAGS"] " " pre[0] " " conftest " -o " _pyconf_tmpdir "/conftest " post[0] " " V["LIBS"] " 2>/dev/null"
        }

        rc = system(cmd)
        system("rm -f " _pyconf_tmpdir "/conftest.c " _pyconf_tmpdir "/conftest.o " _pyconf_tmpdir "/conftest")
        return rc
}

function _pyconf_compile_test(source, extra_flags) {
        return (_pyconf_run_cc(source, extra_flags, "compile") == 0)
}

function _pyconf_compile_test_cd(source, extra_flags) {
        return (_pyconf_run_cc(source, extra_flags, "compile", 1) == 0)
}

function _pyconf_link_test(source, extra_flags) {
        return (_pyconf_run_cc(source, extra_flags, "link") == 0)
}

function _pyconf_link_test_cd(source, extra_flags) {
        return (_pyconf_run_cc(source, extra_flags, "link", 1) == 0)
}

# _pyconf_compute_int — compile-time binary search for an integer expression.
# Matches autoconf's AC_COMPUTE_INT / _AC_COMPUTE_INT_COMPILE.
# Returns the integer value, or "" on failure.
function _pyconf_compute_int(expr, includes, ac_lo, ac_hi, ac_mid, src) {
        # Step 1: determine sign
        src = includes "\nint main(void) {\n  static int test_array [1 - 2 * !((" expr ") >= 0)];\n  test_array[0] = 0;\n  return test_array[0];\n}\n"
        if (_pyconf_compile_test_cd(src)) {
                # Non-negative: search upward from 0
                ac_lo = 0; ac_mid = 0
                while (1) {
                        src = includes "\nint main(void) {\n  static int test_array [1 - 2 * !((" expr ") <= " ac_mid ")];\n  test_array[0] = 0;\n  return test_array[0];\n}\n"
                        if (_pyconf_compile_test_cd(src)) {
                                ac_hi = ac_mid; break
                        }
                        ac_lo = ac_mid + 1
                        if (ac_lo <= ac_mid) return ""
                        ac_mid = 2 * ac_mid + 1
                }
        } else {
                src = includes "\nint main(void) {\n  static int test_array [1 - 2 * !((" expr ") < 0)];\n  test_array[0] = 0;\n  return test_array[0];\n}\n"
                if (_pyconf_compile_test_cd(src)) {
                        # Negative: search downward from -1
                        ac_hi = -1; ac_mid = -1
                        while (1) {
                                src = includes "\nint main(void) {\n  static int test_array [1 - 2 * !((" expr ") >= " ac_mid ")];\n  test_array[0] = 0;\n  return test_array[0];\n}\n"
                                if (_pyconf_compile_test_cd(src)) {
                                        ac_lo = ac_mid; break
                                }
                                ac_hi = ac_mid - 1
                                if (ac_mid <= ac_hi) return ""
                                ac_mid = 2 * ac_mid
                        }
                } else {
                        return ""
                }
        }
        # Step 2: binary search between lo and hi
        while (ac_lo != ac_hi) {
                ac_mid = int((ac_hi - ac_lo) / 2) + ac_lo
                src = includes "\nint main(void) {\n  static int test_array [1 - 2 * !((" expr ") <= " ac_mid ")];\n  test_array[0] = 0;\n  return test_array[0];\n}\n"
                if (_pyconf_compile_test_cd(src))
                        ac_hi = ac_mid
                else
                        ac_lo = ac_mid + 1
        }
        return ac_lo
}

function _pyconf_run_test(source, extra_flags, with_confdefs, conftest, cmd, rc, line, result, outf, pre, post) {
        conftest = _pyconf_tmpdir "/conftest.c"
        if (with_confdefs) {
                _pyconf_write_confdefs(conftest)
                printf "\n%s", source >> conftest
        } else {
                printf "%s", source > conftest
        }
        close(conftest)

        cmd = _pyconf_env_prefix()
        _split_cc_lib_flags(extra_flags, pre, post)
        cmd = cmd V["CC"] " " V["CPPFLAGS"] " " V["CFLAGS"] " " V["LDFLAGS"] " " pre[0] " " conftest " -o " _pyconf_tmpdir "/conftest " post[0] " " V["LIBS"] " 2>/dev/null"
        if (system(cmd) != 0) {
                system("rm -f " conftest " " _pyconf_tmpdir "/conftest")
                _pyconf_run_test_rc = 1
                return ""
        }

        outf = _pyconf_tmpdir "/conftest_out"
        rc = system(_pyconf_tmpdir "/conftest > " outf " 2>/dev/null")
        _pyconf_run_test_rc = rc
        result = ""
        cmd = "cat " outf
        while ((cmd | getline line) > 0)
                result = result (result != "" ? "\n" : "") line
        close(cmd)
        system("rm -f " conftest " " _pyconf_tmpdir "/conftest " outf)
        return result
}

# ---------------------------------------------------------------------------
# High-level check functions
# ---------------------------------------------------------------------------

function pyconf_compile_check(desc, source, extra_flags, compiler, rc, saved_cc) {
        if (desc != "") pyconf_checking(desc)
        if (compiler != "") { saved_cc = V["CC"]; V["CC"] = compiler }
        rc = _pyconf_compile_test_cd(source, extra_flags)
        if (compiler != "") V["CC"] = saved_cc
        if (desc != "") pyconf_result(rc ? "yes" : "no")
        return rc
}

function pyconf_link_check(desc, source, extra_flags, compiler, rc, saved_cc) {
        if (desc != "") pyconf_checking(desc)
        if (compiler != "") { saved_cc = V["CC"]; V["CC"] = compiler }
        rc = _pyconf_link_test_cd(source, extra_flags)
        if (compiler != "") V["CC"] = saved_cc
        if (desc != "") pyconf_result(rc ? "yes" : "no")
        return rc
}

function pyconf_try_link(desc, source, extra_flags) {
        return pyconf_link_check(desc, source, extra_flags)
}

function pyconf_compile_link_check(desc, source, extra_flags, compiler, rc, result, saved_cc) {
        if (desc != "") pyconf_checking(desc)
        if (compiler != "") { saved_cc = V["CC"]; V["CC"] = compiler }
        rc = _pyconf_link_test_cd(source, extra_flags)
        if (compiler != "") V["CC"] = saved_cc
        if (rc)
                result = "yes"
        else
                result = "no"
        if (desc != "") pyconf_result(result)
        return result
}

function pyconf_run_check(desc, source, extra_cflags, extra_libs, cross_val, rc, result) {
        if (desc != "") pyconf_checking(desc)
        # AC_RUN_IFELSE equivalent: skip when cross-compiling.
        # Return cross_val (caller-supplied cross-compiling default).
        if (pyconf_cross_compiling == "yes" || pyconf_cross_compiling == "maybe") {
                if (desc != "") pyconf_result((cross_val + 0) ? "yes" : "no")
                return (cross_val + 0)
        }
        result = _pyconf_run_test(source, extra_cflags " " extra_libs, 1)
        rc = _pyconf_run_test_rc
        if (desc != "") {
                if (rc == 0)
                        pyconf_result(result != "" ? result : "yes")
                else
                        pyconf_result("failed")
        }
        return (rc == 0)
}

function pyconf_run_program_output(desc, source, extra_flags, result) {
        if (desc != "") pyconf_checking(desc)
        # Skip when cross-compiling (return empty string)
        if (pyconf_cross_compiling == "yes" || pyconf_cross_compiling == "maybe") {
                if (desc != "") pyconf_result("")
                return ""
        }
        result = _pyconf_run_test(source, extra_flags, 1)
        if (desc != "") pyconf_result(result != "" ? result : "failed")
        return result
}

function pyconf_check_compile_flag(flag, extra_flags, source, all_flags) {
        source = "int main(void) { return 0; }"
        all_flags = flag
        if (extra_flags[0] + 0 > 0)
                all_flags = all_flags " " _arr_join_quoted(extra_flags, " ")
        return _pyconf_compile_test(source, all_flags)
}

function pyconf_check_linker_flag(flag, value, source) {
        source = "int main(void) { return 0; }"
        if (_pyconf_link_test(source, flag)) {
                if (value != "")
                        return value
                return flag
        }
        return ""
}

# ---------------------------------------------------------------------------
# Header checks
# ---------------------------------------------------------------------------

function _pyconf_includes_from_list(hdr_list, n, arr, i, result) {
        # Convert space-separated header list to #include lines.
        result = ""
        n = split(hdr_list, arr, " ")
        for (i = 1; i <= n; i++) {
                if (arr[i] != "")
                        result = result "#include <" arr[i] ">\n"
        }
        return result
}

function _pyconf_header_to_define(header, d) {
        d = "HAVE_" header
        # Map "-" to "_DASH_" before other replacements so that
        # gdbm-ndbm.h -> HAVE_GDBM_DASH_NDBM_H (distinct from
        # gdbm/ndbm.h -> HAVE_GDBM_NDBM_H), matching autoconf.
        gsub(/-/, "_DASH_", d)
        gsub(/[\/\.\+]/, "_", d)
        d = toupper(d)
        return d
}

function _pyconf_ac_includes_default(    result) {
        result = ""
        if ("HAVE_STDIO_H" in DEFINES) result = result "#include <stdio.h>\n"
        if ("HAVE_STDLIB_H" in DEFINES) result = result "#include <stdlib.h>\n"
        if ("HAVE_STRING_H" in DEFINES) result = result "#include <string.h>\n"
        if ("HAVE_INTTYPES_H" in DEFINES) result = result "#include <inttypes.h>\n"
        if ("HAVE_STDINT_H" in DEFINES) result = result "#include <stdint.h>\n"
        if ("HAVE_STRINGS_H" in DEFINES) result = result "#include <strings.h>\n"
        if ("HAVE_SYS_TYPES_H" in DEFINES) result = result "#include <sys/types.h>\n"
        if ("HAVE_SYS_STAT_H" in DEFINES) result = result "#include <sys/stat.h>\n"
        if ("HAVE_UNISTD_H" in DEFINES) result = result "#include <unistd.h>\n"
        if (result == "")
                result = "#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n"
        return result
}

function pyconf_check_header(header, prologue, default_inc, define, cache_key, extra_cflags, source, cv, rc) {
        if (define == "") define = _pyconf_header_to_define(header)
        if (cache_key == "") cache_key = "ac_cv_header_" header
        gsub(/-/, "_dash_", cache_key)
        gsub(/[\/\.\+]/, "_", cache_key)

        if (cache_key in CACHE) {
                rc = (CACHE[cache_key] == "yes")
        } else {
                pyconf_checking("for " header)
                if (prologue != "")
                        source = prologue "\n#include <" header ">\nint main(void) { return 0; }"
                else if (default_inc != "")
                        source = default_inc "\n#include <" header ">\nint main(void) { return 0; }"
                else
                        source = _pyconf_ac_includes_default() "#include <" header ">\nint main(void) { return 0; }"
                rc = _pyconf_compile_test_cd(source, extra_cflags)
                CACHE[cache_key] = rc ? "yes" : "no"
                V[cache_key] = rc ? "yes" : "no"
                pyconf_result(rc ? "yes" : "no")
        }

        if (rc)
                pyconf_define(define, 1, 0, "Define to 1 if you have the <" header "> header file.")
        return rc
}

function pyconf_check_headers(headers, n, i, all_found) {
        all_found = 1
        n = headers[0] + 0
        for (i = 1; i <= n; i++)
                if (headers[i] != "") {
                        if (!pyconf_check_header(headers[i]))
                                all_found = 0
                }
        return all_found
}

# ---------------------------------------------------------------------------
# Function checks
# ---------------------------------------------------------------------------

function pyconf_check_func(fname, headers, define, source, inc, cv, rc, cache_key, n, arr, i, proto, call, has_headers) {
        if (define == "") define = "HAVE_" toupper(fname)
        cache_key = "ac_cv_func_" fname

        if (cache_key in CACHE) {
                rc = (CACHE[cache_key] == "yes")
                if (rc) pyconf_define(define, 1, 0, "Define to 1 if you have the `" fname "` function.")
                return rc
        }

        pyconf_checking("for " fname)
        inc = ""
        has_headers = 0
        if (headers != "") {
                n = split(headers, arr, " ")
                for (i = 1; i <= n; i++)
                        if (arr[i] != "") {
                                inc = inc "#include <" arr[i] ">\n"
                                has_headers = 1
                        }
        }

        # Match Python pyconf.check_func behaviour:
        # - With headers: compile-only, take address of func (PY_CHECK_FUNC style)
        #   Include default headers + user headers.
        # - Without headers: link test with stub prototype (AC_CHECK_FUNC style)
        #   No headers at all — the stub prototype "char func();" would conflict
        #   with real declarations from standard headers (e.g. stdlib.h declares
        #   clearenv when _GNU_SOURCE is defined).
        if (has_headers) {
                proto = ""
                call = "(void)(&" fname ")"
                inc = _pyconf_ac_includes_default() inc
                source = inc "\n" proto "int main(void) { " call "; return 0; }"
                rc = _pyconf_compile_test_cd(source, "")
        } else {
                proto = "#ifdef __cplusplus\nextern \"C\"\n#endif\nchar " fname "();\n"
                call = fname "()"
                source = proto "int main(void) { " call "; return 0; }"
                rc = _pyconf_link_test_cd(source, "")
        }
        CACHE[cache_key] = rc ? "yes" : "no"
        V[cache_key] = rc ? "yes" : "no"
        pyconf_result(rc ? "yes" : "no")

        if (rc)
                pyconf_define(define, 1, 0, "Define to 1 if you have the `" fname "` function.")
        return rc
}

function pyconf_check_funcs(funcs, n, i) {
        n = funcs[0] + 0
        for (i = 1; i <= n; i++)
                if (funcs[i] != "")
                        pyconf_check_func(funcs[i])
}

function pyconf_replace_funcs(funcs, n, i) {
        # AC_REPLACE_FUNCS — check each function; add missing ones to LIBOBJS.
        n = funcs[0] + 0
        for (i = 1; i <= n; i++) {
                if (funcs[i] != "") {
                        if (!pyconf_check_func(funcs[i])) {
                                if (SUBST["LIBOBJS"] != "")
                                        SUBST["LIBOBJS"] = SUBST["LIBOBJS"] " ${LIBOBJDIR}" funcs[i] "$U.o"
                                else
                                        SUBST["LIBOBJS"] = "${LIBOBJDIR}" funcs[i] "$U.o"
                        }
                }
        }
}

# ---------------------------------------------------------------------------
# Type / sizeof / alignof checks
# ---------------------------------------------------------------------------

function _pyconf_type_to_define(type_name, d) {
        d = type_name
        gsub(/[ \*]/, "_", d)
        return "HAVE_" toupper(d)
}

function pyconf_check_sizeof(type_name, default_val, headers, source, inc, result, n, arr, i, cv, define) {
        cv = "ac_cv_sizeof_" type_name
        gsub(/ /, "_", cv)
        gsub(/\*/, "p", cv)

        define = "SIZEOF_" toupper(type_name)
        gsub(/ /, "_", define)
        gsub(/\*/, "P", define)

        if (cv in CACHE) {
                result = CACHE[cv]
                pyconf_define(define, result, 0, "The size of `" type_name "`, as computed by sizeof.")
                return result
        }

        pyconf_checking("for sizeof " type_name)
        inc = _pyconf_ac_includes_default()
        if (headers != "") {
                n = split(headers, arr, " ")
                for (i = 1; i <= n; i++)
                        if (arr[i] != "")
                                inc = inc "#include <" arr[i] ">\n"
        }

        result = ""
        if (pyconf_cross_compiling != "yes") {
                source = inc "\n#include <stdio.h>\nint main(void) { printf(\"%d\", (int)sizeof(" type_name ")); return 0; }"
                result = _pyconf_run_test(source, "", 1)
        }
        if (result == "") {
                # Cross-compiling or run failed: compile-time binary search
                result = _pyconf_compute_int("(long int)(sizeof(" type_name "))", inc)
        }
        if (result == "") {
                if (default_val != "")
                        result = default_val
                else
                        result = "0"
        }
        CACHE[cv] = result
        V[cv] = result
        pyconf_result(result)
        pyconf_define(define, result, 0, "The size of `" type_name "`, as computed by sizeof.")
        return result
}

function pyconf_check_alignof(type_name, default_val, headers, source, inc, result, n, arr, i, cv, define) {
        cv = "ac_cv_alignof_" type_name
        gsub(/ /, "_", cv)

        define = "ALIGNOF_" toupper(type_name)
        gsub(/ /, "_", define)

        if (cv in CACHE) {
                result = CACHE[cv]
                pyconf_define(define, result, 0, "")
                return result
        }

        pyconf_checking("for alignof " type_name)
        inc = _pyconf_ac_includes_default()
        if (headers != "") {
                n = split(headers, arr, " ")
                for (i = 1; i <= n; i++)
                        if (arr[i] != "")
                                inc = inc "#include <" arr[i] ">\n"
        }

        result = ""
        if (pyconf_cross_compiling != "yes") {
                source = inc "\n#include <stdio.h>\n#include <stddef.h>\nstruct _align_test { char c; " type_name " x; };\nint main(void) { printf(\"%d\", (int)offsetof(struct _align_test, x)); return 0; }"
                result = _pyconf_run_test(source, "", 1)
        }
        if (result == "") {
                # Cross-compiling or run failed: compile-time binary search
                result = _pyconf_compute_int("(long int)(offsetof(struct { char c; " type_name " x; }, x))", "#include <stddef.h>\n" inc)
        }
        if (result == "") result = (default_val != "" ? default_val : "0")
        CACHE[cv] = result
        V[cv] = result
        pyconf_result(result)
        pyconf_define(define, result, 0, "")
        return result
}

function pyconf_sizeof(type_name) {
        return pyconf_check_sizeof(type_name)
}

function pyconf_check_type(type_name, headers, source, inc, cv, rc, define, n, arr, i) {
        define = _pyconf_type_to_define(type_name)
        cv = "ac_cv_type_" type_name
        gsub(/ /, "_", cv)

        if (cv in CACHE)
                return (CACHE[cv] == "yes")

        pyconf_checking("for " type_name)
        inc = _pyconf_ac_includes_default()
        if (headers != "") {
                n = split(headers, arr, " ")
                for (i = 1; i <= n; i++)
                        if (arr[i] != "")
                                inc = inc "#include <" arr[i] ">\n"
        }

        source = inc "\n" type_name " _test_var;\nint main(void) { return 0; }"
        rc = _pyconf_compile_test_cd(source, "")
        CACHE[cv] = rc ? "yes" : "no"
        V[cv] = rc ? "yes" : "no"
        pyconf_result(rc ? "yes" : "no")

        if (rc)
                pyconf_define(define, 1, 0, "Define to 1 if the system has the type `" type_name "`.")
        return rc
}

function pyconf_check_member(member, headers, define, struct_name, field, source, inc, cv, rc, n, arr, i) {
        # member is like "struct stat.st_blksize"
        split(member, arr, ".")
        struct_name = arr[1]
        field = arr[2]
        if (define == "") define = "HAVE_" toupper(struct_name) "_" toupper(field)
        gsub(/ /, "_", define)

        cv = "ac_cv_member_" member
        gsub(/[. ]/, "_", cv)

        if (cv in CACHE)
                return (CACHE[cv] == "yes")

        pyconf_checking("for " member)
        # Match Python version: use custom headers if provided, else use default headers
        if (headers != "") {
                n = split(headers, arr, " ")
                inc = ""
                for (i = 1; i <= n; i++)
                        if (arr[i] != "")
                                inc = inc "#include <" arr[i] ">\n"
        } else {
                inc = _pyconf_ac_includes_default()
        }

        source = inc "\nint main(void) { " struct_name " s; (void)s." field "; return 0; }"
        rc = _pyconf_compile_test_cd(source, "")
        CACHE[cv] = rc ? "yes" : "no"
        V[cv] = rc ? "yes" : "no"
        pyconf_result(rc ? "yes" : "no")

        if (rc)
                pyconf_define(define, 1, 0, "")
        return rc
}

function pyconf_check_members(members, n, arr, i) {
        n = split(members, arr, " ")
        for (i = 1; i <= n; i++)
                if (arr[i] != "")
                        pyconf_check_member(arr[i])
}

function pyconf_check_struct_tm() {
        # AC_STRUCT_TM — check for tm_zone in struct tm
        if (pyconf_check_member("struct tm.tm_zone", "time.h"))
                pyconf_define("HAVE_TM_ZONE", 1, 0, "Define to 1 if your struct tm has tm_zone.")
}

function pyconf_check_struct_timezone(    source, rc) {
        # Check for struct timezone
        source = "#include <sys/time.h>\nint main(void) { struct timezone tz; (void)tz; return 0; }\n"
        rc = _pyconf_compile_test(source, "")
        if (rc)
                pyconf_define("HAVE_STRUCT_TIMEZONE", 1, 0, "Define to 1 if struct timezone exists.")
}

function pyconf_check_c_const(    source) {
        # AC_C_CONST — check for const keyword
        source = "int main(void) {\n  const int x = 0;\n  (void)x;\n  return 0;\n}\n"
        if (!_pyconf_compile_test(source, ""))
                pyconf_define("const", "", 1, "Define to empty if const does not conform to ANSI C.")
}

function pyconf_check_define(macro, headers, source, inc, cv, rc, n, arr, i) {
        cv = "ac_cv_define_" macro
        if (cv in CACHE)
                return (CACHE[cv] == "yes")

        pyconf_checking("for " macro)
        inc = _pyconf_ac_includes_default()
        if (headers != "") {
                n = split(headers, arr, " ")
                for (i = 1; i <= n; i++)
                        if (arr[i] != "")
                                inc = inc "#include <" arr[i] ">\n"
        }
        source = inc "\n#ifndef " macro "\n#error " macro " not defined\n#endif\nint main(void) { return 0; }"
        rc = _pyconf_compile_test_cd(source, "")
        CACHE[cv] = rc ? "yes" : "no"
        V[cv] = rc ? "yes" : "no"
        pyconf_result(rc ? "yes" : "no")
        return rc
}

function pyconf_check_decl(name, includes, define, source, inc, cv, rc, _darr, _di, _dn) {
        if (define == "") define = "HAVE_DECL_" toupper(name)
        cv = "ac_cv_have_decl_" name
        if (cv in CACHE) {
                rc = (CACHE[cv] == "yes")
                pyconf_define(define, rc ? 1 : 0, 0, "")
                return rc
        }

        pyconf_checking("whether " name " is declared")
        inc = ""
        if (includes != "") {
                _dn = split(includes, _darr, " ")
                for (_di = 1; _di <= _dn; _di++)
                        if (_darr[_di] != "")
                                inc = inc "#include <" _darr[_di] ">\n"
        }
        source = inc "\nint main(void) {\n#ifndef " name "\n  (void)" name ";\n#endif\n  return 0;\n}"
        rc = _pyconf_compile_test_cd(source, "")
        CACHE[cv] = rc ? "yes" : "no"
        V[cv] = rc ? "yes" : "no"
        pyconf_result(rc ? "yes" : "no")
        pyconf_define(define, rc ? 1 : 0, 0, "")
        return rc
}

function pyconf_check_decls(decls, n, i, sep, headers) {
        # decls[0]=count, decls[1..n]=decl names or "--" separator followed by headers
        n = decls[0] + 0
        sep = 0
        headers = ""
        # Find "--" separator; everything after it is headers
        for (i = 1; i <= n; i++) {
                if (decls[i] == "--") {
                        sep = i
                        break
                }
        }
        if (sep > 0) {
                for (i = sep + 1; i <= n; i++)
                        if (decls[i] != "")
                                headers = headers (headers != "" ? " " : "") decls[i]
                n = sep - 1
        }
        for (i = 1; i <= n; i++)
                if (decls[i] != "")
                        pyconf_check_decl(decls[i], headers)
}

# ---------------------------------------------------------------------------
# Library checks
# ---------------------------------------------------------------------------

function pyconf_check_lib(lib, fname, extra_cflags, extra_libs, source, rc, flags, cache_key) {
        pyconf_checking("for -l" lib)
        if (fname == "") fname = "main"
        source = "char " fname "();\nint main(void) { return " fname "(); }"
        flags = extra_cflags " -l" lib " " extra_libs
        rc = _pyconf_link_test(source, flags)
        pyconf_result(rc ? "yes" : "no")
        # Set ac_cv_lib_<lib>_<fname> cache variable (autoconf convention)
        cache_key = "ac_cv_lib_" lib "_" fname
        CACHE[cache_key] = rc ? "yes" : "no"
        V[cache_key] = rc ? "yes" : "no"
        return rc
}

function pyconf_search_libs(fname, libs, n, arr, i, source, rc) {
        pyconf_checking("for library containing " fname)
        source = "char " fname "();\nint main(void) { return " fname "(); }"

        # Try without any library first
        if (_pyconf_link_test_cd(source, "")) {
                pyconf_result("none required")
                return "none required"
        }

        n = split(libs, arr, " ")
        for (i = 1; i <= n; i++) {
                if (arr[i] != "" && _pyconf_link_test_cd(source, "-l" arr[i])) {
                        V["LIBS"] = "-l" arr[i] " " V["LIBS"]
                        pyconf_result("-l" arr[i])
                        return "-l" arr[i]
                }
        }
        pyconf_result("no")
        return ""
}

# ---------------------------------------------------------------------------
# Program checks
# ---------------------------------------------------------------------------

# Portable PATH search — avoids reliance on "command -v" which may not be
# available on all /bin/sh implementations.  Splits $PATH and checks each
# directory for an executable file using "test -x".
function _find_in_path(prog, search_path, path, n, dirs, i, candidate) {
        if (prog == "") return ""
        # If prog already contains a slash, treat as a direct path
        if (index(prog, "/") > 0) {
                if (system("test -f " _shell_quote(prog) " && test -x " _shell_quote(prog) " 2>/dev/null") == 0)
                        return prog
                return ""
        }
        path = (search_path != "") ? search_path : ENVIRON["PATH"]
        n = split(path, dirs, ":")
        for (i = 1; i <= n; i++) {
                if (dirs[i] == "") dirs[i] = "."
                candidate = dirs[i] "/" prog
                if (system("test -f " _shell_quote(candidate) " && test -x " _shell_quote(candidate) " 2>/dev/null") == 0)
                        return candidate
        }
        return ""
}

function pyconf_check_prog(prog, search_path, default_val, result) {
        # Matches Python pyconf.check_prog(prog, path=None, default="")
        # Returns found path or default_val.
        result = _find_in_path(prog, search_path)
        if (result != "")
                return result
        return default_val
}

function pyconf_check_progs(progs, n, i) {
        # AC_CHECK_PROGS — return the first program NAME found in PATH.
        # Like the Python version (shutil.which(p) then return p), returns the
        # basename, not the full path.
        n = progs[0] + 0
        for (i = 1; i <= n; i++) {
                if (progs[i] != "" && _find_in_path(progs[i]) != "")
                        return progs[i]
        }
        return ""
}

function pyconf_check_tools(tools, default_val, result) {
        result = pyconf_check_progs(tools)
        if (result != "")
                return result
        return default_val
}

function pyconf_find_prog(prog, result) {
        result = _find_in_path(prog)
        return result
}

function pyconf_find_install(    result) {
        result = _find_in_path("ginstall")
        if (result == "")
                result = _find_in_path("install")
        if (result != "")
                SUBST["INSTALL"] = result " -c"
        else
                SUBST["INSTALL"] = "install -c"
        if (!("INSTALL_PROGRAM" in SUBST))
                SUBST["INSTALL_PROGRAM"] = "${INSTALL}"
        if (!("INSTALL_SCRIPT" in SUBST))
                SUBST["INSTALL_SCRIPT"] = "${INSTALL}"
        if (!("INSTALL_DATA" in SUBST))
                SUBST["INSTALL_DATA"] = "${INSTALL} -m 644"
        return result
}

function pyconf_find_mkdir_p(    result, ver, dirs, n, i, p, prog, progs, cmd) {
        # Search PATH + /opt/sfw/bin for coreutils/BusyBox mkdir
        result = ""
        n = split(ENVIRON["PATH"] ":/opt/sfw/bin", dirs, ":")
        for (i = 1; i <= n; i++) {
                if (dirs[i] == "") continue
                split("mkdir gmkdir", progs, " ")
                for (prog in progs) {
                        p = dirs[i] "/" progs[prog]
                        cmd = p " --version 2>&1"
                        ver = _cmd_output_oneline(cmd)
                        if (index(ver, "coreutils)") > 0 || index(ver, "BusyBox ") > 0 || index(ver, "fileutils) 4.1") > 0) {
                                result = p
                                break
                        }
                }
                if (result != "") break
        }
        if (result != "")
                SUBST["MKDIR_P"] = result " -p"
        else
                SUBST["MKDIR_P"] = "mkdir -p"
        return SUBST["MKDIR_P"]
}

# ---------------------------------------------------------------------------
# String / path utilities
# ---------------------------------------------------------------------------

function pyconf_fnmatch(string, pattern) {
        # Simple glob match using case
        # For exact match patterns, use == comparison
        # For patterns with *, use ~ with converted regex
        return _glob_match(pattern, string)
}

function pyconf_fnmatch_any(string, patterns, n, i) {
        n = patterns[0] + 0
        for (i = 1; i <= n; i++)
                if (_glob_match(patterns[i], string))
                        return 1
        return 0
}

function _glob_match(pattern, string, regex) {
        # Convert shell glob to awk regex
        regex = pattern
        gsub(/\./, "\\.", regex)
        gsub(/\*/, ".*", regex)
        gsub(/\?/, ".", regex)
        return (string ~ ("^" regex "$"))
}

function pyconf_str_remove(s, pat, result) {
        result = s
        gsub(pat, "", result)
        return result
}

function pyconf_sed(str, pattern, replacement, result) {
        result = str
        gsub(pattern, replacement, result)
        return result
}

function pyconf_abspath(path, result) {
        if (substr(path, 1, 1) == "/")
                return path
        result = _cmd_output_oneline("cd " _shell_quote(path) " && pwd")
        return (result != "" ? result : path)
}

function pyconf_basename(path, n, arr) {
        n = split(path, arr, "/")
        return arr[n]
}

function pyconf_getcwd() {
        return _cmd_output_oneline("pwd")
}

function pyconf_readlink(path) {
        return _cmd_output_oneline("readlink " _shell_quote(path))
}

function pyconf_relpath(path, base, n_path, n_base, p, b, i, common, result) {
        # Pure-AWK relative path computation (no external dependencies).
        # Default base to current working directory
        if (base == "") base = _cmd_output_oneline("pwd")
        # Normalize: strip trailing slashes
        gsub(/\/+$/, "", path)
        gsub(/\/+$/, "", base)
        # Split into components
        n_path = split(path, p, "/")
        n_base = split(base, b, "/")
        # Find common prefix length
        common = 0
        for (i = 1; i <= (n_path < n_base ? n_path : n_base); i++) {
                if (p[i] != b[i]) break
                common = i
        }
        # Build result: "../" for each remaining base component, then path tail
        result = ""
        for (i = common + 1; i <= n_base; i++)
                result = result (result != "" ? "/" : "") ".."
        for (i = common + 1; i <= n_path; i++)
                result = result (result != "" ? "/" : "") p[i]
        if (result == "") result = "."
        return result
}

function pyconf_path_join(parts, n, i, result) {
        n = parts[0] + 0
        if (n == 0) return ""
        result = parts[1]
        for (i = 2; i <= n; i++) {
                if (parts[i] == "") continue
                if (substr(parts[i], 1, 1) == "/") {
                        result = parts[i]
                } else if (result == "" || result == ".") {
                        result = parts[i]
                } else {
                        result = result "/" parts[i]
                }
        }
        return result
}

function pyconf_path_parent(path, n, arr, i, result) {
        n = split(path, arr, "/")
        if (n <= 1) return "."
        result = arr[1]
        for (i = 2; i < n; i++)
                result = result "/" arr[i]
        return result
}

function pyconf_path_exists(path) {
        return (_cmd_exit_code("test -e " _shell_quote(path)) == 0)
}

function pyconf_path_is_dir(path) {
        return (_cmd_exit_code("test -d " _shell_quote(path)) == 0)
}

function pyconf_path_is_file(path) {
        return (_cmd_exit_code("test -f " _shell_quote(path)) == 0)
}

function pyconf_path_is_symlink(path) {
        return (_cmd_exit_code("test -L " _shell_quote(path)) == 0)
}

function pyconf_is_executable(path) {
        return (_cmd_exit_code("test -x " _shell_quote(path)) == 0)
}

function pyconf_rm_f(path) {
        system("rm -f " _shell_quote(path))
}

function pyconf_rmdir(path) {
        system("rmdir " _shell_quote(path) " 2>/dev/null")
}

function pyconf_mkdir_p(path) {
        system("mkdir -p " _shell_quote(path))
}

function pyconf_rename_file(src, dst) {
        system("mv " _shell_quote(src) " " _shell_quote(dst))
}

function pyconf_unlink(path) {
        system("rm -f " _shell_quote(path))
}

function pyconf_write_file(path, content) {
        printf "%s", content > path
        close(path)
}

function pyconf_read_file(path, line, result) {
        result = ""
        while ((getline line < path) > 0)
                result = result (result != "" ? "\n" : "") line
        close(path)
        return result
}

function pyconf_read_file_lines(path) {
        return pyconf_read_file(path)
}

function pyconf_glob_files(pattern) {
        return _cmd_output("ls -d " pattern " 2>/dev/null")
}

function pyconf_cmd(args, cmd) {
        cmd = _arr_join_quoted(args, " ")
        return (_cmd_exit_code(cmd) == 0)
}

function pyconf_cmd_output(args, cmd) {
        cmd = _arr_join_quoted(args, " ")
        return _cmd_output(cmd)
}

function pyconf_run_cmd(cmd, rc) {
        rc = system(_expand_cmd_vars(cmd))
        return rc
}

function pyconf_shell(args, cmd) {
        cmd = _arr_join(args, " ")
        return _cmd_output(cmd)
}

function pyconf_shell_export(cmd, varname, full_cmd, output, n, lines, i, eq, k) {
        # Run cmd then echo varname=${varname}; parse output for varname=value
        full_cmd = cmd "\necho " varname "=${" varname "}"
        output = _cmd_output(full_cmd)
        n = split(output, lines, "\n")
        for (i = n; i >= 1; i--) {
                eq = index(lines[i], "=")
                if (eq > 0) {
                        k = substr(lines[i], 1, eq - 1)
                        if (k == varname)
                                return substr(lines[i], eq + 1)
                }
        }
        return ""
}

function pyconf_platform_machine() {
        return _cmd_output_oneline("uname -m")
}

function pyconf_platform_system() {
        return _cmd_output_oneline("uname -s")
}

function pyconf_use_system_extensions() {
        pyconf_define("_ALL_SOURCE", 1, 0, "Enable extensions on AIX 3, Interix.")
        pyconf_define("_DARWIN_C_SOURCE", 1, 0, "Enable extensions on Mac OS X.")
        pyconf_define("_GNU_SOURCE", 1, 0, "Enable GNU extensions on systems that have them.")
        pyconf_define("_HPUX_ALT_XOPEN_SOCKET_API", 1, 0, "Enable extensions on HP-UX.")
        pyconf_define("_NETBSD_SOURCE", 1, 0, "Enable extensions on NetBSD.")
        pyconf_define("_OPENBSD_SOURCE", 1, 0, "Enable extensions on OpenBSD.")
        pyconf_define("_POSIX_PTHREAD_SEMANTICS", 1, 0, "Enable threading extensions on Solaris.")
        pyconf_define("__STDC_WANT_IEC_60559_ATTRIBS_EXT__", 1, 0, "Enable IEC 60559 attribs extensions.")
        pyconf_define("__STDC_WANT_IEC_60559_BFP_EXT__", 1, 0, "Enable IEC 60559 BFP extensions.")
        pyconf_define("__STDC_WANT_IEC_60559_DFP_EXT__", 1, 0, "Enable IEC 60559 DFP extensions.")
        pyconf_define("__STDC_WANT_IEC_60559_EXT__", 1, 0, "Enable IEC 60559 extensions.")
        pyconf_define("__STDC_WANT_IEC_60559_FUNCS_EXT__", 1, 0, "Enable IEC 60559 funcs extensions.")
        pyconf_define("__STDC_WANT_IEC_60559_TYPES_EXT__", 1, 0, "Enable IEC 60559 types extensions.")
        pyconf_define("__STDC_WANT_LIB_EXT2__", 1, 0, "Enable ISO C 2001 extensions.")
        pyconf_define("__STDC_WANT_MATH_SPEC_FUNCS__", 1, 0, "Enable extensions specified in the standard.")
        pyconf_define("_TANDEM_SOURCE", 1, 0, "Enable extensions on HP NonStop.")
        pyconf_define("__EXTENSIONS__", 1, 0, "Enable general extensions on Solaris.")
}

# ---------------------------------------------------------------------------
# Environment save/restore
# ---------------------------------------------------------------------------

function pyconf_save_env(    k) {
        _saved_env_depth++
        # Store values and keys using indexed arrays only — no string
        # concatenation.  Building a long key-list string by repeated
        # concatenation triggers memory corruption in mawk when V[]
        # has hundreds of entries (as on CI runners with large ENVIRONs).
        _saved_env_key_n[_saved_env_depth] = 0
        for (k in V) {
                _saved_env_stack[_saved_env_depth, k] = V[k]
                _saved_env_key_n[_saved_env_depth]++
                _saved_env_key_name[_saved_env_depth, _saved_env_key_n[_saved_env_depth]] = k
        }
}

function pyconf_restore_env(    k, n, i) {
        if (_saved_env_depth < 1) return
        for (k in V)
                delete V[k]
        n = _saved_env_key_n[_saved_env_depth]
        for (i = 1; i <= n; i++) {
                k = _saved_env_key_name[_saved_env_depth, i]
                V[k] = _saved_env_stack[_saved_env_depth, k]
                delete _saved_env_stack[_saved_env_depth, k]
                delete _saved_env_key_name[_saved_env_depth, i]
        }
        delete _saved_env_key_n[_saved_env_depth]
        _saved_env_depth--
}

# ---------------------------------------------------------------------------
# Option handling
# ---------------------------------------------------------------------------

function _pyconf_normalize_opt(name, result) {
        result = name
        gsub(/-/, "_", result)
        return result
}

function pyconf_arg_enable(name, default_val, help_text, metavar, var, false_is, key) {
        key = "enable_" _pyconf_normalize_opt(name)
        _pyconf_opt_count++
        _pyconf_opt_names[_pyconf_opt_count] = key
        OPT_KIND[key] = "enable"
        OPT_DEFAULT[key] = default_val
        OPT_HELP[key] = help_text
        OPT_METAVAR[key] = metavar
        OPT_VAR[key] = var
        OPT_FALSE_IS[key] = false_is
        if (!(key in OPT_GIVEN))
                OPT_GIVEN[key] = 0
        if (!(key in OPT_VAL))
                OPT_VAL[key] = default_val
}

function pyconf_arg_with(name, default_val, help_text, metavar, var, false_is, key) {
        key = "with_" _pyconf_normalize_opt(name)
        _pyconf_opt_count++
        _pyconf_opt_names[_pyconf_opt_count] = key
        OPT_KIND[key] = "with"
        OPT_DEFAULT[key] = default_val
        OPT_HELP[key] = help_text
        OPT_METAVAR[key] = metavar
        OPT_VAR[key] = var
        OPT_FALSE_IS[key] = false_is
        if (!(key in OPT_GIVEN))
                OPT_GIVEN[key] = 0
        if (!(key in OPT_VAL))
                OPT_VAL[key] = default_val
}

function pyconf_env_var(name, help_text) {
        # Seed V[name] from the environment if not already set, mirroring
        # Python's Vars.__getattr__ fallback to os.environ.
        if (!(name in V) && name in ENV)
                V[name] = ENV[name]
}

function pyconf_option_value(key) {
        if (key in OPT_VAL)
                return OPT_VAL[key]
        if (key in OPT_DEFAULT)
                return OPT_DEFAULT[key]
        return ""
}

function pyconf_option_given(key) {
        return (key in OPT_GIVEN && OPT_GIVEN[key])
}

function pyconf_option_is_yes(key, val) {
        val = pyconf_option_value(key)
        return (val == "yes")
}

function pyconf_option_is_no(key, val) {
        val = pyconf_option_value(key)
        return (val == "no")
}

function pyconf_option_value_or(key, default_val, val) {
        val = pyconf_option_value(key)
        if (val == "") return default_val
        return val
}

function pyconf_option_process_bool(key, val, false_is) {
        val = pyconf_option_value(key)
        false_is = OPT_FALSE_IS[key]
        if (val == "yes") return "yes"
        if (val == "no") return (false_is != "" ? false_is : "no")
        return val
}

function pyconf_option_process_value(key) {
        return pyconf_option_value(key)
}

# ---------------------------------------------------------------------------
# pkg-config
# ---------------------------------------------------------------------------

function pyconf_pkg_check_modules(pkg, spec, cmd, rc) {
        cmd = "pkg-config --exists " _shell_quote(spec) " 2>/dev/null"
        rc = (system(cmd) == 0)
        if (rc) {
                V[pkg "_CFLAGS"] = _cmd_output_oneline("pkg-config --cflags " _shell_quote(spec))
                V[pkg "_LIBS"] = _cmd_output_oneline("pkg-config --libs " _shell_quote(spec))
        }
        return rc
}

function pyconf_pkg_config(args) {
        return _cmd_output_oneline("pkg-config " args " 2>/dev/null")
}

function pyconf_pkg_config_check(spec) {
        return (_cmd_exit_code("pkg-config --exists " _shell_quote(spec) " 2>/dev/null") == 0)
}

# ---------------------------------------------------------------------------
# stdlib module handling
# ---------------------------------------------------------------------------

function pyconf_stdlib_module(name, supported, enabled, cflags, ldflags, has_cflags, has_ldflags, key, state, uname, prev_na) {
        uname = toupper(name)
        gsub(/-/, "_", uname)
        key = "MODULE_" uname
        # Check if this module was previously marked N/A via stdlib_module_set_na
        prev_na = (_stdlib_mod_na[uname] + 0)
        if (prev_na) {
                supported = "no"
        }
        if (prev_na)
                state = "n/a"
        else if (enabled != "yes")
                state = "disabled"
        else if (supported != "yes")
                state = "missing"
        else
                state = "yes"
        SUBST[key "_STATE"] = state
        SUBST[key "_TRUE"] = (state == "yes") ? "" : "#"
        if (has_cflags == "yes")
                SUBST[key "_CFLAGS"] = cflags
        if (has_ldflags == "yes")
                SUBST[key "_LDFLAGS"] = ldflags
        _stdlib_mod_count++
        _stdlib_mod_names[_stdlib_mod_count] = uname
        _stdlib_mod_has_cflags[uname] = has_cflags
        _stdlib_mod_has_ldflags[uname] = has_ldflags
}

function pyconf_stdlib_module_simple(name, cflags, ldflags, has_cflags, has_ldflags) {
        pyconf_stdlib_module(name, "yes", "yes", cflags, ldflags, has_cflags, has_ldflags)
}

function pyconf_stdlib_module_set_na(names, n, i, key, uname) {
        n = names[0] + 0
        for (i = 1; i <= n; i++) {
                uname = toupper(names[i])
                key = "MODULE_" uname
                SUBST[key "_STATE"] = "n/a"
                SUBST[key "_TRUE"] = "#"
                _stdlib_mod_na[uname] = 1
        }
}

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

function _pyconf_arg_takes_value(key) {
        # These flags accept a space-separated value (--prefix /path),
        # matching autoconf behaviour.  Covers standard directory vars
        # (_DIR_VARS in pyconf.py), srcdir, cache-file, and system type
        # triplets (host, build, target).
        return (key == "prefix" || key == "exec-prefix" || \
                key == "bindir" || key == "sbindir" || \
                key == "libexecdir" || key == "sysconfdir" || \
                key == "sharedstatedir" || key == "localstatedir" || \
                key == "runstatedir" || key == "libdir" || \
                key == "includedir" || key == "oldincludedir" || \
                key == "datarootdir" || key == "datadir" || \
                key == "infodir" || key == "localedir" || \
                key == "mandir" || key == "docdir" || \
                key == "htmldir" || key == "dvidir" || \
                key == "pdfdir" || key == "psdir" || \
                key == "srcdir" || key == "cache-file" || \
                key == "host" || key == "build" || key == "target")
}

function pyconf_parse_args(    i, arg, key, val, opt_key, eq_pos, config_args) {
        config_args = ""
        for (i = 1; i < ARGC; i++) {
                arg = ARGV[i]
                if (arg == "--") break

                if (substr(arg, 1, 2) == "--") {
                        # Record in CONFIG_ARGS (skip --srcdir which is internal)
                        if (substr(arg, 3, 6) != "srcdir")
                                config_args = config_args (config_args != "" ? " " : "") _shell_quote(arg)

                        arg = substr(arg, 3)
                        eq_pos = index(arg, "=")
                        if (eq_pos > 0) {
                                key = substr(arg, 1, eq_pos - 1)
                                val = substr(arg, eq_pos + 1)
                        } else {
                                key = arg
                                # Directory args and --srcdir accept a
                                # space-separated value (--prefix /path),
                                # matching autoconf behaviour.
                                if (_pyconf_arg_takes_value(key) && (i + 1) < ARGC) {
                                        i++
                                        val = ARGV[i]
                                        # Record the value in config_args too
                                        if (key != "srcdir")
                                                config_args = config_args " " _shell_quote(val)
                                        ARGV[i] = ""
                                } else {
                                        val = "yes"
                                }
                        }

                        # --enable-X / --disable-X
                        if (substr(key, 1, 7) == "enable-") {
                                opt_key = "enable_" _pyconf_normalize_opt(substr(key, 8))
                                OPT_VAL[opt_key] = val
                                OPT_GIVEN[opt_key] = 1
                        } else if (substr(key, 1, 8) == "disable-") {
                                opt_key = "enable_" _pyconf_normalize_opt(substr(key, 9))
                                OPT_VAL[opt_key] = "no"
                                OPT_GIVEN[opt_key] = 1
                        # --with-X / --without-X
                        } else if (substr(key, 1, 5) == "with-") {
                                opt_key = "with_" _pyconf_normalize_opt(substr(key, 6))
                                OPT_VAL[opt_key] = val
                                OPT_GIVEN[opt_key] = 1
                        } else if (substr(key, 1, 8) == "without-") {
                                opt_key = "with_" _pyconf_normalize_opt(substr(key, 9))
                                OPT_VAL[opt_key] = "no"
                                OPT_GIVEN[opt_key] = 1
                        # --prefix, --exec-prefix, --bindir, etc.
                        } else if (key == "srcdir") {
                                _pyconf_srcdir = val
                                pyconf__srcdir_arg = val
                        } else if (key == "help" || key == "h") {
                                _pyconf_help_requested = 1
                        } else {
                                # Store as a dir arg or general option
                                V[_pyconf_normalize_opt(key)] = val
                        }
                } else {
                        # Positional args: VAR=value sets environment + V[],
                        # mirroring Python's pyconf.parse_args() which does
                        # os.environ[name] = value.
                        config_args = config_args (config_args != "" ? " " : "") _shell_quote(arg)
                        eq_pos = index(arg, "=")
                        if (eq_pos > 0) {
                                key = substr(arg, 1, eq_pos - 1)
                                if (match(key, /^[A-Za-z_][A-Za-z0-9_]*$/)) {
                                        val = substr(arg, eq_pos + 1)
                                        V[key] = val
                                        ENV[key] = val
                                        MODIFIED_ENV[key] = 1
                                }
                        }
                }
                # Clear ARGV so awk doesn't try to read as files
                ARGV[i] = ""
        }
        _pyconf_config_args = config_args
}

function pyconf_check_help() {
        if (_pyconf_help_requested) {
                pyconf_print_help()
                return 1
        }
        return 0
}

function pyconf_print_help(    i, key, kind, name, help_text, metavar) {
        print "Usage: configure [options]"
        print ""
        print "Options:"
        print "  --help            display this help and exit"
        print ""

        for (i = 1; i <= _pyconf_opt_count; i++) {
                key = _pyconf_opt_names[i]
                kind = OPT_KIND[key]
                help_text = OPT_HELP[key]
                metavar = OPT_METAVAR[key]

                # Extract the name from key (remove enable_/with_ prefix)
                if (kind == "enable")
                        name = substr(key, 8)  # skip "enable_"
                else
                        name = substr(key, 6)  # skip "with_"
                gsub(/_/, "-", name)

                if (kind == "enable") {
                        if (metavar != "")
                                printf "  --enable-%-20s %s\n", name "=" metavar, help_text
                        else
                                printf "  --enable-%-20s %s\n", name, help_text
                } else {
                        if (metavar != "")
                                printf "  --with-%-22s %s\n", name "=" metavar, help_text
                        else
                                printf "  --with-%-22s %s\n", name, help_text
                }
        }
}

function pyconf_get_dir_arg(name, default, key) {
        key = _pyconf_normalize_opt(name)
        if ((key in V) && V[key] != "")
                return V[key]
        return default
}

# ---------------------------------------------------------------------------
# Config file output
# ---------------------------------------------------------------------------

function pyconf_config_files(files, n, i) {
        n = files[0] + 0
        for (i = 1; i <= n; i++) {
                if (files[i] != "") {
                        _pyconf_config_file_count++
                        CONFIG_FILES[_pyconf_config_file_count] = files[i]
                }
        }
}

function pyconf_config_files_x(files, n, i) {
        n = files[0] + 0
        for (i = 1; i <= n; i++) {
                if (files[i] != "") {
                        _pyconf_config_file_count++
                        CONFIG_FILES[_pyconf_config_file_count] = files[i]
                        _pyconf_config_file_x_count++
                        CONFIG_FILES_X[_pyconf_config_file_x_count] = files[i]
                }
        }
}

function pyconf_output(    i) {
        _pyconf_build_module_block()
        _pyconf_resolve_exports()
        _pyconf_write_pyconfig_h()
        _pyconf_process_config_files()
        _pyconf_write_config_status()
        pyconf_cleanup()
}

function _pyconf_build_module_block(    i, key, uname, state, block) {
        # Populate _module_block_lines array instead of concatenating into a string.
        # This avoids building a large concatenated string in AWK memory, which causes
        # memory corruption in mawk when many modules have CFLAGS/LDFLAGS.
        _module_block_n = 0
        for (i = 1; i <= _stdlib_mod_count; i++) {
                uname = _stdlib_mod_names[i]
                key = "MODULE_" uname
                state = SUBST[key "_STATE"]
                _module_block_lines[++_module_block_n] = key "_STATE=" state
                if (_stdlib_mod_has_cflags[uname] == "yes" && state != "disabled" && state != "n/a" && state != "missing") {
                        _module_block_lines[++_module_block_n] = key "_CFLAGS=" SUBST[key "_CFLAGS"]
                }
                if (_stdlib_mod_has_ldflags[uname] == "yes" && state != "disabled" && state != "n/a" && state != "missing") {
                        _module_block_lines[++_module_block_n] = key "_LDFLAGS=" SUBST[key "_LDFLAGS"]
                }
        }
        # Also populate SUBST["MODULE_BLOCK"] so config.status / regen_status.awk
        # can regenerate Makefile.pre correctly (it uses plain @VAR@ substitution
        # from serialized SUBST data, not _module_block_lines[]).
        block = ""
        for (i = 1; i <= _module_block_n; i++) {
                if (i > 1) block = block "\n"
                block = block _module_block_lines[i]
        }
        SUBST["MODULE_BLOCK"] = block
}

function _pyconf_resolve_exports(    k) {
        # Finalize CONFIG_ARGS from parsed command-line arguments
        if (_pyconf_config_args != "")
                V["CONFIG_ARGS"] = _pyconf_config_args
        for (k in V) {
                if (!(k in SUBST))
                        SUBST[k] = V[k]
        }
}

function _pyconf_write_pyconfig_h(    f, inf, line, name, val, indent, rest, outf) {
        outf = "pyconfig.h"
        inf = _pyconf_srcdir "/pyconfig.h.in"

        # Try template-based approach (matches autoconf config.status behaviour)
        if ((getline line < inf) > 0) {
                # First line was read successfully — process the template
                printf "/* pyconfig.h.  Generated from pyconfig.h.in by configure.  */\n" > outf
                do {
                        if (match(line, /^[ \t]*#[ \t]*undef[ \t]+[A-Za-z_][A-Za-z_0-9]*[ \t]*$/)) {
                                # Extract indent: everything before the '#'
                                indent = ""
                                rest = line
                                if (match(line, /^[ \t]+/))
                                        indent = substr(line, 1, RLENGTH)
                                # Extract the define name: last word on the line
                                name = line
                                sub(/^[ \t]*#[ \t]*undef[ \t]+/, "", name)
                                sub(/[ \t]*$/, "", name)
                                if (name in DEFINES) {
                                        val = DEFINES[name]
                                        if (DEFINE_QUOTED[name] && val != "" && val !~ /^-?[0-9]+$/)
                                                rest = "#define " name " \"" val "\""
                                        else
                                                rest = "#define " name " " val
                                        if (indent != "")
                                                printf "%s# %s\n", indent, substr(rest, 2) >> outf
                                        else
                                                printf "%s\n", rest >> outf
                                } else {
                                        if (indent != "")
                                                printf "%s/* #undef %s */\n", indent, name >> outf
                                        else
                                                printf "/* #undef %s */\n", name >> outf
                                }
                        } else {
                                print line >> outf
                        }
                } while ((getline line < inf) > 0)
                close(inf)
                close(outf)
                return
        }
        close(inf)

        # Fallback: write from scratch (no template available)
        printf "/* Generated by configure (awk) */\n" > outf
        printf "#ifndef Py_PYCONFIG_H\n" >> outf
        printf "#define Py_PYCONFIG_H\n\n" >> outf

        for (f = 1; f <= _pyconf_defines_order_n; f++) {
                name = _pyconf_defines_order[f]
                if (name in DEFINES) {
                        val = DEFINES[name]
                        if (name in DEFINE_DESC && DEFINE_DESC[name] != "")
                                printf "/* %s */\n", DEFINE_DESC[name] >> outf
                        if (DEFINE_QUOTED[name] && val != "" && val !~ /^-?[0-9]+$/)
                                printf "#define %s \"%s\"\n", name, val >> outf
                        else
                                printf "#define %s %s\n", name, val >> outf
                        printf "\n" >> outf
                }
        }

        printf "#endif /* Py_PYCONFIG_H */\n" >> outf
        close(outf)
}

function _pyconf_process_config_files(    i, inf, outf, outdir, saved_abs_srcdir, saved_abs_builddir, slash_pos, sd) {
        # Determine srcdir: prefer the transpiled pyconf_srcdir (set by
        # u_setup_source_dirs), fall back to _pyconf_srcdir (from --srcdir arg).
        sd = _pyconf_srcdir
        if (pyconf_srcdir != "" && pyconf_srcdir != ".")
                sd = pyconf_srcdir
        for (i = 1; i <= _pyconf_config_file_count; i++) {
                outf = CONFIG_FILES[i]
                if (sd != "" && sd != ".")
                        inf = sd "/" outf ".in"
                else
                        inf = outf ".in"
                # Skip if template does not exist (mirrors pyconf.py behaviour)
                if (system("test -f " _shell_quote(inf)) != 0) {
                        pyconf_warn("config file template not found: " inf)
                        continue
                }
                # Compute per-file abs_srcdir/abs_builddir like autoconf does:
                # ac_abs_srcdir = abs_top_srcdir + dir-suffix-of-outfile
                outdir = ""
                slash_pos = _last_index(outf, "/")
                if (slash_pos > 0)
                        outdir = substr(outf, 1, slash_pos - 1)
                saved_abs_srcdir = SUBST["abs_srcdir"]
                saved_abs_builddir = SUBST["abs_builddir"]
                if (outdir != "") {
                        SUBST["abs_srcdir"] = saved_abs_srcdir "/" outdir
                        SUBST["abs_builddir"] = saved_abs_builddir "/" outdir
                        system("mkdir -p " _shell_quote(outdir))
                }
                _pyconf_subst_file(inf, outf)
                SUBST["abs_srcdir"] = saved_abs_srcdir
                SUBST["abs_builddir"] = saved_abs_builddir
        }
        for (i = 1; i <= _pyconf_config_file_x_count; i++) {
                if (system("test -f " _shell_quote(CONFIG_FILES_X[i])) == 0)
                        system("chmod +x " _shell_quote(CONFIG_FILES_X[i]))
        }
}

function _pyconf_write_config_status(    outf, config_args, sd, k, val, i, cf, cfx, sep) {
        outf = "config.status"
        config_args = SUBST["CONFIG_ARGS"]
        if (config_args == "")
                config_args = _pyconf_config_args
        sd = SUBST["srcdir"]
        if (sd == "")
                sd = "."

        # Save config file lists into SUBST so they get serialized.
        cf = ""
        sep = ""
        for (i = 1; i <= _pyconf_config_file_count; i++) {
                cf = cf sep CONFIG_FILES[i]
                sep = " "
        }
        SUBST["_config_files"] = cf
        cfx = ""
        sep = ""
        for (i = 1; i <= _pyconf_config_file_x_count; i++) {
                cfx = cfx sep CONFIG_FILES_X[i]
                sep = " "
        }
        SUBST["_config_files_x"] = cfx

        # Write config.status as a thin shell wrapper.
        # --recheck re-runs configure; everything else delegates to
        # regen_status.awk which reads saved data from after the
        # @CONFIG_STATUS_DATA@ marker at the end of this file.
        printf "#!/bin/sh\n" > outf
        printf "# Generated by configure.  Do not edit.\n" >> outf
        printf "#\n" >> outf
        printf "# This script can regenerate output files from templates\n" >> outf
        printf "# without re-running the full configure.\n" >> outf
        printf "\n" >> outf
        printf "CONFIG_ARGS='%s'\n", config_args >> outf
        printf "SRCDIR='%s'\n", sd >> outf
        printf "\n" >> outf
        printf "for arg in \"$@\"; do\n" >> outf
        printf "  case \"$arg\" in\n" >> outf
        printf "    --recheck)\n" >> outf
        printf "      echo \"running configure with args: $CONFIG_ARGS\"\n" >> outf
        printf "      exec \"$SRCDIR\"/configure $CONFIG_ARGS\n" >> outf
        printf "      ;;\n" >> outf
        printf "  esac\n" >> outf
        printf "done\n" >> outf
        printf "\n" >> outf
        # Emit _find_awk: prefer mawk/nawk, avoid gawk crash bug.
        printf "_find_awk() {\n" >> outf
        printf "    for _c in mawk nawk awk; do\n" >> outf
        printf "        IFS=:\n" >> outf
        printf "        for _d in $PATH; do\n" >> outf
        printf "            if [ -x \"${_d}/${_c}\" ]; then\n" >> outf
        printf "                case $(\"${_d}/${_c}\" --version < /dev/null 2>&1) in\n" >> outf
        printf "                    *\"GNU Awk\"*) ;;\n" >> outf
        printf "                    *) echo \"${_d}/${_c}\"; return 0 ;;\n" >> outf
        printf "                esac\n" >> outf
        printf "            fi\n" >> outf
        printf "        done\n" >> outf
        printf "        unset IFS\n" >> outf
        printf "    done\n" >> outf
        printf "    echo awk\n" >> outf
        printf "}\n" >> outf
        printf "_awk=$(_find_awk)\n" >> outf
        printf "\n" >> outf
        printf "exec \"$_awk\" -f \"$SRCDIR\"/Tools/configure/transpiler/regen_status.awk \\\n" >> outf
        printf "    -v cs_args=\"$*\" \\\n" >> outf
        printf "    -v cs_config_files=\"${CONFIG_FILES-@UNSET@}\" \\\n" >> outf
        printf "    -v cs_config_headers=\"${CONFIG_HEADERS-@UNSET@}\" \\\n" >> outf
        printf "    -v srcdir=\"$SRCDIR\" \\\n" >> outf
        printf "    \"$0\"\n" >> outf
        printf "\n" >> outf
        printf "exit 0\n" >> outf

        # Data section: the shell never reaches here (exit above).
        # regen_status.awk scans for @CONFIG_STATUS_DATA@ to find this data.
        printf "@CONFIG_STATUS_DATA@\n" >> outf
        printf "@SUBST@\n" >> outf
        for (k in SUBST) {
                val = SUBST[k]
                _cs_write_data_entry(outf, k, val)
        }
        printf "@DEFINES@\n" >> outf
        for (i = 1; i <= _pyconf_defines_order_n; i++) {
                k = _pyconf_defines_order[i]
                if (k in DEFINES) {
                        printf "%s\n", k >> outf
                        printf "%s\n", DEFINES[k] >> outf
                        printf "%s\n", (k in DEFINE_DESC ? DEFINE_DESC[k] : "") >> outf
                        printf "%s\n", (DEFINE_QUOTED[k] ? "1" : "0") >> outf
                }
        }
        printf "@END@\n" >> outf

        close(outf)
        system("chmod +x " _shell_quote(outf))
}

# Write a SUBST entry: key, line_count, value lines
function _cs_write_data_entry(outf, key, val,    n, lines, i) {
        n = split(val, lines, "\n")
        if (n == 0) n = 1
        printf "%s\n%d\n", key, n >> outf
        if (val == "") {
                printf "\n" >> outf
        } else {
                for (i = 1; i <= n; i++)
                        printf "%s\n", lines[i] >> outf
        }
}


function _last_index(s, ch, i, last) {
        last = 0
        for (i = 1; i <= length(s); i++)
                if (substr(s, i, 1) == ch)
                        last = i
        return last
}

function _pyconf_subst_file(inf, outf, line, k, pat, val, pos, before, after, skip, i, b, a) {
        while ((getline line < inf) > 0) {
                skip = 0
                # Handle @MODULE_BLOCK@ specially: emit line-by-line to avoid
                # building a large concatenated string in AWK memory (mawk safety).
                if (index(line, "@MODULE_BLOCK@") > 0) {
                        pos = index(line, "@MODULE_BLOCK@")
                        before = substr(line, 1, pos - 1)
                        after = substr(line, pos + length("@MODULE_BLOCK@"))
                        if (_module_block_n > 0) {
                                for (i = 1; i <= _module_block_n; i++) {
                                        b = (i == 1) ? before : ""
                                        a = (i == _module_block_n) ? after : ""
                                        printf "%s%s%s\n", b, _module_block_lines[i], a > outf
                                }
                        } else {
                                # No modules, just print the before and after parts
                                printf "%s%s\n", before, after > outf
                        }
                        skip = 1
                }
                if (!skip) {
                        # Replace @VAR@ patterns with SUBST values
                        for (k in SUBST) {
                                pat = "@" k "@"
                                while (index(line, pat) > 0) {
                                        val = SUBST[k]
                                        pos = index(line, pat)
                                        before = substr(line, 1, pos - 1)
                                        after = substr(line, pos + length(pat))
                                        # If the value contains newlines, emit directly and skip normal print
                                        if (index(val, "\n") > 0) {
                                                printf "%s%s%s\n", before, val, after > outf
                                                skip = 1
                                                break
                                        } else {
                                                line = before val after
                                        }
                                }
                                if (skip) break
                        }
                }
                if (!skip) {
                        # Neutralise VPATH when srcdir == "." (in-tree build):
                        # blank out lines like "VPATH=  ." or "VPATH= $(srcdir)"
                        if (SUBST["srcdir"] == "." && _is_vpath_srcdir_line(line))
                                print "" > outf
                        else
                                print line > outf
                }
        }
        close(inf)
        close(outf)
}

function _is_vpath_srcdir_line(line, val) {
        # Match lines like: VPATH=<whitespace><value>
        # where value is ".", "$(srcdir)", or "${srcdir}"
        if (!match(line, /^[ \t]*VPATH[ \t]*=[ \t]*/))
                return 0
        val = substr(line, RSTART + RLENGTH)
        # Strip leading/trailing whitespace
        gsub(/^[ \t]+|[ \t]+$/, "", val)
        if (val == "." || val == "$(srcdir)" || val == "${srcdir}")
                return 1
        return 0
}

# ---------------------------------------------------------------------------
# Run helpers
# ---------------------------------------------------------------------------

function pyconf_run(cmd) {
        system(_expand_cmd_vars(cmd))
}

function pyconf_run_capture(cmd, ecmd, outf, rc) {
        ecmd = _expand_cmd_vars(cmd)
        outf = _pyconf_tmpdir "/stdout_capture"
        rc = system(ecmd " > " outf " 2>/dev/null")
        _pyconf_run_cmd_stdout = _cmd_output("cat " outf)
        _pyconf_run_cmd_stderr = ""
        system("rm -f " outf)
        return rc
}

function pyconf_run_capture_input(cmd, input_data, tmpf, outf, ecmd, rc) {
        ecmd = _expand_cmd_vars(cmd)
        tmpf = _pyconf_tmpdir "/stdin_input"
        outf = _pyconf_tmpdir "/stdout_capture"
        printf "%s", input_data > tmpf
        close(tmpf)
        rc = system(ecmd " < " tmpf " > " outf " 2>/dev/null")
        _pyconf_run_cmd_stdout = _cmd_output("cat " outf)
        _pyconf_run_cmd_stderr = ""
        system("rm -f " tmpf " " outf)
        return rc
}

function pyconf_format_yn(val) {
        if (val == "yes" || val == 1 || val == "true")
                return "yes"
        return "no"
}

function pyconf_run_check_with_cc_flag(desc, flag, source, saved, rc) {
        saved = V["CFLAGS"]
        V["CFLAGS"] = V["CFLAGS"] " " flag
        rc = pyconf_run_check(desc, source, "", "")
        V["CFLAGS"] = saved
        return rc
}

function pyconf_macro(name, result) {
        # Placeholder for macro expansion
        return ""
}

function pyconf_cmd_status(args, result_arr, cmd) {
        cmd = _arr_join_quoted(args, " ")
        result_arr[0] = system(cmd " > " _pyconf_tmpdir "/cmd_out 2>&1")
        result_arr[1] = _cmd_output("cat " _pyconf_tmpdir "/cmd_out")
        system("rm -f " _pyconf_tmpdir "/cmd_out")
}

function pyconf_canonical_host(    result, guess, csub, parts, n, host_arg, build_arg, canon) {
        guess = _pyconf_srcdir "/config.guess"
        csub = _pyconf_srcdir "/config.sub"

        # Check for --host and --build from parsed args (stored by pyconf_parse_args)
        host_arg = V["host"]
        build_arg = V["build"]

        # Determine build triplet
        if (build_arg != "") {
                canon = _cmd_output_oneline(csub " " build_arg " 2>/dev/null")
                pyconf_build = (canon != "") ? canon : build_arg
        } else {
                result = _cmd_output_oneline(guess " 2>/dev/null")
                if (result != "") {
                        # Canonicalize through config.sub if available
                        canon = _cmd_output_oneline(csub " " result " 2>/dev/null")
                        result = (canon != "") ? canon : result
                }
                if (result == "") {
                        # Fallback: uname-based triplet
                        result = _cmd_output_oneline("uname -m") "-pc-" tolower(_cmd_output_oneline("uname -s")) "-gnu"
                }
                pyconf_build = result
        }

        # Determine host triplet and cross-compilation status
        if (host_arg != "") {
                canon = _cmd_output_oneline(csub " " host_arg " 2>/dev/null")
                pyconf_host = (canon != "") ? canon : host_arg
                n = split(pyconf_host, parts, "-")
                pyconf_host_cpu = parts[1]
                if (build_arg != "") {
                        pyconf_cross_compiling = (pyconf_host != pyconf_build) ? "yes" : "no"
                } else {
                        # autoconf sets "maybe" when --host is given without --build
                        pyconf_cross_compiling = (pyconf_host != pyconf_build) ? "maybe" : "no"
                }
        } else {
                # For native builds, host == build
                pyconf_host = pyconf_build
                n = split(pyconf_build, parts, "-")
                pyconf_host_cpu = parts[1]
                pyconf_cross_compiling = "no"
        }
}

function pyconf_find_compiler(user_cc, user_cpp, cc, cpp, ver) {
        # Find C compiler — match AC_PROG_CC search order: gcc, cc
        if (user_cc != "") {
                cc = user_cc
        } else if (ENVIRON["CC"] != "") {
                cc = ENVIRON["CC"]
        } else {
                # Try gcc first, then cc (short names, not full paths)
                if (_cmd_exit_code("gcc -c -o /dev/null /dev/null 2>/dev/null") == 0)
                        cc = "gcc"
                else if (_cmd_exit_code("cc -c -o /dev/null /dev/null 2>/dev/null") == 0)
                        cc = "cc"
                else
                        cc = "cc"
        }
        pyconf_CC = cc
        # Find C preprocessor
        if (user_cpp != "") {
                cpp = user_cpp
        } else if (ENVIRON["CPP"] != "") {
                cpp = ENVIRON["CPP"]
        } else {
                cpp = cc " -E"
        }
        pyconf_CPP = cpp
        # Identify compiler via --version output
        # Detect emcc before clang — emcc's version contains "clang"
        ver = _cmd_output(cc " --version 2>/dev/null")
        if (pyconf_basename(cc) == "emcc" || tolower(ver) ~ /emscripten/) {
                pyconf_ac_cv_cc_name = "emcc"
                pyconf_ac_cv_gcc_compat = "yes"
                pyconf_GCC = "yes"
        } else if (tolower(ver) ~ /clang/) {
                pyconf_ac_cv_cc_name = "clang"
                pyconf_ac_cv_gcc_compat = "yes"
                pyconf_GCC = "yes"
        } else if (ver ~ /GCC/ || ver ~ /Free Software Foundation/) {
                pyconf_ac_cv_cc_name = "gcc"
                pyconf_ac_cv_gcc_compat = "yes"
                pyconf_GCC = "yes"
        } else {
                pyconf_ac_cv_cc_name = "unknown"
                pyconf_ac_cv_gcc_compat = "no"
                pyconf_GCC = "no"
        }
        pyconf_ac_cv_prog_cc_g = "yes"
}

function pyconf_check_emscripten_port(pkg_var, emport_args,    flag, cname, lname) {
        # Set {pkg_var}_CFLAGS and {pkg_var}_LIBS to emport_args when
        # building with Emscripten and user hasn't provided values.
        if (V["ac_sys_system"] != "Emscripten") return
        flag = emport_args
        if (substr(flag, 1, 1) != "-") flag = "-s" flag
        cname = pkg_var "_CFLAGS"
        lname = pkg_var "_LIBS"
        if (V[cname] == "" && V[lname] == "") {
                V[cname] = flag
                V[lname] = flag
        }
}

function pyconf_ax_c_float_words_bigendian(on_big, on_little, on_unknown, src, conftest, exe, cmd, rc, has_big, has_little, pre, post) {
        # Use the same compile-and-grep technique as the autoconf
        # AX_C_FLOAT_WORDS_BIGENDIAN macro: the magic constant
        # 9.090423496703681e+223 encodes as bytes containing "noonsees"
        # when float words are big-endian and "seesnoon" when little-endian.
        # We link the test program and search the binary for these markers,
        # which works even when cross-compiling.
        src = "#include <stdlib.h>\nstatic double m[] = {9.090423496703681e+223, 0.0};\nint main(int argc, char *argv[]) {\n  m[atoi(argv[1])] += atof(argv[2]);\n  return m[atoi(argv[3])] > 0.0;\n}\n"
        conftest = _pyconf_tmpdir "/conftest.c"
        exe = _pyconf_tmpdir "/conftest"
        printf "%s", src > conftest
        close(conftest)
        cmd = _pyconf_env_prefix()
        cmd = cmd V["CC"] " " V["CPPFLAGS"] " " V["CFLAGS"] " " V["LDFLAGS"] " " conftest " -o " exe " " V["LIBS"] " 2>/dev/null"
        rc = system(cmd)
        if (rc != 0) {
                system("rm -f " _pyconf_tmpdir "/conftest*")
                _pyconf_retval = "unknown"
                return
        }
        has_big = (system("grep -c noonsees " _pyconf_tmpdir "/conftest* >/dev/null 2>&1") == 0)
        has_little = (system("grep -c seesnoon " _pyconf_tmpdir "/conftest* >/dev/null 2>&1") == 0)
        system("rm -f " _pyconf_tmpdir "/conftest*")
        if (has_big && !has_little)
                _pyconf_retval = "big"
        else if (has_little && !has_big)
                _pyconf_retval = "little"
        else
                _pyconf_retval = "unknown"
}

function pyconf_check_c_bigendian(    result_val, conftest, exe, cmd, rc, data, has_big, has_little, source) {
        # AC_C_BIGENDIAN — multi-stage detection matching autoconf.
        result_val = "unknown"

        # Stage 1: sys/param.h BYTE_ORDER macros
        if (result_val == "unknown") {
                source = "#include <sys/types.h>\n#include <sys/param.h>\nint main(void) {\n#if ! (defined BYTE_ORDER && defined BIG_ENDIAN && defined LITTLE_ENDIAN && BYTE_ORDER && BIG_ENDIAN && LITTLE_ENDIAN)\n  bogus endian macros\n#endif\n  return 0;\n}\n"
                if (_pyconf_compile_test_cd(source)) {
                        source = "#include <sys/types.h>\n#include <sys/param.h>\nint main(void) {\n#if BYTE_ORDER != BIG_ENDIAN\n  not big endian\n#endif\n  return 0;\n}\n"
                        if (_pyconf_compile_test_cd(source))
                                result_val = "yes"
                        else
                                result_val = "no"
                }
        }

        # Stage 2: limits.h _LITTLE_ENDIAN / _BIG_ENDIAN (e.g. Solaris)
        if (result_val == "unknown") {
                source = "#include <limits.h>\nint main(void) {\n#if ! (defined _LITTLE_ENDIAN || defined _BIG_ENDIAN)\n  bogus endian macros\n#endif\n  return 0;\n}\n"
                if (_pyconf_compile_test_cd(source)) {
                        source = "#include <limits.h>\nint main(void) {\n#ifndef _BIG_ENDIAN\n  not big endian\n#endif\n  return 0;\n}\n"
                        if (_pyconf_compile_test_cd(source))
                                result_val = "yes"
                        else
                                result_val = "no"
                }
        }

        # Stage 3/4: object-file grep (cross) or runtime test (native)
        if (result_val == "unknown") {
                if (pyconf_cross_compiling == "yes") {
                        # Link with marker strings and grep the binary
                        source = "unsigned short int ascii_mm[] =\n  { 0x4249, 0x4765, 0x6E44, 0x6961, 0x6E53, 0x7953, 0 };\nunsigned short int ascii_ii[] =\n  { 0x694C, 0x5454, 0x656C, 0x6E45, 0x6944, 0x6E61, 0 };\nint use_ascii(int i) {\n  return ascii_mm[i] + ascii_ii[i];\n}\nint main(int argc, char **argv) {\n  char *p = argv[0];\n  ascii_mm[1] = *p++; ascii_ii[1] = *p++;\n  return use_ascii(argc);\n}\n"
                        conftest = _pyconf_tmpdir "/conftest.c"
                        exe = _pyconf_tmpdir "/conftest"
                        _pyconf_write_confdefs(conftest)
                        printf "\n%s", source >> conftest
                        close(conftest)
                        cmd = _pyconf_env_prefix()
                        cmd = cmd V["CC"] " " V["CPPFLAGS"] " " V["CFLAGS"] " " V["LDFLAGS"] " " conftest " -o " exe " " V["LIBS"] " 2>/dev/null"
                        rc = system(cmd)
                        if (rc == 0) {
                                has_big = (system("grep -c BIGenDianSyS " exe " >/dev/null 2>&1") == 0)
                                has_little = (system("grep -c LiTTleEnDian " exe " >/dev/null 2>&1") == 0)
                                if (has_big && !has_little)
                                        result_val = "yes"
                                else if (has_little && !has_big)
                                        result_val = "no"
                        }
                        system("rm -f " conftest " " exe)
                } else {
                        source = "#include <stdio.h>\nint main(void) {\n  unsigned int x = 1;\n  unsigned char *p = (unsigned char *)&x;\n  printf(\"%d\\n\", p[0] == 0 ? 1 : 0);\n  return 0;\n}\n"
                        data = _pyconf_run_test(source, "", 1)
                        gsub(/[ \t\n]/, "", data)
                        if (data == "1")
                                result_val = "yes"
                        else if (data == "0")
                                result_val = "no"
                }
        }

        if (result_val == "yes")
                pyconf_define("WORDS_BIGENDIAN", 1, 0, "Define to 1 if your processor stores words big-endian.")
}

