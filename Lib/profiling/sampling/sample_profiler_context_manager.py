# sample_profiler_context_manager_pipe.py
import os
import time
import multiprocessing as mp
from multiprocessing.connection import Connection
from typing import Any

from .sample import SampleProfiler, print_sampled_stats
from .pstats_collector import PstatsCollector
from .sampling_messages import (
    CommandMessage, ResponseMessage,
    StartCommand, StatsCommand, ExportCommand, ShutdownCommand,
    AckResponse, StatsResponse, ErrorResponse
)
from .sampling_daemon import _SamplingDaemon

class SampleProfilerContextManager:
    def __init__(self, *, interval_usec: int = 1000, all_threads: bool = False,
                 start_timeout: float = 5.0):
        self.interval_usec = interval_usec
        self.all_threads = all_threads
        self._start_timeout = start_timeout

        self._parent_conn: Connection | None = None
        self._daemon: mp.Process | None = None
        self._req_counter = 0

    def _send_command(self, command: CommandMessage) -> None:
        if self._parent_conn:
            self._parent_conn.send(command)

    def _wait_for_response(self, response_type, timeout: float = 5.0):
        if not self._parent_conn:
            raise RuntimeError("No connection to worker")

        if self._parent_conn.poll(timeout):
            response = self._parent_conn.recv()
            if isinstance(response, ErrorResponse):
                raise RuntimeError(response.msg)
            elif isinstance(response, response_type):
                return response
            else:
                raise RuntimeError(f"Expected {response_type.__name__}, got {type(response).__name__}")
        else:
            raise TimeoutError(f"Timeout waiting for {response_type.__name__}")

    def __enter__(self):
        my_pid = os.getpid()

        self._parent_conn, child_conn = mp.Pipe(duplex=True)

        self._daemon = _SamplingDaemon(
            child_conn, my_pid, self.interval_usec, self.all_threads
        )
        self._daemon.start()

        start_cmd = StartCommand()
        self._send_command(start_cmd)

        response = self._wait_for_response(AckResponse, timeout=self._start_timeout)
        if not response.success:
            raise RuntimeError(f"Failed to start profiler: {response}")

        return self

    def __exit__(self, exc_type, exc, tb):
        try:
            if self._parent_conn:
                self._send_command(ShutdownCommand())
                try:
                    self._wait_for_response(AckResponse, timeout=2.0)
                except (TimeoutError, RuntimeError):
                    pass

                self._parent_conn.close()
                self._parent_conn = None
        except Exception:
            pass

        if self._daemon:
            self._daemon.join(timeout=2.0)
            if self._daemon.is_alive():
                self._daemon.terminate()
                self._daemon.join()
            self._daemon = None

    def _get_stats(self, limit: int | None = None, sort: int | None = None,
                   show_summary: bool | None = None, req_id: Any | None = None) -> str:
        if not req_id:
            self._req_counter += 1
            req_id = f"stats_{self._req_counter}"

        stats_cmd = StatsCommand(
            req_id=req_id,
            limit=limit,
            sort=sort,
            show_summary=show_summary
        )
        self._send_command(stats_cmd)

        response = self._wait_for_response(StatsResponse, timeout=5.0)
        return response.text

    def print_stats(self, limit: int | None = None, sort: int | None = None, show_summary: bool | None = None,
                   file=None, req_id: Any | None = None) -> str:
        text = self._get_stats(limit=limit, sort=sort, show_summary=show_summary, req_id=req_id)
        print(text, file=file)
        return text

    def export_stats(self, filename: str, req_id: Any | None = None) -> str:
        # TODO How should we handle this in loop?
        if not req_id:
            self._req_counter += 1
            req_id = f"export_{self._req_counter}"

        export_cmd = ExportCommand(filename=filename, req_id=req_id)
        self._send_command(export_cmd)

        response = self._wait_for_response(StatsResponse, timeout=5.0)
        confirmation_msg: str = response.text
        print(confirmation_msg)
        return confirmation_msg

    def dump_to_file(self, filename: str, limit: int | None = None, sort: int | None = None,
                     show_summary: bool | None = None, req_id: Any | None = None) -> str:
        # TODO How should we handle this in loop?
        text = self._get_stats(limit=limit, sort=sort, show_summary=show_summary, req_id=req_id)

        try:
            with open(filename, 'w') as f:
                f.write(text)
            confirmation_msg = f"Formatted profiling report written to {filename}"
            print(confirmation_msg)
            return confirmation_msg
        except Exception as e:
            error_msg = f"Error writing to file {filename}: {e!r}"
            print(error_msg)
            return error_msg
