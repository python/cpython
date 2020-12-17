# -*- coding: utf-8 -*-
"""A minimal non-colored version of https://pypi.org/project/halo, to track list progress"""
from __future__ import absolute_import, unicode_literals

import os
import sys
import threading
from collections import OrderedDict
from datetime import datetime

import py

threads = []

if os.name == "nt":
    import ctypes

    class _CursorInfo(ctypes.Structure):
        _fields_ = [("size", ctypes.c_int), ("visible", ctypes.c_byte)]


def _file_support_encoding(chars, file):
    encoding = getattr(file, "encoding", None)
    if encoding is not None:
        for char in chars:
            try:
                char.encode(encoding)
            except UnicodeEncodeError:
                break
        else:
            return True
    return False


class Spinner(object):
    CLEAR_LINE = "\033[K"
    max_width = 120
    UNICODE_FRAMES = ["⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"]
    ASCII_FRAMES = ["|", "-", "+", "x", "*"]

    def __init__(self, enabled=True, refresh_rate=0.1):
        self.refresh_rate = refresh_rate
        self.enabled = enabled
        self._file = sys.stdout
        self.frames = (
            self.UNICODE_FRAMES
            if _file_support_encoding(self.UNICODE_FRAMES, sys.stdout)
            else self.ASCII_FRAMES
        )
        self.stream = py.io.TerminalWriter(file=self._file)
        self._envs = OrderedDict()
        self._frame_index = 0

    def clear(self):
        if self.enabled:
            self.stream.write("\r")
            self.stream.write(self.CLEAR_LINE)

    def render(self):
        while True:
            self._stop_spinner.wait(self.refresh_rate)
            if self._stop_spinner.is_set():
                break
            self.render_frame()
        return self

    def render_frame(self):
        if self.enabled:
            self.clear()
            self.stream.write("\r{}".format(self.frame()))

    def frame(self):
        frame = self.frames[self._frame_index]
        self._frame_index += 1
        self._frame_index = self._frame_index % len(self.frames)
        text_frame = "[{}] {}".format(len(self._envs), " | ".join(self._envs))
        if len(text_frame) > self.max_width - 1:
            text_frame = "{}...".format(text_frame[: self.max_width - 1 - 3])
        return "{} {}".format(*[(frame, text_frame)][0])

    def __enter__(self):
        if self.enabled:
            self.disable_cursor()
        self.render_frame()
        self._stop_spinner = threading.Event()
        self._spinner_thread = threading.Thread(target=self.render)
        self._spinner_thread.setDaemon(True)
        self._spinner_thread.start()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if not self._stop_spinner.is_set():
            if self._spinner_thread:
                self._stop_spinner.set()
                self._spinner_thread.join()

            self._frame_index = 0
            if self.enabled:
                self.clear()
                self.enable_cursor()

        return self

    def add(self, name):
        self._envs[name] = datetime.now()

    def succeed(self, key):
        self.finalize(key, "✔ OK", green=True)

    def fail(self, key):
        self.finalize(key, "✖ FAIL", red=True)

    def skip(self, key):
        self.finalize(key, "⚠ SKIP", white=True)

    def finalize(self, key, status, **kwargs):
        start_at = self._envs[key]
        del self._envs[key]
        if self.enabled:
            self.clear()
        self.stream.write(
            "{} {} in {}{}".format(
                status,
                key,
                td_human_readable(datetime.now() - start_at),
                os.linesep,
            ),
            **kwargs
        )
        if not self._envs:
            self.__exit__(None, None, None)

    def disable_cursor(self):
        if self._file.isatty():
            if os.name == "nt":
                ci = _CursorInfo()
                handle = ctypes.windll.kernel32.GetStdHandle(-11)
                ctypes.windll.kernel32.GetConsoleCursorInfo(handle, ctypes.byref(ci))
                ci.visible = False
                ctypes.windll.kernel32.SetConsoleCursorInfo(handle, ctypes.byref(ci))
            elif os.name == "posix":
                self.stream.write("\033[?25l")

    def enable_cursor(self):
        if self._file.isatty():
            if os.name == "nt":
                ci = _CursorInfo()
                handle = ctypes.windll.kernel32.GetStdHandle(-11)
                ctypes.windll.kernel32.GetConsoleCursorInfo(handle, ctypes.byref(ci))
                ci.visible = True
                ctypes.windll.kernel32.SetConsoleCursorInfo(handle, ctypes.byref(ci))
            elif os.name == "posix":
                self.stream.write("\033[?25h")


def td_human_readable(delta):
    seconds = int(delta.total_seconds())
    periods = [
        ("year", 60 * 60 * 24 * 365),
        ("month", 60 * 60 * 24 * 30),
        ("day", 60 * 60 * 24),
        ("hour", 60 * 60),
        ("minute", 60),
        ("second", 1),
    ]

    texts = []
    for period_name, period_seconds in periods:
        if seconds > period_seconds or period_seconds == 1:
            period_value, seconds = divmod(seconds, period_seconds)
            if period_name == "second":
                ms = delta.total_seconds() - int(delta.total_seconds())
                period_value = round(period_value + ms, 3)
            has_s = "s" if period_value != 1 else ""
            texts.append("{} {}{}".format(period_value, period_name, has_s))
    return ", ".join(texts)
