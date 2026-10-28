import itertools
import io
import json
import os
import platform
import sys
import tempfile
import threading
import time

from .collector import Collector, filter_internal_frames
from .opcode_utils import get_opcode_info, format_opcode
try:
    from _remote_debugging import THREAD_STATUS_HAS_GIL, THREAD_STATUS_ON_CPU, THREAD_STATUS_UNKNOWN, THREAD_STATUS_GIL_REQUESTED, THREAD_STATUS_HAS_EXCEPTION, THREAD_STATUS_MAIN_THREAD
except ImportError:
    # Fallback if module not available (shouldn't happen in normal use)
    THREAD_STATUS_HAS_GIL = (1 << 0)
    THREAD_STATUS_ON_CPU = (1 << 1)
    THREAD_STATUS_UNKNOWN = (1 << 2)
    THREAD_STATUS_GIL_REQUESTED = (1 << 3)
    THREAD_STATUS_HAS_EXCEPTION = (1 << 4)
    THREAD_STATUS_MAIN_THREAD = (1 << 5)


# Categories matching Firefox Profiler expectations
GECKO_CATEGORIES = [
    {"name": "Other", "color": "grey", "subcategories": ["Other"]},
    {"name": "Python", "color": "yellow", "subcategories": ["Other"]},
    {"name": "Native", "color": "blue", "subcategories": ["Other"]},
    {"name": "GC", "color": "orange", "subcategories": ["Other"]},
    {"name": "GIL", "color": "green", "subcategories": ["Other"]},
    {"name": "CPU", "color": "purple", "subcategories": ["Other"]},
    {"name": "Code Type", "color": "red", "subcategories": ["Other"]},
    {"name": "Opcodes", "color": "magenta", "subcategories": ["Other"]},
    {"name": "Exception", "color": "lightblue", "subcategories": ["Other"]},
]

# Category indices
CATEGORY_OTHER = 0
CATEGORY_PYTHON = 1
CATEGORY_NATIVE = 2
CATEGORY_GC = 3
CATEGORY_GIL = 4
CATEGORY_CPU = 5
CATEGORY_CODE_TYPE = 6
CATEGORY_OPCODES = 7
CATEGORY_EXCEPTION = 8

# Subcategory indices
DEFAULT_SUBCATEGORY = 0

GECKO_FORMAT_VERSION = 32
GECKO_PREPROCESSED_VERSION = 57

# Resource type constants
RESOURCE_TYPE_LIBRARY = 1

# Frame constants
FRAME_ADDRESS_NONE = -1
FRAME_INLINE_DEPTH_ROOT = 0

# Process constants
PROCESS_TYPE_MAIN = 0
STACKWALK_DISABLED = 0

# In-memory buffer before spilling to disk
DEFAULT_SPILL_BUFFER_BYTES = 128 * 1024
_JSON_SEPARATORS = (",", ":")
_JSON_ENCODER = json.JSONEncoder(
    separators=_JSON_SEPARATORS, allow_nan=False
)


class SpillColumn:
    def __init__(self, directory, basename, *,
                 buffer_bytes=None):
        self.path = os.path.join(directory, basename)
        self.buffer = bytearray()
        self._buffer_bytes = (
            DEFAULT_SPILL_BUFFER_BYTES if buffer_bytes is None
            else buffer_bytes
        )

    def append(self, value):
        self.buffer += (_JSON_ENCODER.encode(value) + "\n").encode("utf-8")
        if len(self.buffer) >= self._buffer_bytes:
            self.flush()

    def flush(self):
        with open(self.path, "ab") as file:
            file.write(self.buffer)
        self.buffer.clear()

    def iter_tokens(self):
        with open(self.path, encoding="utf-8") as file:
            for line in file:
                yield line.rstrip("\n")


