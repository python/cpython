# regen_status.awk - Regenerate output files from config.status data.
#
# This script is invoked by config.status to regenerate output files
# (Makefile.pre, pyconfig.h, etc.) from their .in templates, using
# substitution values saved at the end of config.status itself.
#
# Usage (from config.status):
#   awk -f $SRCDIR/Tools/configure/transpiler/regen_status.awk \
#       -v cs_args="$CS_ARGS" \
#       -v cs_config_files="${CONFIG_FILES-@UNSET@}" \
#       -v cs_config_headers="${CONFIG_HEADERS-@UNSET@}" \
#       -v srcdir="$SRCDIR" \
#       config.status
#
# The config.status file has a data section after @CONFIG_STATUS_DATA@
# containing saved SUBST and DEFINES values.

# ---------------------------------------------------------------------------
# Data parsing — skip shell preamble, then read SUBST and DEFINES
# ---------------------------------------------------------------------------

BEGIN {
    section = "skip"
    if (srcdir == "") srcdir = "."
}

section == "skip" && /^@CONFIG_STATUS_DATA@$/ { section = ""; next }
section == "skip" { next }

section == "" && /^@SUBST@$/ { section = "subst_key"; next }
section == "subst_key" && /^@DEFINES@$/ { section = "def_key"; next }
section == "def_key" && /^@END@$/ { section = "done"; do_output(); next }

# SUBST parsing: key, line_count, then value lines
section == "subst_key" {
    _cs_key = $0; section = "subst_nlines"; next
}
section == "subst_nlines" {
    _cs_nlines = $0 + 0; _cs_val = ""; _cs_vi = 0
    if (_cs_nlines <= 0) _cs_nlines = 1
    section = "subst_val"; next
}
section == "subst_val" {
    _cs_vi++
    if (_cs_vi == 1) _cs_val = $0
    else _cs_val = _cs_val "\n" $0
    if (_cs_vi >= _cs_nlines) {
        SUBST[_cs_key] = _cs_val
        section = "subst_key"
    }
    next
}

# DEFINES parsing: key, value, description, quoted (4 lines each)
section == "def_key" {
    _def_key = $0; section = "def_val"; next
}
section == "def_val" {
    _def_val = $0; section = "def_desc"; next
}
section == "def_desc" {
    _def_desc = $0; section = "def_quoted"; next
}
section == "def_quoted" {
    _def_n++
    DEF_KEYS[_def_n] = _def_key
    DEF_VAL[_def_key] = _def_val
    DEF_DESC[_def_key] = _def_desc
    DEF_QUOTED[_def_key] = ($0 + 0)
    section = "def_key"; next
}

# ---------------------------------------------------------------------------
# Output logic
# ---------------------------------------------------------------------------

function do_output(    do_headers, do_filter, n, files, i) {
    # Determine what to regenerate based on -v variables set by the
    # config.status shell wrapper.
    #   cs_args       — positional args ("Modules/Setup.bootstrap")
    #   cs_config_files   — $CONFIG_FILES or "@UNSET@" if not set
    #   cs_config_headers — $CONFIG_HEADERS or "@UNSET@" if not set

    do_headers = 1
    if (cs_config_headers != "@UNSET@") {
        if (cs_config_headers == "") do_headers = 0
    }
    if (cs_args != "") do_headers = 0

    do_filter = 0
    if (cs_args != "") {
        # Positional args: regenerate only those files
        do_filter = 1
        n = split(cs_args, files, " ")
        for (i = 1; i <= n; i++) {
            if (files[i] == "pyconfig.h") do_headers = 1
            else _cs_requested[files[i]] = 1
        }
    } else if (cs_config_files != "@UNSET@") {
        do_filter = 1
        if (cs_config_files != "") {
            n = split(cs_config_files, files, " ")
            for (i = 1; i <= n; i++) _cs_requested[files[i]] = 1
        }
    }

    if (do_headers) _cs_write_pyconfig_h()
    _cs_process_config_files(do_filter)
}

# ---------------------------------------------------------------------------
# Regenerate pyconfig.h
# ---------------------------------------------------------------------------

