import dataclasses
import json
from _colorize import get_colors  # type: ignore[import-not-found]
from typing import Any

from .utils import (
    StrJSON, TestName, FilterTuple,
    format_duration, normalize_test_name, print_warning)


@dataclasses.dataclass(slots=True)
class TestStats:
    tests_run: int = 0
    failures: int = 0
    skipped: int = 0

    @staticmethod
    def from_unittest(result):
        return TestStats(result.testsRun,
                         len(result.failures),
                         len(result.skipped))

    @staticmethod
    def from_doctest(results):
        return TestStats(results.attempted,
                         results.failed,
                         results.skipped)

    def accumulate(self, stats):
        self.tests_run += stats.tests_run
        self.failures += stats.failures
        self.skipped += stats.skipped


# Avoid enum.Enum to reduce the number of imports when tests are run
class State:
    PASSED = "PASSED"
    FAILED = "FAILED"
    SKIPPED = "SKIPPED"
    UNCAUGHT_EXC = "UNCAUGHT_EXC"
    REFLEAK = "REFLEAK"
    ENV_CHANGED = "ENV_CHANGED"
    RESOURCE_DENIED = "RESOURCE_DENIED"
    INTERRUPTED = "INTERRUPTED"
    WORKER_FAILED = "WORKER_FAILED"   # non-zero worker process exit code
    WORKER_BUG = "WORKER_BUG"         # exception when running a worker
    DID_NOT_RUN = "DID_NOT_RUN"
    TIMEOUT = "TIMEOUT"

    @staticmethod
    def is_failed(state):
        return state in {
            State.FAILED,
            State.UNCAUGHT_EXC,
            State.REFLEAK,
            State.WORKER_FAILED,
            State.WORKER_BUG,
            State.TIMEOUT}

    @staticmethod
    def has_meaningful_duration(state):
        # Consider that the duration is meaningless for these cases.
        # For example, if a whole test file is skipped, its duration
        # is unlikely to be the duration of executing its tests,
        # but just the duration to execute code which skips the test.
        return state not in {
            State.SKIPPED,
            State.RESOURCE_DENIED,
            State.INTERRUPTED,
            State.WORKER_FAILED,
            State.WORKER_BUG,
            State.DID_NOT_RUN}

    @staticmethod
    def must_stop(state):
        return state in {
            State.INTERRUPTED,
            State.WORKER_BUG,
        }


FileName = str
LineNo = int
Location = tuple[FileName, LineNo]


