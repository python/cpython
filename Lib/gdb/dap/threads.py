# Copyright 2022-2024 Free Software Foundation, Inc.

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

from .server import request


def _thread_name(thr):
    if thr.name is not None:
        return thr.name
    if thr.details is not None:
        return thr.details
    return None


@request("threads")
def threads(**args):
    result = []
    for thr in gdb.selected_inferior().threads():
        one_result = {
            "id": thr.global_num,
        }
        name = _thread_name(thr)
        if name is not None:
            one_result["name"] = name
        result.append(one_result)
    return {
        "threads": result,
    }
