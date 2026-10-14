import html
import os
import pathlib
import re
from _colorize import decolor  # type: ignore[import-not-found]
from typing import TextIO

from .result import TestResult
lazy from .results import TestResults
from .runtests import RunTests
from .utils import count


class Reporter:
    """Report test failures, for example to a CI service.

    The default reporter does nothing.
    """

    def run_started(self, runtests: RunTests) -> None:
        """Start a run of tests.

        Called in the main process at the start of each run (the first run
        and the re-run), and in the process running the tests before each
        test file.
        """

    def test_case_failed(self, test_case: str, traceback: str,
                         stream: TextIO) -> None:
        """Report a failed test case.

        Called just after unittest printed the failure, or at the end of the
        test file in quiet mode.
        """

    def test_result(self, result: TestResult) -> None:
        """Report the result of a test file, in the main process."""

    def summary(self, results: TestResults, state: str) -> None:
        """Report the results of all runs, in the main process."""

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


def get_reporter() -> Reporter:
    if GitHubActionsReporter.enabled():
        return GitHubActionsReporter()
    return Reporter()


class GitHubActionsReporter(Reporter):
    """Annotate failures in the GitHub Actions job log and list them in the
    job summary."""

    # Escape the data and the properties of workflow commands
    ESCAPE_DATA = str.maketrans({"%": "%25", "\r": "%0D", "\n": "%0A"})
    ESCAPE_PROPERTY = ESCAPE_DATA | str.maketrans({":": "%3A", ",": "%2C"})

    @staticmethod
    def enabled() -> bool:
        return "GITHUB_STEP_SUMMARY" in os.environ

    def run_started(self, runtests: RunTests) -> None:
        self.runtests = runtests

    def test_case_failed(self, test_case: str, traceback: str,
                         stream: TextIO) -> None:
        # Only report the failures which decide the result
        if not self.runtests.final_run:
            return
        # The job log has the full traceback: only keep the exception
        filename, line, exception = self.parse_traceback(decolor(traceback))
        self.annotate(test_case, exception, filename, line, stream)

    def test_result(self, result: TestResult) -> None:
        # Failed test cases are annotated by test_case_failed()
        if (not self.runtests.final_run
                or not result.is_failed(self.runtests.fail_env_changed)
                or result.failed_test_cases):
            return
        message = "\n".join([str(result), *(result.env_changed_reasons or ())])
        self.annotate(result.test_name, decolor(message),
                      result.find_filename(self.runtests.test_dir))

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

    def summary(self, results: TestResults, state: str) -> None:
        # Test files which failed in the last run (the re-run, if any)
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
        with open(os.environ["GITHUB_STEP_SUMMARY"], "a",
                  encoding="utf-8") as fp:
            fp.write("".join(f"{block}\n\n" for block in blocks))
