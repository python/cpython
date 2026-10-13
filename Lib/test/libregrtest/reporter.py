import html
import os
import pathlib
import re
from _colorize import decolor  # type: ignore[import-not-found]
from typing import TextIO

from .findtests import findtestdir
from .result import TestResult
from .runtests import RunTests
from .utils import count


class Reporter:
    """Report test failures, for example to a CI service.

    The default reporter does nothing.
    """

    def test_case_failed(self, test_case: str, traceback: str,
                         stream: TextIO) -> None:
        """Report a failed test case.

        Called just after unittest printed the failure, or at the end of the
        test file in quiet mode (failures are not printed).
        """

    def test_result(self, result: TestResult, runtests: RunTests) -> None:
        """Report the result of a test file, in the main process."""

    def run_failed(self, failed: list[TestResult], state: str) -> None:
        """Report a failed run: failed is the list of failed test files."""


def get_reporter(runtests: RunTests) -> Reporter:
    if runtests.github_summary:
        return GitHubActionsReporter(runtests.github_summary)
    return Reporter()


class GitHubActionsReporter(Reporter):
    """Report test failures to GitHub Actions.

    Annotate failures in the job log and list them in the job summary.
    """

    # Escape the data and the properties of workflow commands
    ESCAPE_DATA = str.maketrans({"%": "%25", "\r": "%0D", "\n": "%0A"})
    ESCAPE_PROPERTY = ESCAPE_DATA | str.maketrans({":": "%3A", ",": "%2C"})

    def __init__(self, summary_filename: str) -> None:
        self.summary_filename = summary_filename

    def test_case_failed(self, test_case: str, traceback: str,
                         stream: TextIO) -> None:
        traceback = decolor(traceback)
        # Get a file + line number from the backtrace: the last frame in a
        # test file, or else the first frame (the test). Without frames, the
        # failure is still annotated, just not located.
        filename = line = None
        if frames := re.findall(r'^ *File "(.*)", line (\d+)', traceback,
                                re.MULTILINE):
            test_frames = [frame for frame in frames
                           if pathlib.Path(frame[0]).name.startswith('test_')]
            filename, line = test_frames[-1] if test_frames else frames[0]
        # Only keep the exception: strip the traceback frames, each one a
        # '  File' line followed by its indented source lines
        message = re.split(r'^  File .*\n(?:    .*\n)*', traceback,
                           flags=re.MULTILINE)[-1].strip()
        self.annotate(test_case, message, filename, line, stream)

    def test_result(self, result: TestResult, runtests: RunTests) -> None:
        # Annotate a test file which failed without failed test cases, for
        # example: crash, timeout, env changed. Failed test cases are
        # annotated by test_case_failed().
        if (not result.is_failed(runtests.fail_env_changed)
                or result.failed_test_cases):
            return
        filename = result.filename
        if not filename:
            # The worker crashed or timed out before reporting the test file:
            # convert python import test name to relative file path.
            # Ex: "test.test_x.test_y" is test_x/test_y.py
            name = result.test_name.removeprefix("test.")
            path = pathlib.Path(findtestdir(runtests.test_dir),
                                *name.split("."))
            if path.with_suffix(".py").exists():
                filename = str(path.with_suffix(".py"))
            elif (path / "__init__.py").exists():
                filename = str(path / "__init__.py")
            # else not a test file (ex: "<regrtest worker>" bug): no location
        message = "\n".join([str(result), *(result.env_changed_reasons or ())])
        self.annotate(result.test_name, decolor(message), filename)

    def annotate(self, title: str, message: str, filename: str | None,
                 line: str | None = None, stream: TextIO | None = None) -> None:
        if filename:
            # Normalize stdlib path to be relative inside the repository
            # checkout.
            path = pathlib.Path(filename)
            stdlib_dir = pathlib.Path(os.__file__).parent
            if path.is_relative_to(stdlib_dir):
                filename = ("Lib" / path.relative_to(stdlib_dir)).as_posix()
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
                # Only default open short tracebacks of up to 5 test cases.
                is_open = ncases <= 5 and traceback.count("\n") < 30
                blocks += [f"<details{' open' if is_open else ''}>"
                           f"<summary>{html.escape(name)}</summary>",
                           f"```pytb\n{decolor(traceback).rstrip()}\n```",
                           "</details>"]
        # Separate Markdown blocks with an empty line
        with open(self.summary_filename, "a", encoding="utf-8") as fp:
            fp.write("".join(f"{block}\n\n" for block in blocks))

