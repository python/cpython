# Extended prompt utilities.
# Copyright (C) 2011-2024 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

""" Extended prompt library functions."""

import os

import gdb


def _prompt_pwd(ignore):
    "The current working directory."
    return os.getcwd()


def _prompt_object_attr(func, what, attr, nattr):
    """Internal worker for fetching GDB attributes."""
    if attr is None:
        attr = nattr
    try:
        obj = func()
    except gdb.error:
        return "<no %s>" % what
    if hasattr(obj, attr):
        result = getattr(obj, attr)
        if callable(result):
            result = result()
        return result
    else:
        return "<no attribute %s on current %s>" % (attr, what)


def _prompt_frame(attr):
    "The selected frame; an argument names a frame parameter."
    return _prompt_object_attr(gdb.selected_frame, "frame", attr, "name")


def _prompt_thread(attr):
    "The selected thread; an argument names a thread parameter."
    return _prompt_object_attr(gdb.selected_thread, "thread", attr, "num")


def _prompt_version(attr):
    "The version of GDB."
    return gdb.VERSION


def _prompt_esc(attr):
    "The ESC character."
    return "\033"


def _prompt_bs(attr):
    "A backslash."
    return "\\"


def _prompt_n(attr):
    "A newline."
    return "\n"


def _prompt_r(attr):
    "A carriage return."
    return "\r"


def _prompt_param(attr):
    "A parameter's value; the argument names the parameter."
    return gdb.parameter(attr)


def _prompt_noprint_begin(attr):
    "Begins a sequence of non-printing characters."
    return "\001"


def _prompt_noprint_end(attr):
    "Ends a sequence of non-printing characters."
    return "\002"


prompt_substitutions = {
    "e": _prompt_esc,
    "\\": _prompt_bs,
    "n": _prompt_n,
    "r": _prompt_r,
    "v": _prompt_version,
    "w": _prompt_pwd,
    "f": _prompt_frame,
    "t": _prompt_thread,
    "p": _prompt_param,
    "[": _prompt_noprint_begin,
    "]": _prompt_noprint_end,
}


def prompt_help():
    """Generate help dynamically from the __doc__ strings of attribute
    functions."""

    result = ""
    keys = sorted(prompt_substitutions.keys())
    for key in keys:
        result += "  \\%s\t%s\n" % (key, prompt_substitutions[key].__doc__)
    result += """
A substitution can be used in a simple form, like "\\f".
An argument can also be passed to it, like "\\f{name}".
The meaning of the argument depends on the particular substitution."""
    return result


def substitute_prompt(prompt):
    "Perform substitutions on PROMPT."

    result = ""
    plen = len(prompt)
    i = 0
    while i < plen:
        if prompt[i] == "\\":
            i = i + 1
            if i >= plen:
                break
            cmdch = prompt[i]

            if cmdch in prompt_substitutions:
                cmd = prompt_substitutions[cmdch]

                if i + 1 < plen and prompt[i + 1] == "{":
                    j = i + 1
                    while j < plen and prompt[j] != "}":
                        j = j + 1
                    # Just ignore formatting errors.
                    if j >= plen or prompt[j] != "}":
                        arg = None
                    else:
                        arg = prompt[i + 2 : j]
                        i = j
                else:
                    arg = None
                result += str(cmd(arg))
            else:
                # Unrecognized escapes are turned into the escaped
                # character itself.
                result += prompt[i]
        else:
            result += prompt[i]

        i = i + 1

    return result
