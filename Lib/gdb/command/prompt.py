# Extended prompt.
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

"""GDB command for working with extended prompts."""

import gdb
import gdb.prompt


class _ExtendedPrompt(gdb.Parameter):
    """Set the extended prompt.

    Usage: set extended-prompt VALUE

    Substitutions are applied to VALUE to compute the real prompt.

    The currently defined substitutions are:"""

    # Add the prompt library's dynamically generated help to the
    # __doc__ string.
    __doc__ = __doc__ + "\n" + gdb.prompt.prompt_help()

    set_doc = "Set the extended prompt."
    show_doc = "Show the extended prompt."

    def __init__(self):
        super(_ExtendedPrompt, self).__init__(
            "extended-prompt", gdb.COMMAND_SUPPORT, gdb.PARAM_STRING_NOESCAPE
        )
        self.value = ""
        self.hook_set = False

    def get_show_string(self, pvalue):
        if self.value:
            return "The extended prompt is: " + self.value
        else:
            return "The extended prompt is not set."

    def get_set_string(self):
        if self.hook_set is False:
            gdb.prompt_hook = self.before_prompt_hook
            self.hook_set = True
        return ""

    def before_prompt_hook(self, current):
        if self.value:
            return gdb.prompt.substitute_prompt(self.value)
        else:
            return None


_ExtendedPrompt()
