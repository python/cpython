import dataclasses
import json
from typing import Any

from .utils import (
    StrPath, StrJSON, TestTuple, FilterTuple, FilterDict)


@dataclasses.dataclass(slots=True, frozen=True)
class HuntRefleak:
    warmups: int
    runs: int
    filename: StrPath


@dataclasses.dataclass(slots=True, frozen=True)
class RunTests:
    tests: TestTuple
    fail_fast: bool
    fail_env_changed: bool
    match_tests: FilterTuple | None
    ignore_tests: FilterTuple | None
    match_tests_dict: FilterDict | None
    rerun: bool
    forever: bool
    pgo: bool
    pgo_extended: bool
    output_on_failure: bool
    timeout: float | None
    verbose: int
    quiet: bool
    hunt_refleak: HuntRefleak | None
    test_dir: StrPath | None
    use_junit: bool
    memory_limit: str | None
    gc_threshold: int | None
    use_resources: tuple[str]
    python_cmd: tuple[str] | None
    randomize: bool
    random_seed: int | None
    # On Unix, it's a file descriptor.
    # On Windows, it's a handle.
    json_fd: int | None

    def copy(self, **override):
        state = dataclasses.asdict(self)
        state.update(override)
        return RunTests(**state)

    def get_match_tests(self, test_name) -> FilterTuple | None:
        if self.match_tests_dict is not None:
            return self.match_tests_dict.get(test_name, None)
        else:
            return None

    def get_jobs(self):
        # Number of run_single_test() calls needed to run all tests.
        # None means that there is not bound limit (--forever option).
        if self.forever:
            return None
        return len(self.tests)

    def iter_tests(self):
        if self.forever:
            while True:
                yield from self.tests
        else:
            yield from self.tests

    def as_json(self) -> StrJSON:
        return json.dumps(self, cls=_EncodeRunTests)

    @staticmethod
    def from_json(worker_json: StrJSON) -> 'RunTests':
        return json.loads(worker_json, object_hook=_decode_runtests)


class _EncodeRunTests(json.JSONEncoder):
    def default(self, o: Any) -> dict[str, Any]:
        if isinstance(o, RunTests):
            result = dataclasses.asdict(o)
            result["__runtests__"] = True
            return result
        else:
            return super().default(o)


def _decode_runtests(data: dict[str, Any]) -> RunTests | dict[str, Any]:
    if "__runtests__" in data:
        data.pop('__runtests__')
        if data['hunt_refleak']:
            data['hunt_refleak'] = HuntRefleak(**data['hunt_refleak'])
        return RunTests(**data)
    else:
        return data