class GeckoThreadSpill:
    _COLUMNS = (
        ("samples_stack", "samples-stack.json"),
        ("samples_time", "samples-time.json"),
        ("markers_name", "markers-name.json"),
        ("markers_start_time", "markers-start-time.json"),
        ("markers_end_time", "markers-end-time.json"),
        ("markers_phase", "markers-phase.json"),
        ("markers_category", "markers-category.json"),
        ("markers_data", "markers-data.json"),
    )

    def __init__(self, directory, tid):
        prefix = f"thread-{tid}-"
        for attr, basename in self._COLUMNS:
            setattr(self, attr, SpillColumn(directory, prefix + basename))
        self.sample_count = 0
        self.marker_count = 0

    def append_sample(self, stack_index, time_ms):
        self.samples_stack.append(stack_index)
        self.samples_time.append(time_ms)
        self.sample_count += 1

    def append_marker(self, name_idx, start_time, end_time, phase, category, data):
        self.markers_name.append(name_idx)
        self.markers_start_time.append(start_time)
        self.markers_end_time.append(end_time)
        self.markers_phase.append(phase)
        self.markers_category.append(category)
        self.markers_data.append(data)
        self.marker_count += 1

    def prepare_read(self):
        for attr, _basename in self._COLUMNS:
            getattr(self, attr).flush()


