import dataclasses as dc
from typing import Literal,  NoReturn, overload


@dc.dataclass
class ClinicError(Exception):
    message: str
    _: dc.KW_ONLY
    lineno: int | None = None
    filename: str | None = None

    def __post_init__(self) -> None:
        super().__init__(self.message)

    def report(self, *, warn_only: bool = False) -> str:
        msg = "Warning" if warn_only else "Error"
        if self.filename is not None:
            msg += f" in file {self.filename!r}"
        if self.lineno is not None:
            msg += f" on line {self.lineno}"
        msg += ":\n"
        msg += f"{self.message}\n"
        return msg


class ParseError(ClinicError):
    pass


@overload
def warn_or_fail(
    *args: object,
    fail: Literal[True],
    filename: str | None = None,
    line_number: int | None = None,
) -> NoReturn: ...

@overload
def warn_or_fail(
    *args: object,
    fail: Literal[False] = False,
    filename: str | None = None,
    line_number: int | None = None,
) -> None: ...

def warn_or_fail(
    *args: object,
    fail: bool = False,
    filename: str | None = None,
    line_number: int | None = None,
) -> None:
    joined = " ".join([str(a) for a in args])
    error = ClinicError(joined, filename=filename, lineno=line_number)
    if fail:
        raise error
    else:
        print(error.report(warn_only=True))


def warn(
    *args: object,
    filename: str | None = None,
    line_number: int | None = None,
) -> None:
    return warn_or_fail(*args, filename=filename, line_number=line_number, fail=False)

def fail(
    *args: object,
    filename: str | None = None,
    line_number: int | None = None,
) -> NoReturn:
    warn_or_fail(*args, filename=filename, line_number=line_number, fail=True)
