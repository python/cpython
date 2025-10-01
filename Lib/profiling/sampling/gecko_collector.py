import itertools
import json
import os
import platform
import sys
import threading
import time

from .collector import Collector
try:
    from _remote_debugging import THREAD_STATUS_HAS_GIL, THREAD_STATUS_ON_CPU, THREAD_STATUS_UNKNOWN, THREAD_STATUS_GIL_REQUESTED
except ImportError:
    # Fallback if module not available (shouldn't happen in normal use)
    THREAD_STATUS_HAS_GIL = (1 << 0)
    THREAD_STATUS_ON_CPU = (1 << 1)
    THREAD_STATUS_UNKNOWN = (1 << 2)
    THREAD_STATUS_GIL_REQUESTED = (1 << 3)


# Categories matching Firefox Profiler expectations
GECKO_CATEGORIES = [
    {"name": "Other", "color": "grey", "subcategories": ["Other"]},
    {"name": "Python", "color": "yellow", "subcategories": ["Other"]},
    {"name": "Native", "color": "blue", "subcategories": ["Other"]},
    {"name": "GC", "color": "orange", "subcategories": ["Other"]},
    {"name": "GIL", "color": "green", "subcategories": ["Other"]},
    {"name": "CPU", "color": "purple", "subcategories": ["Other"]},
    {"name": "Code Type", "color": "red", "subcategories": ["Other"]},
]

# Category indices
CATEGORY_OTHER = 0
CATEGORY_PYTHON = 1
CATEGORY_NATIVE = 2
CATEGORY_GC = 3
CATEGORY_GIL = 4
CATEGORY_CPU = 5
CATEGORY_CODE_TYPE = 6

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