class GeckoCollector(Collector):
    aggregating = True

    def __init__(self, sample_interval_usec, *, skip_idle=False, opcodes=False):
        self.sample_interval_usec = sample_interval_usec
        self.skip_idle = skip_idle
        self.opcodes_enabled = opcodes
        self.start_time = time.monotonic() * 1000  # milliseconds since start

        # Global string table (shared across all threads)
        self.global_strings = ["(root)"]  # Start with root
        self.global_string_map = {"(root)": 0}

        # Per-thread data structures
        self.threads = {}  # tid -> thread data
        self.spill_dir = None
        self.exported = False

        # Global tables
        self.libs = []

        # Sampling interval tracking
        self.sample_count = 0
        self.last_sample_time = 0
        self.interval = 1.0  # Will be calculated from actual sampling

        # State tracking for interval markers (tid -> start_time)
        self.has_gil_start = {}           # Thread has the GIL
        self.no_gil_start = {}            # Thread doesn't have the GIL
        self.on_cpu_start = {}            # Thread is running on CPU
        self.off_cpu_start = {}           # Thread is off CPU
        self.python_code_start = {}       # Thread running Python code (has GIL)
        self.native_code_start = {}       # Thread running native code (on CPU without GIL)
        self.gil_wait_start = {}          # Thread waiting for GIL
        self.exception_start = {}         # Thread has an exception set
        self.no_exception_start = {}      # Thread has no exception set

        # GC event tracking: track GC start time per thread
        self.gc_start_per_thread = {}  # tid -> start_time

        # Track which threads have been initialized for state tracking
        self.initialized_threads = set()

        # Opcode state tracking per thread: tid -> (opcode, lineno, col_offset, funcname, filename, start_time)
        self.opcode_state = {}

        # For binary replay: track base timestamp (first sample's timestamp)
        self._replay_base_timestamp_us = None

    def _track_state_transition(self, tid, condition, active_dict, inactive_dict,
                                  active_name, inactive_name, category, current_time):
        """Track binary state transitions and emit markers.

        Args:
            tid: Thread ID
            condition: Whether the active state is true
            active_dict: Dict tracking start time of active state
            inactive_dict: Dict tracking start time of inactive state
            active_name: Name for active state marker
            inactive_name: Name for inactive state marker
            category: Gecko category for the markers
            current_time: Current timestamp
        """
        # On first observation of a thread, just record the current state
        # without creating a marker (we don't know what the previous state was)
        if tid not in self.initialized_threads:
            if condition:
                active_dict[tid] = current_time
            else:
                inactive_dict[tid] = current_time
            return

        # For already-initialized threads, track transitions
        if condition:
            active_dict.setdefault(tid, current_time)
            if tid in inactive_dict:
                self._add_marker(tid, inactive_name, inactive_dict.pop(tid),
                               current_time, category)
        else:
            inactive_dict.setdefault(tid, current_time)
            if tid in active_dict:
                self._add_marker(tid, active_name, active_dict.pop(tid),
                               current_time, category)

    def collect(self, stack_frames, timestamps_us=None):
        """Collect samples from stack frames.

        Args:
            stack_frames: List of interpreter/thread frame info
            timestamps_us: List of timestamps in microseconds (None for live sampling)
        """
        if self.exported:
            raise RuntimeError("cannot append to GeckoCollector after export")

        # Handle live sampling (no timestamps provided)
        if timestamps_us is None:
            current_time = (time.monotonic() * 1000) - self.start_time
            times = [current_time]
        else:
            if not timestamps_us:
                return
            # Initialize base timestamp if needed
            if self._replay_base_timestamp_us is None:
                self._replay_base_timestamp_us = timestamps_us[0]
            # Convert all timestamps to times (ms relative to first sample)
            base = self._replay_base_timestamp_us
            times = [(ts - base) / 1000 for ts in timestamps_us]

        first_time = times[0]

        # Update interval calculation
        if self.sample_count > 0 and self.last_sample_time > 0:
            self.interval = (times[-1] - self.last_sample_time) / self.sample_count
        self.last_sample_time = times[-1]

        # Process async tasks
        if stack_frames and hasattr(stack_frames[0], "awaited_by"):
            for frames, thread_id, _ in self._iter_async_frames(stack_frames):
                frames = filter_internal_frames(frames)
                if not frames:
                    continue

                if thread_id not in self.threads:
                    self.threads[thread_id] = self._create_thread(
                        thread_id, False
                    )

                self._record_stack_sample(
                    self.threads[thread_id], frames, thread_id, times, first_time
                )

            self.sample_count += len(times)
            return

        # Process threads
        for interpreter_info in stack_frames:
            for thread_info in interpreter_info.threads:
                frames = filter_internal_frames(thread_info.frame_info)
                tid = thread_info.thread_id
                status_flags = thread_info.status
                is_main_thread = bool(status_flags & THREAD_STATUS_MAIN_THREAD)

                # Initialize thread if needed
                if tid not in self.threads:
                    self.threads[tid] = self._create_thread(tid, is_main_thread)

                thread_data = self.threads[tid]

                # Decode status flags
                has_gil = bool(status_flags & THREAD_STATUS_HAS_GIL)
                on_cpu = bool(status_flags & THREAD_STATUS_ON_CPU)
                gil_requested = bool(status_flags & THREAD_STATUS_GIL_REQUESTED)

                # Track state transitions using first timestamp
                self._track_state_transition(
                    tid, has_gil, self.has_gil_start, self.no_gil_start,
                    "Has GIL", "No GIL", CATEGORY_GIL, first_time
                )
                self._track_state_transition(
                    tid, on_cpu, self.on_cpu_start, self.off_cpu_start,
                    "On CPU", "Off CPU", CATEGORY_CPU, first_time
                )

                # Track code type
                if has_gil:
                    self._track_state_transition(
                        tid, True, self.python_code_start, self.native_code_start,
                        "Python Code", "Native Code", CATEGORY_CODE_TYPE, first_time
                    )
                elif on_cpu:
                    self._track_state_transition(
                        tid, True, self.native_code_start, self.python_code_start,
                        "Native Code", "Python Code", CATEGORY_CODE_TYPE, first_time
                    )
                else:
                    if tid in self.initialized_threads:
                        if tid in self.python_code_start:
                            self._add_marker(tid, "Python Code", self.python_code_start.pop(tid),
                                           first_time, CATEGORY_CODE_TYPE)
                        if tid in self.native_code_start:
                            self._add_marker(tid, "Native Code", self.native_code_start.pop(tid),
                                           first_time, CATEGORY_CODE_TYPE)

                # Track GIL wait
                if gil_requested:
                    self.gil_wait_start.setdefault(tid, first_time)
                elif tid in self.gil_wait_start:
                    self._add_marker(tid, "Waiting for GIL", self.gil_wait_start.pop(tid),
                                   first_time, CATEGORY_GIL)

                # Track exception state
                has_exception = bool(status_flags & THREAD_STATUS_HAS_EXCEPTION)
                self._track_state_transition(
                    tid, has_exception, self.exception_start, self.no_exception_start,
                    "Has Exception", "No Exception", CATEGORY_EXCEPTION, first_time
                )

                # Track GC events
                has_gc_frame = any(frame[2] == "<GC>" for frame in frames)
                if has_gc_frame:
                    if tid not in self.gc_start_per_thread:
                        self.gc_start_per_thread[tid] = first_time
                elif tid in self.gc_start_per_thread:
                    self._add_marker(tid, "GC Collecting", self.gc_start_per_thread.pop(tid),
                                   first_time, CATEGORY_GC)

                # Mark thread as initialized
                self.initialized_threads.add(tid)

                # Skip idle threads if requested
                is_idle = not has_gil and not on_cpu
                if self.skip_idle and is_idle:
                    continue

                if not frames:
                    continue

                self._record_stack_sample(
                    thread_data, frames, tid, times, first_time
                )

        self.sample_count += len(times)

    def _record_stack_sample(self, thread_data, frames, tid, times, first_time):
        stack_index = self._process_stack(thread_data, frames)

        thread_spill = thread_data["_spill"]
        for t in times:
            thread_spill.append_sample(stack_index, t)

        if self.opcodes_enabled and frames:
            leaf_frame = frames[0]
            filename, location, funcname, opcode = leaf_frame
            if isinstance(location, tuple):
                lineno, _, col_offset, _ = location
            else:
                lineno = location
                col_offset = -1

            current_state = (opcode, lineno, col_offset, funcname, filename)

            if tid not in self.opcode_state:
                self.opcode_state[tid] = (*current_state, first_time)
            elif self.opcode_state[tid][:5] != current_state:
                (
                    prev_opcode, prev_lineno, prev_col, prev_funcname,
                    prev_filename, prev_start
                ) = self.opcode_state[tid]
                self._add_opcode_interval_marker(
                    tid, prev_opcode, prev_lineno, prev_col, prev_funcname,
                    prev_start, first_time
                )
                self.opcode_state[tid] = (*current_state, first_time)

    def _create_thread(self, tid, is_main_thread):
        """Create a new thread structure with processed profile format."""
        if self.spill_dir is None:
            self.spill_dir = tempfile.TemporaryDirectory()

        thread = {
            "name": f"Thread-{tid}",
            "isMainThread": is_main_thread,
            "processStartupTime": 0,
            "processShutdownTime": None,
            "registerTime": 0,
            "unregisterTime": None,
            "pausedRanges": [],
            "pid": str(os.getpid()),
            "tid": tid,
            "processType": "default",
            "processName": "Python Process",
            # Stack table - processed format
            "stackTable": {
                "frame": [],
                "category": [],
                "subcategory": [],
                "prefix": [],
                "length": 0,  # Will be updated on export
            },
            # Frame table - processed format
            "frameTable": {
                "address": [],
                "category": [],
                "subcategory": [],
                "func": [],
                "innerWindowID": [],
                "implementation": [],
                "optimizations": [],
                "line": [],
                "column": [],
                "inlineDepth": [],
                "nativeSymbol": [],
                "length": 0,  # Will be updated on export
            },
            # Function table - processed format
            "funcTable": {
                "name": [],
                "isJS": [],
                "relevantForJS": [],
                "resource": [],
                "fileName": [],
                "lineNumber": [],
                "columnNumber": [],
                "length": 0,  # Will be updated on export
            },
            # Resource table - processed format
            "resourceTable": {
                "lib": [],
                "name": [],
                "host": [],
                "type": [],
                "length": 0,  # Will be updated on export
            },
            # Native symbols table (empty for Python)
            "nativeSymbols": {
                "libIndex": [],
                "address": [],
                "name": [],
                "functionSize": [],
                "length": 0,
            },
            # Caches for deduplication
            "_stackCache": {},
            "_frameCache": {},
            "_funcCache": {},
            "_resourceCache": {},
            "_spill": GeckoThreadSpill(self.spill_dir.name, tid),
        }

        return thread

    def _is_python(self, filename: str) -> bool:
        return not filename.startswith("<") or filename in ["<stdin>", "<string>"]

    def _get_category(self, filename: str) -> int:
        return CATEGORY_PYTHON if self._is_python(filename) else CATEGORY_NATIVE

    def _intern_string(self, s):
        """Intern a string in the global string table."""
        if s in self.global_string_map:
            return self.global_string_map[s]
        idx = len(self.global_strings)
        self.global_strings.append(s)
        self.global_string_map[s] = idx
        return idx

    def _add_marker(self, tid, name, start_time, end_time, category):
        """Add an interval marker for a specific thread."""
        if tid not in self.threads:
            return

        duration = end_time - start_time

        name_idx = self._intern_string(name)
        self.threads[tid]["_spill"].append_marker(
            name_idx, start_time, end_time, 1, category, {
                "type": name.replace(" ", ""),
                "duration": duration,
                "tid": tid,
            }
        )

    def _add_opcode_interval_marker(self, tid, opcode, lineno, col_offset,
                                    funcname, start_time, end_time):
        """Add an interval marker for opcode execution span."""
        if tid not in self.threads or opcode is None:
            return

        opcode_info = get_opcode_info(opcode)
        # Use formatted opcode name (with base opcode for specialized ones)
        formatted_opname = format_opcode(opcode)

        name_idx = self._intern_string(formatted_opname)

        self.threads[tid]["_spill"].append_marker(
            name_idx, start_time, end_time, 1, CATEGORY_OPCODES, {
                "type": "Opcode",
                "opcode": opcode,
                "opname": formatted_opname,
                "base_opname": opcode_info["base_opname"],
                "is_specialized": opcode_info["is_specialized"],
                "line": lineno,
                "column": col_offset if col_offset >= 0 else None,
                "function": funcname,
                "duration": end_time - start_time,
            }
        )

    def _process_stack(self, thread_data, frames):
        """Process a stack and return the stack index."""
        if not frames:
            return None

        # Cache references to avoid repeated dictionary lookups
        stack_cache = thread_data["_stackCache"]
        stack_table = thread_data["stackTable"]
        stack_frames = stack_table["frame"]
        stack_prefix = stack_table["prefix"]
        stack_category = stack_table["category"]
        stack_subcategory = stack_table["subcategory"]

        # Build stack bottom-up (from root to leaf)
        prefix_stack_idx = None

        for frame_tuple in reversed(frames):
            # frame_tuple is (filename, location, funcname, opcode)
            # location is (lineno, end_lineno, col_offset, end_col_offset) or just lineno
            filename, location, funcname, opcode = frame_tuple
            if isinstance(location, tuple):
                lineno, end_lineno, col_offset, end_col_offset = location
            else:
                # Legacy format: location is just lineno
                lineno = location
                col_offset = -1
                end_col_offset = -1

            # Get or create function
            func_idx = self._get_or_create_func(
                thread_data, filename, funcname, lineno
            )

            # Get or create frame (include column for precise source location)
            frame_idx = self._get_or_create_frame(
                thread_data, func_idx, lineno, col_offset
            )

            # Check stack cache
            stack_key = (frame_idx, prefix_stack_idx)
            if stack_key in stack_cache:
                prefix_stack_idx = stack_cache[stack_key]
            else:
                # Create new stack entry
                stack_idx = len(stack_frames)
                stack_frames.append(frame_idx)
                stack_prefix.append(prefix_stack_idx)

                # Determine category
                category = self._get_category(filename)
                stack_category.append(category)
                stack_subcategory.append(DEFAULT_SUBCATEGORY)

                stack_cache[stack_key] = stack_idx
                prefix_stack_idx = stack_idx

        return prefix_stack_idx

    def _get_or_create_func(self, thread_data, filename, funcname, lineno):
        """Get or create a function entry."""
        func_cache = thread_data["_funcCache"]
        func_key = (filename, funcname)

        if func_key in func_cache:
            return func_cache[func_key]

        # Cache references for func table
        func_table = thread_data["funcTable"]
        func_names = func_table["name"]
        func_is_js = func_table["isJS"]
        func_relevant = func_table["relevantForJS"]
        func_resources = func_table["resource"]
        func_filenames = func_table["fileName"]
        func_line_numbers = func_table["lineNumber"]
        func_column_numbers = func_table["columnNumber"]

        func_idx = len(func_names)

        # Intern strings in global table
        name_idx = self._intern_string(funcname)

        # Determine if Python
        is_python = self._is_python(filename)

        # Create resource
        resource_idx = self._get_or_create_resource(thread_data, filename)

        # Add function
        func_names.append(name_idx)
        func_is_js.append(is_python)
        func_relevant.append(is_python)
        func_resources.append(resource_idx)

        if is_python:
            filename_idx = self._intern_string(os.path.basename(filename))
            func_filenames.append(filename_idx)
            func_line_numbers.append(lineno)
        else:
            func_filenames.append(None)
            func_line_numbers.append(None)
        func_column_numbers.append(None)

        func_cache[func_key] = func_idx
        return func_idx

    def _get_or_create_resource(self, thread_data, filename):
        """Get or create a resource entry."""
        resource_cache = thread_data["_resourceCache"]

        if filename in resource_cache:
            return resource_cache[filename]

        # Cache references for resource table
        resource_table = thread_data["resourceTable"]
        resource_libs = resource_table["lib"]
        resource_names = resource_table["name"]
        resource_hosts = resource_table["host"]
        resource_types = resource_table["type"]

        resource_idx = len(resource_names)
        resource_name = (
            os.path.basename(filename) if "/" in filename else filename
        )
        name_idx = self._intern_string(resource_name)

        resource_libs.append(None)
        resource_names.append(name_idx)
        resource_hosts.append(None)
        resource_types.append(RESOURCE_TYPE_LIBRARY)

        resource_cache[filename] = resource_idx
        return resource_idx

    def _get_or_create_frame(self, thread_data, func_idx, lineno, col_offset=-1):
        """Get or create a frame entry."""
        frame_cache = thread_data["_frameCache"]
        # Include column in cache key for precise frame identification
        frame_key = (func_idx, lineno, col_offset if col_offset >= 0 else None)

        if frame_key in frame_cache:
            return frame_cache[frame_key]

        # Cache references for frame table
        frame_table = thread_data["frameTable"]
        frame_addresses = frame_table["address"]
        frame_inline_depths = frame_table["inlineDepth"]
        frame_categories = frame_table["category"]
        frame_subcategories = frame_table["subcategory"]
        frame_funcs = frame_table["func"]
        frame_native_symbols = frame_table["nativeSymbol"]
        frame_inner_window_ids = frame_table["innerWindowID"]
        frame_implementations = frame_table["implementation"]
        frame_lines = frame_table["line"]
        frame_columns = frame_table["column"]
        frame_optimizations = frame_table["optimizations"]

        frame_idx = len(frame_funcs)

        # Determine category based on function - use cached func table reference
        is_python = thread_data["funcTable"]["isJS"][func_idx]
        category = CATEGORY_PYTHON if is_python else CATEGORY_NATIVE

        frame_addresses.append(FRAME_ADDRESS_NONE)
        frame_inline_depths.append(FRAME_INLINE_DEPTH_ROOT)
        frame_categories.append(category)
        frame_subcategories.append(DEFAULT_SUBCATEGORY)
        frame_funcs.append(func_idx)
        frame_native_symbols.append(None)
        frame_inner_window_ids.append(None)
        frame_implementations.append(None)
        frame_lines.append(lineno if lineno else None)
        # Store column offset if available (>= 0), otherwise None
        frame_columns.append(col_offset if col_offset >= 0 else None)
        frame_optimizations.append(None)

        frame_cache[frame_key] = frame_idx
        return frame_idx

    def _finalize_markers(self):
        """Close any open markers at the end of profiling."""
        end_time = self.last_sample_time

        # Close all open markers for each thread using a generic approach
        marker_states = [
            (self.has_gil_start, "Has GIL", CATEGORY_GIL),
            (self.no_gil_start, "No GIL", CATEGORY_GIL),
            (self.on_cpu_start, "On CPU", CATEGORY_CPU),
            (self.off_cpu_start, "Off CPU", CATEGORY_CPU),
            (self.python_code_start, "Python Code", CATEGORY_CODE_TYPE),
            (self.native_code_start, "Native Code", CATEGORY_CODE_TYPE),
            (self.gil_wait_start, "Waiting for GIL", CATEGORY_GIL),
            (self.gc_start_per_thread, "GC Collecting", CATEGORY_GC),
            (self.exception_start, "Has Exception", CATEGORY_EXCEPTION),
            (self.no_exception_start, "No Exception", CATEGORY_EXCEPTION),
        ]

        for state_dict, marker_name, category in marker_states:
            for tid in list(state_dict.keys()):
                self._add_marker(tid, marker_name, state_dict[tid], end_time, category)
                del state_dict[tid]

        # Close any open opcode markers
        for tid, state in list(self.opcode_state.items()):
            opcode, lineno, col_offset, funcname, filename, start_time = state
            self._add_opcode_interval_marker(tid, opcode, lineno, col_offset, funcname, start_time, end_time)
        self.opcode_state.clear()

    def export(self, filename):
        """Export the profile to a Gecko JSON file."""
        if self.sample_count > 0 and self.last_sample_time > 0:
            self.interval = self.last_sample_time / self.sample_count

        # Spinner for progress indication
        spinner = itertools.cycle(['⠋', '⠙', '⠹', '⠸', '⠼', '⠴', '⠦', '⠧', '⠇', '⠏'])
        stop_spinner = threading.Event()

        def spin():
            message = 'Building Gecko profile...'
            while not stop_spinner.is_set():
                sys.stderr.write(f'\r{next(spinner)} {message}')
                sys.stderr.flush()
                time.sleep(0.1)
            # Clear the spinner line
            sys.stderr.write('\r' + ' ' * (len(message) + 3) + '\r')
            sys.stderr.flush()

        spinner_thread = threading.Thread(target=spin, daemon=True)
        spinner_thread.start()

        temp_path = None
        replaced = False
        try:
            self._prepare_for_serialization()
            output_dir = os.path.dirname(os.path.abspath(filename)) or "."
            with tempfile.NamedTemporaryFile(
                "w", dir=output_dir, delete=False
            ) as file:
                temp_path = file.name
                self._stream_profile(file)
            os.replace(temp_path, filename)
            replaced = True
        finally:
            self.exported = True
            stop_spinner.set()
            spinner_thread.join(timeout=1.0)
            # Small delay to ensure the clear happens
            time.sleep(0.01)
            if temp_path is not None and not replaced:
                try:
                    os.unlink(temp_path)
                except FileNotFoundError:
                    pass
            self._cleanup_spills()

        print(f"Gecko profile written to {filename}")
        print(
            f"Open in Firefox Profiler: https://profiler.firefox.com/"
        )
        return True

    def _build_marker_schema(self):
        """Build marker schema definitions for Firefox Profiler."""
        schema = []

        # Opcode marker schema (only if opcodes enabled)
        if self.opcodes_enabled:
            schema.append({
                "name": "Opcode",
                "display": ["marker-table", "marker-chart"],
                "tooltipLabel": "{marker.data.opname}",
                "tableLabel": "{marker.data.opname} at line {marker.data.line}",
                "chartLabel": "{marker.data.opname}",
                "fields": [
                    {"key": "opname", "label": "Opcode", "format": "string", "searchable": True},
                    {"key": "base_opname", "label": "Base Opcode", "format": "string"},
                    {"key": "is_specialized", "label": "Specialized", "format": "string"},
                    {"key": "line", "label": "Line", "format": "integer"},
                    {"key": "column", "label": "Column", "format": "integer"},
                    {"key": "function", "label": "Function", "format": "string"},
                    {"key": "duration", "label": "Duration", "format": "duration"},
                ],
            })

        return schema

    def _build_profile(self):
        """Build the complete profile structure in processed format."""
        try:
            self._prepare_for_serialization()
            file = io.StringIO()
            self._stream_profile(file)
            return json.loads(file.getvalue())
        finally:
            self.exported = True
            self._cleanup_spills()

    def _profile_head(self):
        return {
            "meta": {
                "interval": self.interval,
                "startTime": self.start_time,
                "abi": platform.machine(),
                "misc": "Python profiler",
                "oscpu": platform.machine(),
                "platform": platform.system(),
                "processType": PROCESS_TYPE_MAIN,
                "categories": GECKO_CATEGORIES,
                "stackwalk": STACKWALK_DISABLED,
                "toolkit": "",
                "version": GECKO_FORMAT_VERSION,
                "preprocessedProfileVersion": GECKO_PREPROCESSED_VERSION,
                "appBuildID": "",
                "physicalCPUs": os.cpu_count() or 0,
                "logicalCPUs": os.cpu_count() or 0,
                "CPUName": "",
                "product": "Python",
                "symbolicated": True,
                "markerSchema": self._build_marker_schema(),
                "importedFrom": "Tachyon Sampling Profiler",
                "extensions": {
                    "id": [],
                    "name": [],
                    "baseURL": [],
                    "length": 0,
                },
            },
            "libs": self.libs,
        }

    def _profile_tail(self):
        return {
            "pages": [],
            "shared": {
                "stringArray": self.global_strings,
                "sources": {"length": 0, "uuid": [], "filename": []},
            },
        }

    def _prepare_for_serialization(self):
        if self.exported:
            raise RuntimeError("GeckoCollector has already been exported")
        self._finalize_markers()
        for thread_data in self.threads.values():
            thread_data["_spill"].prepare_read()
            thread_data["stackTable"]["length"] = len(thread_data["stackTable"]["frame"])
            thread_data["frameTable"]["length"] = len(thread_data["frameTable"]["func"])
            thread_data["funcTable"]["length"] = len(thread_data["funcTable"]["name"])
            thread_data["resourceTable"]["length"] = len(thread_data["resourceTable"]["name"])

    def _cleanup_spills(self):
        if self.spill_dir is not None:
            self.spill_dir.cleanup()
            self.spill_dir = None

    def _stream_profile(self, file):
        file.write("{")
        first = True
        for key, value in self._profile_head().items():
            first = _write_json_member(file, key, value, first)

        first = _write_member_name(file, "threads", first)
        file.write("[")
        for index, (tid, thread_data) in enumerate(self.threads.items()):
            if index:
                file.write(",")
            self._stream_thread(file, tid, thread_data)
        file.write("]")

        for key, value in self._profile_tail().items():
            first = _write_json_member(file, key, value, first)
        file.write("}")

    def _stream_thread(self, file, tid, thread_data):
        spill = thread_data["_spill"]
        metadata = {
            "name": thread_data["name"],
            "isMainThread": thread_data["isMainThread"],
            "processStartupTime": thread_data["processStartupTime"],
            "processShutdownTime": thread_data["processShutdownTime"],
            "registerTime": thread_data["registerTime"],
            "unregisterTime": thread_data["unregisterTime"],
            "pausedRanges": thread_data["pausedRanges"],
            "pid": thread_data["pid"],
            "tid": thread_data["tid"],
            "processType": thread_data["processType"],
            "processName": thread_data["processName"],
        }
        file.write("{")
        first = True
        for key, value in metadata.items():
            first = _write_json_member(file, key, value, first)

        first = _write_member_name(file, "samples", first)
        self._stream_samples(file, spill)
        for key in (
            "stackTable",
            "frameTable",
            "funcTable",
            "resourceTable",
            "nativeSymbols",
        ):
            first = _write_json_member(file, key, thread_data[key], first)
        first = _write_member_name(file, "markers", first)
        self._stream_markers(file, spill)
        file.write("}")

    def _stream_samples(self, file, spill):
        _stream_column_table(
            file,
            (
                ("stack", spill.samples_stack.iter_tokens()),
                ("time", spill.samples_time.iter_tokens()),
                ("eventDelay", ("null" for _ in range(spill.sample_count))),
            ),
            spill.sample_count,
            (
                ("weight", None),
                ("weightType", "samples"),
                ("length", spill.sample_count),
            ),
        )

    def _stream_markers(self, file, spill):
        _stream_column_table(
            file,
            (
                ("data", spill.markers_data.iter_tokens()),
                ("name", spill.markers_name.iter_tokens()),
                ("startTime", spill.markers_start_time.iter_tokens()),
                ("endTime", spill.markers_end_time.iter_tokens()),
                ("phase", spill.markers_phase.iter_tokens()),
                ("category", spill.markers_category.iter_tokens()),
            ),
            spill.marker_count,
            (("length", spill.marker_count),),
        )


def _write_json(file, value):
    for chunk in _JSON_ENCODER.iterencode(value):
        file.write(chunk)


def _write_member_name(file, name, first):
    if not first:
        file.write(",")
    _write_json(file, name)
    file.write(":")
    return False


def _write_json_member(file, name, value, first):
    first = _write_member_name(file, name, first)
    _write_json(file, value)
    return first


def _stream_column_table(file, columns, expected_count, trailing_members=()):
    file.write("{")
    first = True
    for name, token_iter in columns:
        first = _write_member_name(file, name, first)
        _stream_array(file, token_iter, expected_count, name)
    for name, value in trailing_members:
        first = _write_json_member(file, name, value, first)
    file.write("}")


def _stream_array(file, token_iter, expected_count, label="array"):
    file.write("[")
    count = 0
    for token in token_iter:
        if count:
            file.write(",")
        file.write(token)
        count += 1
    if count != expected_count:
        raise RuntimeError(
            f"streamed {count} {label} items, expected {expected_count}"
        )
    file.write("]")
