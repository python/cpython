import re
from pathlib import Path

from typing import Iterable

from test.support.project_files_helper import iter_all_c_files


def iter_global_strings() -> Iterable[str]:
    id_regex = re.compile(r"\b_Py_ID\((\w+)\)")
    str_regex = re.compile(r'\b_Py_DECLARE_STR\((?:\w+), "(.*?)"\)')
    for filename in iter_all_c_files():
        infile = Path(filename)
        if not infile.exists():
            # The file must have been a temporary file.
            continue
        with infile.open(encoding="utf-8") as infile_open:
            for line in infile_open:
                for m in id_regex.finditer(line):
                    yield m.group(1)
                for m in str_regex.finditer(line):
                    yield m.group(1)
