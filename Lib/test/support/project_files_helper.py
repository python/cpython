from pathlib import Path

from typing import Iterable

python_root = Path(__file__).resolve().parents[3]


def iter_all_c_files() -> Iterable[Path]:
    for top_directory_name in (
        "Modules",
        "Objects",
        "Parser",
        "PC",
        "Programs",
        "Python",
    ):
        for dirname, _, files in (python_root / top_directory_name).walk():
            for name in files:
                if not name.endswith((".c", ".h")):
                    continue
                yield dirname / name