class GeckoCollector(Collector):
    def __init__(self, *, skip_idle=False):
        self.skip_idle = skip_idle
        self.start_time = time.time() * 1000  # milliseconds since epoch

        # Global string table (shared across all threads)
        self.global_strings = ["(root)"]  # Start with root
        self.global_string_map = {"(root)": 0}

        # Per-thread data structures
        self.threads = {}  # tid -> thread data

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

        # GC event tracking: track if we're currently in a GC
        self.potential_gc_start = None

    def collect(self, stack_frames):
        """Collect a sample from stack frames."""
        current_time = (time.time() * 1000) - self.start_time

        # Update interval calculation
        if self.sample_count > 0 and self.last_sample_time > 0:
            self.interval = (
                current_time - self.last_sample_time
            ) / self.sample_count
        self.last_sample_time = current_time

        # GC Event Detection and process threads
        gc_collecting = False

        for interpreter_info in stack_frames:
            for thread_info in interpreter_info.threads:
                # Track GC status
                if thread_info.gc_collecting:
                    gc_collecting = True

                frames = thread_info.frame_info
                tid = thread_info.thread_id

                # Initialize thread if needed
                if tid not in self.threads:
                    self.threads[tid] = self._create_thread(tid)

                thread_data = self.threads[tid]

                # Decode status flags
                status_flags = thread_info.status
                has_gil = bool(status_flags & THREAD_STATUS_HAS_GIL)
                on_cpu = bool(status_flags & THREAD_STATUS_ON_CPU)
                unknown = bool(status_flags & THREAD_STATUS_UNKNOWN)
                gil_requested = bool(status_flags & THREAD_STATUS_GIL_REQUESTED)

                # Track GIL possession (interval marker: "Has GIL" / "No GIL")
                if has_gil:
                    if tid not in self.has_gil_start:
                        self.has_gil_start[tid] = current_time
                    # End "No GIL" if it was running
                    if tid in self.no_gil_start:
                        start_time = self.no_gil_start[tid]
                        self._add_marker(tid, "No GIL", start_time, current_time, CATEGORY_GIL)
                        del self.no_gil_start[tid]
                else:
                    if tid not in self.no_gil_start:
                        self.no_gil_start[tid] = current_time
                    # End "Has GIL" if it was running
                    if tid in self.has_gil_start:
                        start_time = self.has_gil_start[tid]
                        self._add_marker(tid, "Has GIL", start_time, current_time, CATEGORY_GIL)
                        del self.has_gil_start[tid]

                # Track "On CPU" / "Off CPU" state
                if on_cpu:
                    if tid not in self.on_cpu_start:
                        self.on_cpu_start[tid] = current_time
                    # End "Off CPU" if it was running
                    if tid in self.off_cpu_start:
                        start_time = self.off_cpu_start[tid]
                        self._add_marker(tid, "Off CPU", start_time, current_time, CATEGORY_CPU)
                        del self.off_cpu_start[tid]
                else:
                    if tid not in self.off_cpu_start:
                        self.off_cpu_start[tid] = current_time
                    # End "On CPU" if it was running
                    if tid in self.on_cpu_start:
                        start_time = self.on_cpu_start[tid]
                        self._add_marker(tid, "On CPU", start_time, current_time, CATEGORY_CPU)
                        del self.on_cpu_start[tid]

                # Track "Python Code" (has GIL) / "Native Code" (on CPU without GIL)
                if has_gil:
                    if tid not in self.python_code_start:
                        self.python_code_start[tid] = current_time
                    # End "Native Code" if it was running
                    if tid in self.native_code_start:
                        start_time = self.native_code_start[tid]
                        self._add_marker(tid, "Native Code", start_time, current_time, CATEGORY_CODE_TYPE)
                        del self.native_code_start[tid]
                elif on_cpu:
                    # Native code: on CPU without GIL
                    if tid not in self.native_code_start:
                        self.native_code_start[tid] = current_time
                    # End "Python Code" if it was running
                    if tid in self.python_code_start:
                        start_time = self.python_code_start[tid]
                        self._add_marker(tid, "Python Code", start_time, current_time, CATEGORY_CODE_TYPE)
                        del self.python_code_start[tid]
                else:
                    # Neither has GIL nor on CPU - end both if running
                    if tid in self.python_code_start:
                        start_time = self.python_code_start[tid]
                        self._add_marker(tid, "Python Code", start_time, current_time, CATEGORY_CODE_TYPE)
                        del self.python_code_start[tid]
                    if tid in self.native_code_start:
                        start_time = self.native_code_start[tid]
                        self._add_marker(tid, "Native Code", start_time, current_time, CATEGORY_CODE_TYPE)
                        del self.native_code_start[tid]

                # Track "Waiting for GIL" intervals
                if gil_requested:
                    if tid not in self.gil_wait_start:
                        self.gil_wait_start[tid] = current_time
                else:
                    if tid in self.gil_wait_start:
                        start_time = self.gil_wait_start[tid]
                        self._add_marker(tid, "Waiting for GIL", start_time, current_time, CATEGORY_GIL)
                        del self.gil_wait_start[tid]

                # Categorize: idle if neither has GIL nor on CPU
                is_idle = not has_gil and not on_cpu

                # Skip idle threads if skip_idle is enabled
                if self.skip_idle and is_idle:
                    continue

                if not frames:
                    continue

                # Process the stack
                stack_index = self._process_stack(thread_data, frames)

                # Add sample - cache references to avoid dictionary lookups
                samples = thread_data["samples"]
                samples["stack"].append(stack_index)
                samples["time"].append(current_time)
                samples["eventDelay"].append(None)

        # Handle GC event markers after processing all threads
        if gc_collecting:
            if self.potential_gc_start is None:
                # Start of GC
                self.potential_gc_start = current_time
        else:
            # End of GC
            if self.potential_gc_start is not None:
                self._add_gc_marker(self.potential_gc_start, current_time)
            self.potential_gc_start = None

        self.sample_count += 1

    def _create_thread(self, tid):
        """Create a new thread structure with processed profile format."""

        # Determine if this is the main thread
        try:
            is_main = tid == threading.main_thread().ident
        except (RuntimeError, AttributeError):
            is_main = False

        thread = {
            "name": f"Thread-{tid}",
            "isMainThread": is_main,
            "processStartupTime": 0,
            "processShutdownTime": None,
            "registerTime": 0,
            "unregisterTime": None,
            "pausedRanges": [],
            "pid": str(os.getpid()),
            "tid": tid,
            "processType": "default",
            "processName": "Python Process",
            # Sample data - processed format with direct arrays
            "samples": {
                "stack": [],
                "time": [],
                "eventDelay": [],
                "weight": None,
                "weightType": "samples",
                "length": 0,  # Will be updated on export
            },
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
            # Markers - processed format (arrays)
            "markers": {
                "data": [],
                "name": [],
                "startTime": [],
                "endTime": [],
                "phase": [],
                "category": [],
                "length": 0,
            },
            # Caches for deduplication
            "_stackCache": {},
            "_frameCache": {},
            "_funcCache": {},
            "_resourceCache": {},
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

        thread_data = self.threads[tid]
        duration = end_time - start_time

        name_idx = self._intern_string(name)
        markers = thread_data["markers"]
        markers["name"].append(name_idx)
        markers["startTime"].append(start_time)
        markers["endTime"].append(end_time)
        markers["phase"].append(1)  # 1 = interval marker
        markers["category"].append(category)
        markers["data"].append({
            "type": name.replace(" ", ""),
            "duration": duration,
            "tid": tid
        })

    def _add_gc_marker(self, start_time, end_time):
        """Add a GC Collecting event marker to the main thread (or first thread we see)."""
        if not self.threads:
            return

        # Add GC marker to the first thread (typically the main thread)
        first_tid = next(iter(self.threads))
        self._add_marker(first_tid, "GC Collecting", start_time, end_time, CATEGORY_GC)

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
            # frame_tuple is (filename, lineno, funcname)
            filename, lineno, funcname = frame_tuple

            # Get or create function
            func_idx = self._get_or_create_func(
                thread_data, filename, funcname, lineno
            )

            # Get or create frame
            frame_idx = self._get_or_create_frame(
                thread_data, func_idx, lineno
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

    def _get_or_create_frame(self, thread_data, func_idx, lineno):
        """Get or create a frame entry."""
        frame_cache = thread_data["_frameCache"]
        frame_key = (func_idx, lineno)

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
        frame_columns.append(None)
        frame_optimizations.append(None)

        frame_cache[frame_key] = frame_idx
        return frame_idx

    def _finalize_markers(self):
        """Close any open markers at the end of profiling."""
        end_time = self.last_sample_time

        # Close all open markers for each thread
        for tid in list(self.has_gil_start.keys()):
            start_time = self.has_gil_start[tid]
            self._add_marker(tid, "Has GIL", start_time, end_time, CATEGORY_GIL)
            del self.has_gil_start[tid]

        for tid in list(self.no_gil_start.keys()):
            start_time = self.no_gil_start[tid]
            self._add_marker(tid, "No GIL", start_time, end_time, CATEGORY_GIL)
            del self.no_gil_start[tid]

        for tid in list(self.on_cpu_start.keys()):
            start_time = self.on_cpu_start[tid]
            self._add_marker(tid, "On CPU", start_time, end_time, CATEGORY_CPU)
            del self.on_cpu_start[tid]

        for tid in list(self.off_cpu_start.keys()):
            start_time = self.off_cpu_start[tid]
            self._add_marker(tid, "Off CPU", start_time, end_time, CATEGORY_CPU)
            del self.off_cpu_start[tid]

        for tid in list(self.python_code_start.keys()):
            start_time = self.python_code_start[tid]
            self._add_marker(tid, "Python Code", start_time, end_time, CATEGORY_CODE_TYPE)
            del self.python_code_start[tid]

        for tid in list(self.native_code_start.keys()):
            start_time = self.native_code_start[tid]
            self._add_marker(tid, "Native Code", start_time, end_time, CATEGORY_CODE_TYPE)
            del self.native_code_start[tid]

        for tid in list(self.gil_wait_start.keys()):
            start_time = self.gil_wait_start[tid]
            self._add_marker(tid, "Waiting for GIL", start_time, end_time, CATEGORY_GIL)
            del self.gil_wait_start[tid]

        # Close any open GC marker
        if self.potential_gc_start is not None:
            self._add_gc_marker(self.potential_gc_start, end_time)
            self.potential_gc_start = None

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

        try:
            # Finalize any open markers before building profile
            self._finalize_markers()

            profile = self._build_profile()

            with open(filename, "w") as f:
                json.dump(profile, f, separators=(",", ":"))
        finally:
            stop_spinner.set()
            spinner_thread.join(timeout=1.0)
            # Small delay to ensure the clear happens
            time.sleep(0.01)

        print(f"Gecko profile written to {filename}")
        print(
            f"Open in Firefox Profiler: https://profiler.firefox.com/"
        )

    def _build_profile(self):
        """Build the complete profile structure in processed format."""
        # Convert thread data to final format
        threads = []

        for tid, thread_data in self.threads.items():
            # Update lengths
            samples = thread_data["samples"]
            stack_table = thread_data["stackTable"]
            frame_table = thread_data["frameTable"]
            func_table = thread_data["funcTable"]
            resource_table = thread_data["resourceTable"]

            samples["length"] = len(samples["stack"])
            stack_table["length"] = len(stack_table["frame"])
            frame_table["length"] = len(frame_table["func"])
            func_table["length"] = len(func_table["name"])
            resource_table["length"] = len(resource_table["name"])
            thread_data["markers"]["length"] = len(thread_data["markers"]["name"])

            # Clean up internal caches
            del thread_data["_stackCache"]
            del thread_data["_frameCache"]
            del thread_data["_funcCache"]
            del thread_data["_resourceCache"]

            threads.append(thread_data)

        # Main profile structure in processed format
        profile = {
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
                "markerSchema": [],
                "importedFrom": "Tachyon Sampling Profiler",
                "extensions": {
                    "id": [],
                    "name": [],
                    "baseURL": [],
                    "length": 0,
                },
            },
            "libs": self.libs,
            "threads": threads,
            "pages": [],
            "shared": {
                "stringArray": self.global_strings,
                "sources": {"length": 0, "uuid": [], "filename": []},
            },
        }

        return profile
