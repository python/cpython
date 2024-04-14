import dataclasses as dc


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
