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
    fail_fast: bool = False
    fail_env_changed: bool = False
    match_tests: FilterTuple | None = None
    ignore_tests: FilterTuple | None = None
    match_tests_dict: FilterDict | None = None
    rerun: bool = False
    forever: bool = False
    pgo: bool = False
    pgo_extended: bool = False
    output_on_failure: bool = False
    timeout: float | None = None
    verbose: int = 0
    quiet: bool = False
    hunt_refleak: HuntRefleak | None = None
    test_dir: StrPath | None = None
    use_junit: bool = False
    memory_limit: str | None = None
    gc_threshold: int | None = None
    use_resources: list[str] = dataclasses.field(default_factory=list)
    python_cmd: list[str] | None = None

    def copy(self, **override):
        state = dataclasses.asdict(self)
        state.update(override)
        return RunTests(**state)

    def get_match_tests(self, test_name) -> FilterTuple | None:
        if self.match_tests_dict is not None:
            return self.match_tests_dict.get(test_name, None)
        else:
            return None

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
