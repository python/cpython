# Copyright 2023-2024 Free Software Foundation, Inc.

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

import gdb

from .server import capability, request
from .startup import in_gdb_thread


@in_gdb_thread
def module_id(objfile):
    """Return the module ID for the objfile."""
    return objfile.username


@in_gdb_thread
def is_module(objfile):
    """Return True if OBJFILE represents a valid Module."""
    return objfile.is_valid() and objfile.owner is None


@in_gdb_thread
def make_module(objf):
    """Return a Module representing the objfile OBJF.

    The objfile must pass the 'is_module' test."""
    result = {
        "id": module_id(objf),
        "name": objf.username,
    }
    if objf.is_file:
        result["path"] = objf.filename
    return result


@capability("supportsModulesRequest")
@request("modules")
def modules(*, startModule: int = 0, moduleCount: int = 0, **args):
    # Don't count invalid objfiles or separate debug objfiles.
    objfiles = [x for x in gdb.objfiles() if is_module(x)]
    if moduleCount == 0:
        # Use all items.
        last = len(objfiles)
    else:
        last = startModule + moduleCount
    return {
        "modules": [make_module(x) for x in objfiles[startModule:last]],
        "totalModules": len(objfiles),
    }
