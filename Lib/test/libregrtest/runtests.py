import dataclasses
import json
from typing import Any

from . import FilterTuple


TestsTuple = tuple[str, ...]
FilterDict = dict[str, FilterTuple]


@dataclasses.dataclass(slots=True, frozen=True)
class RunTests:
    tests: TestsTuple
    # --match option
    match_tests: FilterTuple | None = None
    # --ignore option
    ignore_tests: FilterTuple | None = None
    # used by --rerun
    match_tests_dict: FilterDict | None = None
    rerun: bool = False
    forever: bool = False
    use_junit: bool = False
    fail_env_changed: bool = False
    fail_fast: bool = False
    pgo: bool = False
    pgo_extended: bool = False
    verbose: int = False
    quiet: bool = False
    timeout: float | None = None
    # --verbose3 option
    output_on_failure: bool = False
    test_dir: str | None = None
    huntrleaks: tuple[int, int, str] | None = None
    memlimit: str | None = None
    gc_threshold: int | None = None
    use_resources: list[str] = dataclasses.field(default_factory=list)
    python_executable: list[str] | None = None

    def get_match_tests(self, test_name: str) -> FilterTuple | None:
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

    def copy(self, **override):
        state = dataclasses.asdict(self)
        state.update(override)
        return RunTests(**state)

    def as_json(self):
        return json.dumps(self, cls=_EncodeRunTests)

    def from_json(worker_json):
        return json.loads(worker_json, object_hook=_decode_runtests)



class _EncodeRunTests(json.JSONEncoder):
    def default(self, o: Any) -> dict[str, Any]:
        if isinstance(o, RunTests):
            result = dataclasses.asdict(o)
            result["__runtests__"] = True
            return result
        else:
            return super().default(o)


def _decode_runtests(d: dict[str, Any]) -> RunTests | dict[str, Any]:
    if "__runtests__" in d:
        d.pop('__runtests__')
        return RunTests(**d)
    else:
        return d
