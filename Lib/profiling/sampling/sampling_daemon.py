import os
import sys
import time
import pstats
import threading
import multiprocessing as mp
from multiprocessing.connection import Connection
from io import StringIO
from contextlib import redirect_stdout
from typing import Any
from dataclasses import asdict

from .sample import SampleProfiler, print_sampled_stats
from .pstats_collector import PstatsCollector
from .sampling_messages import (
    CommandMessage, ResponseMessage, is_profiler_command,
    StartCommand, StatsCommand, ExportCommand, ShutdownCommand,
    AckResponse, StatsResponse, ErrorResponse
)


class _SamplingDaemon(mp.Process):
    def __init__(self, conn: Connection, target_pid: int, interval_usec: int, all_threads: bool):
        super().__init__(name="SamplingDaemon")
        self.conn = conn
        self.target_pid = target_pid
        self.interval_usec = interval_usec
        self.all_threads = all_threads

        self.profiler: SampleProfiler | None = None
        self.collector: PstatsCollector | None = None
        self._sampler_thread: threading.Thread | None = None
        self._stop_evt: threading.Event | None = None
        self._lock: threading.Lock | None = None
        self._chunk_secs = 0.25

    def _send_response(self, response: ResponseMessage) -> None:
        try:
            self.conn.send(response)
        except (OSError, EOFError, BrokenPipeError):
            # Connection is broken, parent process likely died
            self._stop_sampling()
            sys.exit(0)
        except Exception as e:
            # Unexpected error, log it and exit since we can't communicate
            print(f"Error: Failed to send response {type(response).__name__}: {e}", file=sys.stderr)
            self._stop_sampling()
            sys.exit(1)

    def _initialize_profiler(self) -> None:
        if self.profiler:
            return

        try:
            self._stop_evt = threading.Event()
            self._lock = threading.Lock()

            self.profiler = SampleProfiler(self.target_pid, self.interval_usec, all_threads=self.all_threads)
            self.profiler.realtime_stats = False

            # TODO: Where should we filter stack frames? RemoteUnwinder?
            filter_func = self._create_profiler_filter()
            self.collector = PstatsCollector(self.interval_usec, filter_func=filter_func)

        except Exception as e:
            raise RuntimeError(f"Failed to initialize profiler: {e}")

    def _create_profiler_filter(self):
        """Create a filter function that excludes profiler internal frames."""
        def should_exclude_frame(frame):
            filename = frame.filename

            exclude_patterns = [
                'sample_profiler_context_manager.py',
                'sampling_daemon.py',
                'pstats_collector.py',
                'connection.py',
                'selectors.py',
            ]

            for pattern in exclude_patterns:
                if pattern in filename:
                    return True

            return False

        return should_exclude_frame

    def _start_sampling(self) -> None:
        if self._sampler_thread and self._sampler_thread.is_alive():
            raise RuntimeError("Sampling already in progress")

        try:
            self._initialize_profiler()

            self._stop_evt.clear()
            self._sampler_thread = threading.Thread(target=self._sampler_loop, name="SamplerThread", daemon=True)
            self._sampler_thread.start()

        except Exception as e:
            raise RuntimeError(f"Failed to start sampling thread: {e}")

    def _sampler_loop(self) -> None:
        if not self.profiler or not self.collector or not self._stop_evt:
            self._send_response(ErrorResponse(msg="Profiler, collector, or threading objects not initialized"))
            return

        while not self._stop_evt.is_set():
            try:
                with self._lock:
                    self.profiler.sample(self.collector, duration_sec=self._chunk_secs)

            except ProcessLookupError:
                self._send_response(ErrorResponse(msg="Target process ended"))
                break

            except PermissionError as e:
                self._send_response(ErrorResponse(msg=f"Permission denied: {e}"))
                break

            except OSError as e:
                self._send_response(ErrorResponse(msg=f"OS error: {e}"))
                break

            except Exception as e:
                self._send_response(ErrorResponse(msg=f"Sampling error: {e!r}"))
                break

    def _stop_sampling(self) -> None:
        if self._stop_evt:
            self._stop_evt.set()
        if self._sampler_thread and self._sampler_thread.is_alive():
            self._sampler_thread.join(timeout=2.0)
        self._sampler_thread = None

    def _render_stats(self, params: StatsCommand) -> str:
        if not self.collector:
            raise RuntimeError("Profiler not started")

        with self._lock:
            self.collector.create_stats()
            stats_obj = pstats.SampledStats(self.collector).strip_dirs()

            buf = StringIO()
            with redirect_stdout(buf):
                kwargs = {k: v for k, v in asdict(params).items()
                         if v is not None and k != 'req_id'}
                kwargs['sample_interval_usec'] = self.collector.sample_interval_usec

                print_sampled_stats(stats_obj, **kwargs)
            return buf.getvalue()

    def _export_stats(self, filename: str) -> str:
        if not self.collector:
            raise RuntimeError("Profiler not started")

        with self._lock:
            self.collector.export(filename)
        return f"Profile data exported to {filename}"

    def _handle_start_command(self, cmd: StartCommand) -> None:
        try:
            self._start_sampling()
            self._send_response(AckResponse(cmd="start", success=True))
        except Exception as e:
            self._send_response(ErrorResponse(msg=f"Start failed: {e!r}"))

    def _handle_stats_command(self, cmd: StatsCommand) -> None:
        try:
            text = self._render_stats(cmd)
            self._send_response(StatsResponse(text=text, req_id=cmd.req_id))
        except Exception as e:
            self._send_response(ErrorResponse(msg=f"Stats failed: {e!r}", req_id=cmd.req_id))

    def _handle_export_command(self, cmd: ExportCommand) -> None:
        try:
            result_msg = self._export_stats(cmd.filename)
            self._send_response(StatsResponse(text=result_msg, req_id=cmd.req_id))
        except Exception as e:
            self._send_response(ErrorResponse(msg=f"Export failed: {e!r}", req_id=cmd.req_id))

    def _handle_shutdown_command(self, cmd: ShutdownCommand) -> None:
        self._stop_sampling()
        self._send_response(AckResponse(cmd="shutdown"))

    def run(self) -> None:
        try:
            while True:
                if not self.conn.poll(0.1):
                    continue

                try:
                    msg = self.conn.recv()
                except (EOFError, OSError, BrokenPipeError):
                    break
                except Exception as e:
                    self._send_response(ErrorResponse(msg=f"Failed to receive message: {e!r}"))
                    continue

                if not is_profiler_command(msg):
                    self._send_response(ErrorResponse(msg=f"Unknown message type: {type(msg).__name__}"))
                    continue

                if isinstance(msg, ShutdownCommand):
                    self._handle_shutdown_command(msg)
                    break

                elif isinstance(msg, StartCommand):
                    self._handle_start_command(msg)

                elif isinstance(msg, StatsCommand):
                    self._handle_stats_command(msg)

                elif isinstance(msg, ExportCommand):
                    self._handle_export_command(msg)

                else:
                    self._send_response(ErrorResponse(msg=f"Unknown command type: {type(msg).__name__}"))
        finally:
            self._stop_sampling()
            self.conn.close()