@dataclasses.dataclass(slots=True)
class TestResult:
    test_name: TestName
    state: str | None = None
    # Test duration in seconds
    duration: float | None = None
    xml_data: list[str] | None = None
    stats: TestStats | None = None

    # errors and failures copied from support.TestFailedWithDetails
    errors: list[tuple[str, str]] | None = None
    failures: list[tuple[str, str]] | None = None

    # partial coverage in a worker run; not used by sequential in-process runs
    covered_lines: list[Location] | None = None

    def is_failed(self, fail_env_changed: bool) -> bool:
        if self.state == State.ENV_CHANGED:
            return fail_env_changed
        return State.is_failed(self.state)

    def _format_failed(self):
        ansi = get_colors()
        red, reset = ansi.BOLD_RED, ansi.RESET
        if self.errors and self.failures:
            le = len(self.errors)
            lf = len(self.failures)
            error_s = "error" + ("s" if le > 1 else "")
            failure_s = "failure" + ("s" if lf > 1 else "")
            return (
                f"{red}{self.test_name} failed "
                f"({le} {error_s}, {lf} {failure_s}){reset}"
            )

        if self.errors:
            le = len(self.errors)
            error_s = "error" + ("s" if le > 1 else "")
            return f"{red}{self.test_name} failed ({le} {error_s}){reset}"

        if self.failures:
            lf = len(self.failures)
            failure_s = "failure" + ("s" if lf > 1 else "")
            return f"{red}{self.test_name} failed ({lf} {failure_s}){reset}"

        return f"{red}{self.test_name} failed{reset}"

    def __str__(self) -> str:
        ansi = get_colors()
        green = ansi.GREEN
        red = ansi.BOLD_RED
        reset = ansi.RESET
        yellow = ansi.YELLOW

        match self.state:
            case State.PASSED:
                return f"{green}{self.test_name} passed{reset}"
            case State.FAILED:
                return f"{red}{self._format_failed()}{reset}"
            case State.SKIPPED:
                return f"{yellow}{self.test_name} skipped{reset}"
            case State.UNCAUGHT_EXC:
                return (
                    f"{red}{self.test_name} failed (uncaught exception){reset}"
                )
            case State.REFLEAK:
                return f"{red}{self.test_name} failed (reference leak){reset}"
            case State.ENV_CHANGED:
                return f"{red}{self.test_name} failed (env changed){reset}"
            case State.RESOURCE_DENIED:
                return f"{yellow}{self.test_name} skipped (resource denied){reset}"
            case State.INTERRUPTED:
                return f"{yellow}{self.test_name} interrupted{reset}"
            case State.WORKER_FAILED:
                return (
                    f"{red}{self.test_name} worker non-zero exit code{reset}"
                )
            case State.WORKER_BUG:
                return f"{red}{self.test_name} worker bug{reset}"
            case State.DID_NOT_RUN:
                return f"{yellow}{self.test_name} ran no tests{reset}"
            case State.TIMEOUT:
                assert self.duration is not None, "self.duration is None"
                return f"{self.test_name} timed out ({format_duration(self.duration)})"
            case _:
                raise ValueError(
                    f"{red}unknown result state: {{state!r}}{reset}"
                )

    def has_meaningful_duration(self):
        return State.has_meaningful_duration(self.state)

    def set_env_changed(self):
        if self.state is None or self.state == State.PASSED:
            self.state = State.ENV_CHANGED

    def must_stop(self, fail_fast: bool, fail_env_changed: bool) -> bool:
        if State.must_stop(self.state):
            return True
        if fail_fast and self.is_failed(fail_env_changed):
            return True
        return False

    def get_rerun_match_tests(self) -> FilterTuple | None:
        match_tests = []

        errors = self.errors or []
        failures = self.failures or []
        for error_list, is_error in (
            (errors, True),
            (failures, False),
        ):
            for full_name, *_ in error_list:
                match_name = normalize_test_name(full_name, is_error=is_error)
                if match_name is None:
                    # 'setUpModule (test.test_sys)': don't filter tests
                    return None
                if not match_name:
                    error_type = "ERROR" if is_error else "FAIL"
                    print_warning(f"rerun failed to parse {error_type} test name: "
                                  f"{full_name!r}: don't filter tests")
                    return None
                match_tests.append(match_name)

        if not match_tests:
            return None
        return tuple(match_tests)

    def write_json_into(self, file) -> None:
        json.dump(self, file, cls=_EncodeTestResult)

    @staticmethod
    def from_json(worker_json: StrJSON) -> 'TestResult':
        return json.loads(worker_json, object_hook=_decode_test_result)


class _EncodeTestResult(json.JSONEncoder):
    def default(self, o: Any) -> dict[str, Any]:
        if isinstance(o, TestResult):
            result = dataclasses.asdict(o)
            result["__test_result__"] = o.__class__.__name__
            return result
        else:
            return super().default(o)


def _decode_test_result(data: dict[str, Any]) -> TestResult | dict[str, Any]:
    if "__test_result__" in data:
        data.pop('__test_result__')
        if data['stats'] is not None:
            data['stats'] = TestStats(**data['stats'])
        if data['covered_lines'] is not None:
            data['covered_lines'] = [
                tuple(loc) for loc in data['covered_lines']
            ]
        return TestResult(**data)
    else:
        return data
