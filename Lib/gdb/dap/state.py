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

from .startup import exec_and_log, in_gdb_thread, log


@in_gdb_thread
def set_thread(thread_id):
    """Set the current thread to THREAD_ID."""
    if thread_id == 0:
        log("+++ Thread == 0 +++")
    else:
        exec_and_log("thread " + str(thread_id))
