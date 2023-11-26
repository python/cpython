from pathlib import Path

ROOT = Path(__file__).parent.parent.parent
DEFAULT_INPUT = (ROOT / "Python/bytecodes.c").absolute()

def root_relative_path(filename: str) -> str:
    return Path(filename).relative_to(ROOT).as_posix()
