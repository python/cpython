import html
import os
import pathlib
import re
from _colorize import decolor  # type: ignore[import-not-found]
from typing import TextIO

from .findtests import findtestdir
from .result import TestResult
from .runtests import RunTests
from .utils import StrPath, count


class Reporter:
    """Report test failures, for example to a CI service.

    The default reporter does nothing.
    """

    def test_case_failed(self, test_case: str, traceback: str,
                         stream: TextIO) -> None:
        """Report a failed test case.

        Called just after unittest printed the failure, or at the end of the
        test file in quiet mode.
        """

    def test_result(self, result: TestResult, runtests: RunTests) -> None:
        """Report the result of a test file, in the main process."""

    def run_failed(self, failed: list[TestResult], state: str) -> None:
        """Report the test files which failed the run."""

    @staticmethod
    def parse_traceback(traceback: str) -> tuple[str | None, str | None, str]:
        """Parse the traceback of a failure into (filename, line, exception).

        The location is the last frame in a test file, or else the first frame
        (the test).
        """
        frames = re.findall(r'^ *File "(.*)", line (\d+)', traceback,
                            re.MULTILINE)
        test_frames = [frame for frame in frames
                       if pathlib.Path(frame[0]).name.startswith('test_')]
        filename, line = (test_frames[-1] if test_frames
                          else frames[0] if frames
                          else (None, None))
        exception = re.split(r'^  File .*\n(?:    .*\n)*', traceback,
                             flags=re.MULTILINE)[-1].strip()
        return filename, line, exception

    @staticmethod
    def test_filename(result: TestResult,
                      test_dir: StrPath | None) -> str | None:
        """Get the filename of a test, or None if it is not found."""
        if result.filename:
            return result.filename
        # The worker crashed or timed out before importing the test:
        # "test.test_x.test_y" is test_x/test_y.py or test_x/test_y/__init__.py
        name = result.test_name.removeprefix("test.")
        path = pathlib.Path(findtestdir(test_dir), *name.split("."))
        for filename in (path.with_suffix(".py"), path / "__init__.py"):
            if filename.exists():
                return str(filename)
        return None


def get_reporter(github_summary: str | None) -> Reporter:
    if github_summary:
        return GitHubActionsReporter(github_summary)
    return Reporter()


class GitHubActionsReporter(Reporter):
    """Annotate failures in the GitHub Actions job log and list them in the
    job summary."""

    # Escape the data and the properties of workflow commands
    ESCAPE_DATA = str.maketrans({"%": "%25", "\r": "%0D", "\n": "%0A"})
    ESCAPE_PROPERTY = ESCAPE_DATA | str.maketrans({":": "%3A", ",": "%2C"})

    def __init__(self, summary_filename: str) -> None:
        self.summary_filename = summary_filename

    def test_case_failed(self, test_case: str, traceback: str,
                         stream: TextIO) -> None:
        # The job log has the full traceback: only keep the exception
        filename, line, exception = self.parse_traceback(decolor(traceback))
        self.annotate(test_case, exception, filename, line, stream)

    def test_result(self, result: TestResult, runtests: RunTests) -> None:
        # Failed test cases are annotated by test_case_failed()
        if (not result.is_failed(runtests.fail_env_changed)
                or result.failed_test_cases):
            return
        message = "\n".join([str(result), *(result.env_changed_reasons or ())])
        self.annotate(result.test_name, decolor(message),
                      self.test_filename(result, runtests.test_dir))

    def annotate(self, title: str, message: str, filename: str | None,
                 line: str | None = None, stream: TextIO | None = None) -> None:
        if filename:
            # The stdlib can be outside of the repository checkout
            path = pathlib.Path(filename)
            stdlib_dir = pathlib.Path(os.__file__).parent
            if path.is_relative_to(stdlib_dir):
                filename = ("Lib" / path.relative_to(stdlib_dir)).as_posix()
            # Without a line, the annotation is not shown on the file
            line = line or "1"
        props = {"file": filename, "line": line, "title": title}
        props_text = ",".join(f"{key}={value.translate(self.ESCAPE_PROPERTY)}"
                              for key, value in props.items() if value)
        print(f"::error {props_text}::{message.translate(self.ESCAPE_DATA)}",
              file=stream, flush=True)

    def run_failed(self, failed: list[TestResult], state: str) -> None:
        ncases = sum(len(result.failed_test_cases) for result in failed)
        blocks = [f"## {decolor(state)}: "
                  f"{count(len(failed), 'test file')} and "
                  f"{count(ncases, 'test case')} failed"]
        for result in failed:
            blocks.append(f"### {decolor(str(result))}")
            if result.env_changed_reasons:
                blocks.append("\n".join(f"- {reason}"
                                        for reason in result.env_changed_reasons))
            for name, traceback in result.failed_test_cases:
                # Expand short tracebacks if there are only a few
                is_open = ncases <= 5 and traceback.count("\n") < 30
                blocks += [f"<details{' open' if is_open else ''}>"
                           f"<summary>{html.escape(name)}</summary>",
                           f"```pytb\n{decolor(traceback).rstrip()}\n```",
                           "</details>"]
        # Markdown blocks are separated by an empty line
        with open(self.summary_filename, "a", encoding="utf-8") as fp:
            fp.write("".join(f"{block}\n\n" for block in blocks))