function _cs_write_pyconfig_h(    inf, outf, line, name, val, indent, rest, i) {
    outf = "pyconfig.h"
    inf = srcdir "/pyconfig.h.in"
    if ((getline line < inf) > 0) {
        printf "/* pyconfig.h.  Generated from pyconfig.h.in by configure.  */\n" > outf
        do {
            # Match lines like:  #undef NAME  (with optional leading indent)
            if (match(line, /^[ \t]*#[ \t]*undef[ \t]+[A-Za-z_][A-Za-z_0-9]*[ \t]*$/)) {
                indent = ""
                if (match(line, /^[ \t]+/))
                    indent = substr(line, 1, RLENGTH)
                name = line
                sub(/^[ \t]*#[ \t]*undef[ \t]+/, "", name)
                sub(/[ \t]*$/, "", name)
                if (name in DEF_VAL) {
                    val = DEF_VAL[name]
                    if (DEF_QUOTED[name] && val != "" && val !~ /^-?[0-9]+$/)
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
    } else {
        # No template — write defines directly (fallback)
        printf "/* pyconfig.h.  Generated by configure.  */\n" > outf
        printf "#ifndef Py_PYCONFIG_H\n" >> outf
        printf "#define Py_PYCONFIG_H\n\n" >> outf
        for (i = 1; i <= _def_n; i++) {
            name = DEF_KEYS[i]
            val = DEF_VAL[name]
            if (DEF_DESC[name] != "")
                printf "/* %s */\n", DEF_DESC[name] >> outf
            if (DEF_QUOTED[name] && val != "" && val !~ /^-?[0-9]+$/)
                printf "#define %s \"%s\"\n", name, val >> outf
            else
                printf "#define %s %s\n", name, val >> outf
            printf "\n" >> outf
        }
        printf "#endif /* Py_PYCONFIG_H */\n" >> outf
        close(outf)
    }
}

# ---------------------------------------------------------------------------
# Regenerate config files (Makefile.pre, etc.) from .in templates
# ---------------------------------------------------------------------------

function _cs_process_config_files(do_filter,    all_files, n, i, outf, inf, x_files, nx, j) {
    all_files = SUBST["_config_files"]
    if (all_files == "") return
    n = split(all_files, files, " ")
    nx = split(SUBST["_config_files_x"], x_files, " ")
    for (i = 1; i <= n; i++) {
        outf = files[i]
        if (do_filter && !(outf in _cs_requested)) continue
        if (srcdir != "" && srcdir != ".")
            inf = srcdir "/" outf ".in"
        else
            inf = outf ".in"
        if (system("test -f " _cs_quote(inf)) != 0) continue
        _cs_subst_file(inf, outf)
    }
    for (j = 1; j <= nx; j++) {
        if (system("test -f " _cs_quote(x_files[j])) == 0)
            system("chmod +x " _cs_quote(x_files[j]))
    }
}

# Perform @VAR@ substitution on a template file.
function _cs_subst_file(inf, outf,    line, k, pat, val, pos, before, after, skip) {
    while ((getline line < inf) > 0) {
        skip = 0
        for (k in SUBST) {
            pat = "@" k "@"
            while (index(line, pat) > 0) {
                val = SUBST[k]
                pos = index(line, pat)
                before = substr(line, 1, pos - 1)
                after = substr(line, pos + length(pat))
                if (index(val, "\n") > 0) {
                    printf "%s%s%s\n", before, val, after > outf
                    skip = 1; break
                } else {
                    line = before val after
                }
            }
            if (skip) break
        }
        if (!skip) {
            if (SUBST["srcdir"] == "." && _cs_is_vpath(line))
                print "" > outf
            else
                print line > outf
        }
    }
    close(inf)
    close(outf)
}

function _cs_is_vpath(line,    val) {
    if (!match(line, /^[ \t]*VPATH[ \t]*=[ \t]*/)) return 0
    val = substr(line, RSTART + RLENGTH)
    gsub(/^[ \t]+|[ \t]+$/, "", val)
    return (val == "." || val == "$(srcdir)" || val == "${srcdir}")
}

function _cs_quote(s) {
    gsub(/'/, "'\\''", s)
    return "'" s "'"
}
