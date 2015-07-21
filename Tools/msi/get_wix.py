'''
Downloads and extracts WiX to a local directory
'''

__author__ = 'Steve Dower <steve.dower@microsoft.com>'

import io
import os
import sys

from pathlib import Path
from subprocess import Popen
from zipfile import ZipFile

EXTERNALS_DIR = None
for p in (Path.cwd() / __file__).parents:
    if any(p.glob("PCBuild/*.vcxproj")):
        EXTERNALS_DIR = p / "externals"
        break

if not EXTERNALS_DIR:
    print("Cannot find project root")
    sys.exit(1)

WIX_BINARIES_ZIP = 'http://wixtoolset.org/downloads/v3.10.0.1823/wix310-binaries.zip'
TARGET_BIN_ZIP = EXTERNALS_DIR / "wix.zip"
TARGET_BIN_DIR = EXTERNALS_DIR / "wix"

POWERSHELL_COMMAND = "[IO.File]::WriteAllBytes('{}', (Invoke-WebRequest {} -UseBasicParsing).Content)"

if __name__ == '__main__':
    if TARGET_BIN_DIR.exists() and any(TARGET_BIN_DIR.glob("*")):
        print('WiX is already installed')
        sys.exit(0)

    try:
        TARGET_BIN_DIR.mkdir()
    except FileExistsError:
        pass

    print('Downloading WiX to', TARGET_BIN_ZIP)
    p = Popen(["powershell.exe", "-Command", POWERSHELL_COMMAND.format(TARGET_BIN_ZIP, WIX_BINARIES_ZIP)])
    p.wait()
    print('Extracting WiX to', TARGET_BIN_DIR)
    with ZipFile(str(TARGET_BIN_ZIP)) as z:
        z.extractall(str(TARGET_BIN_DIR))
    TARGET_BIN_ZIP.unlink()

    print('Extracted WiX')
