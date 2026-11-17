import html
import os
import pathlib
import re
from _colorize import decolor  # type: ignore[import-not-found]
from typing import TextIO

from .findtests import findtestdir
from .result import TestResult
from .results import TestResults
from .runtests import RunTests
from .utils import count


class Reporter:
    """Report test failures, for example to a CI service. A no-op here."""

    def run_started(self, runtests: RunTests) -> None:
        """Called at the start of each run, and in the test process."""

    def test_case_failed(self, test_case: str, traceback: str,
                         stream: TextIO) -> None:
        """Called in the test process after a test case failed."""

    def test_result(self, result: TestResult) -> None:
        """Called in the main process after a test file completed."""

    def summary(self, results: TestResults, state: str) -> None:
        """Called in the main process after all runs."""

    @staticmethod
    def parse_traceback(traceback: str) -> tuple[str | None, str | None, str]:
        """Return (filename, line, exception) of a failure."""
        # A frame is its "File" line and its source lines
        frames = list(re.finditer(
            r'^ *File "(.*)", line (\d+).*\n(?:    .*\n)*', traceback,
            re.MULTILINE))
        filename = line = None
        exception = traceback
        if frames:
            filename, line = frames[0].groups()
            exception = traceback[frames[-1].end():]
        # Prefer the last frame in a test file, the assertion
        for frame in frames:
            if pathlib.Path(frame[1]).name.startswith("test_"):
                filename, line = frame.groups()
        return filename, line, exception.strip()


def get_reporter() -> Reporter:
    if GitHubActionsReporter.enabled():
        return GitHubActionsReporter()
    return Reporter()


class GitHubActionsReporter(Reporter):
    """Annotate failures in the job log and list them in the job summary."""

    ESCAPE_DATA = str.maketrans({"%": "%25", "\r": "%0D", "\n": "%0A"})
    ESCAPE_PROPERTY = ESCAPE_DATA | str.maketrans({":": "%3A", ",": "%2C"})

    @staticmethod
    def enabled() -> bool:
        return bool(os.environ.get("GITHUB_STEP_SUMMARY"))

    def run_started(self, runtests: RunTests) -> None:
        self.runtests = runtests

    def test_case_failed(self, test_case: str, traceback: str,
                         stream: TextIO) -> None:
        if not self.runtests.final_run:
            return
        filename, line, exception = self.parse_traceback(decolor(traceback))
        self.annotate(test_case, exception, filename, line, stream)

    def test_result(self, result: TestResult) -> None:
        if not self.runtests.final_run:
            return
        if not result.is_failed(self.runtests.fail_env_changed):
            return
        # Already annotated by test_case_failed()
        if result.failed_test_cases:
            return
        # "test.test_x.test_y" is test_x/test_y.py or test_x/test_y/__init__.py
        name = result.test_name.removeprefix("test.")
        path = pathlib.Path(findtestdir(self.runtests.test_dir),
                            *name.split("."))
        filename = None
        for candidate in (path.with_suffix(".py"), path / "__init__.py"):
            if candidate.exists():
                filename = str(candidate)
        message = "\n".join([str(result), *(result.env_changed_reasons or ())])
        self.annotate(result.test_name, decolor(message), filename)

    def annotate(self, title: str, message: str, filename: str | None,
                 line: str | None = None,
                 stream: TextIO | None = None) -> None:
        if filename:
            # Make installed stdlib paths relative to the checkout
            path = pathlib.Path(filename)
            stdlib_dir = pathlib.Path(os.__file__).parent
            if path.is_relative_to(stdlib_dir):
                filename = ("Lib" / path.relative_to(stdlib_dir)).as_posix()
            # GitHub needs a line to show the annotation on the file
            line = line or "1"
        props = {"file": filename, "line": line, "title": title}
        props_text = ",".join(f"{key}={value.translate(self.ESCAPE_PROPERTY)}"
                              for key, value in props.items() if value)
        print(f"::error {props_text}::{message.translate(self.ESCAPE_DATA)}",
              file=stream, flush=True)

    def summary(self, results: TestResults, state: str) -> None:
        failed = [result for result in results.rerun_results
                  if result.is_failed(self.runtests.fail_env_changed)]
        if not failed:
            return
        ncases = sum(len(result.failed_test_cases) for result in failed)
        blocks = [f"## {decolor(state)}: "
                  f"{count(len(failed), 'test file')} and "
                  f"{count(ncases, 'test case')} failed"]
        for result in failed:
            blocks.append(f"### {decolor(str(result))}")
            if result.env_changed_reasons:
                blocks.append("\n".join(
                    f"- {reason}" for reason in result.env_changed_reasons))
            for name, traceback in result.failed_test_cases:
                # Expand short tracebacks if there are only a few
                if ncases <= 5 and traceback.count("\n") < 30:
                    details = "<details open>"
                else:
                    details = "<details>"
                blocks += [f"{details}<summary>{html.escape(name)}</summary>",
                           f"```pytb\n{decolor(traceback).rstrip()}\n```",
                           "</details>"]
        with open(os.environ["GITHUB_STEP_SUMMARY"], "a",
                  encoding="utf-8") as fp:
            fp.write("".join(f"{block}\n\n" for block in blocks))
