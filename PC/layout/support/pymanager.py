from .constants import *

URL_BASE = "https://www.python.org/ftp/python/"

XYZ_VERSION = f"{VER_MAJOR}.{VER_MINOR}.{VER_MICRO}"
WIN32_VERSION = f"{VER_MAJOR}.{VER_MINOR}.{VER_MICRO}.{VER_FIELD4}"
FULL_VERSION = f"{VER_MAJOR}.{VER_MINOR}.{VER_MICRO}{VER_SUFFIX}"


def _not_empty(n, key=None):
    result = []
    for i in n:
        if key:
            i_l = i[key]
        else:
            i_l = i
        if not i_l:
            continue
        result.append(i)
    return result


def calculate_install_json(ns, *, for_embed=False, for_test=False):
    TARGET = "python.exe"
    TARGETW = "pythonw.exe"

    SYS_ARCH = {
        "win32": "32bit",
        "amd64": "64bit",
        "arm64": "64bit", # Unfortunate, but this is how it's spec'd
    }[ns.arch]
    TAG_ARCH = {
        "win32": "-32",
        "amd64": "-64",
        "arm64": "-arm64",
    }[ns.arch]

    COMPANY = "PythonCore"
    DISPLAY_NAME = "Python"
    TAG_SUFFIX = ""
    ALIAS_PREFIX = "python"
    ALIAS_WPREFIX = "pythonw"
    FILE_PREFIX = "python-"
    FILE_SUFFIX = f"-{ns.arch}"
    DISPLAY_TAGS = [{
        "win32": "32-bit",
        "amd64": "",
        "arm64": "ARM64",
    }[ns.arch]]

    if for_test:
        # Packages with the test suite come under a different Company
        COMPANY = "PythonTest"
        DISPLAY_TAGS.append("with tests")
        FILE_SUFFIX = f"-test-{ns.arch}"
    if for_embed:
        # Embeddable distro comes under a different Company
        COMPANY = "PythonEmbed"
        TARGETW = None
        ALIAS_PREFIX = None
        ALIAS_WPREFIX = None
        DISPLAY_TAGS.append("embeddable")
        # Deliberately name the file differently from the existing distro
        # so we can republish old versions without replacing files.
        FILE_SUFFIX = f"-embeddable-{ns.arch}"
    if ns.include_freethreaded:
        # Free-threaded distro comes with a tag suffix
        TAG_SUFFIX = "t"
        TARGET = f"python{VER_MAJOR}.{VER_MINOR}t.exe"
        TARGETW = f"pythonw{VER_MAJOR}.{VER_MINOR}t.exe"
        DISPLAY_TAGS.append("free-threaded")
        FILE_SUFFIX = f"t-{ns.arch}"

    FULL_TAG = f"{VER_MAJOR}.{VER_MINOR}.{VER_MICRO}{VER_SUFFIX}{TAG_SUFFIX}"
    FULL_ARCH_TAG = f"{FULL_TAG}{TAG_ARCH}"
    XY_TAG = f"{VER_MAJOR}.{VER_MINOR}{TAG_SUFFIX}"
    XY_ARCH_TAG = f"{XY_TAG}{TAG_ARCH}"
    X_TAG = f"{VER_MAJOR}{TAG_SUFFIX}"
    X_ARCH_TAG = f"{X_TAG}{TAG_ARCH}"

    # Tag used in runtime ID (for side-by-side install/updates)
    ID_TAG = XY_ARCH_TAG
    # Tag shown in 'py list' output.
    # gh-139810: We only include '-dev' here for prereleases, even though it
    # works for final releases too.
    DISPLAY_TAG = f"{XY_TAG}-dev{TAG_ARCH}" if VER_SUFFIX else XY_ARCH_TAG
    # Tag used for PEP 514 registration
    SYS_WINVER = XY_TAG + (TAG_ARCH if TAG_ARCH != '-64' else '')

    DISPLAY_SUFFIX = ", ".join(i for i in DISPLAY_TAGS if i)
    if DISPLAY_SUFFIX:
        DISPLAY_SUFFIX = f" ({DISPLAY_SUFFIX})"
    DISPLAY_VERSION = f"{XYZ_VERSION}{VER_SUFFIX}{DISPLAY_SUFFIX}"

    STD_RUN_FOR = []
    STD_ALIAS = []
    STD_PEP514 = []
    STD_START = []
    STD_UNINSTALL = []

    # The list of 'py install <TAG>' tags that will match this runtime.
    # Architecture should always be included here because PyManager will add it.
    INSTALL_TAGS = [
        FULL_ARCH_TAG,
        XY_ARCH_TAG,
        X_ARCH_TAG,
        # gh-139810: The -dev tags are always included so that the latest
        # release is chosen, no matter whether it's prerelease or final.
        f"{XY_TAG}-dev{TAG_ARCH}" if XY_TAG else "",
        f"{X_TAG}-dev{TAG_ARCH}" if X_TAG else "",
    ]

    # Generate run-for entries for each target.
    # Again, include architecture because PyManager will add it.
    for base in [
        {"target": TARGET},
        {"target": TARGETW, "windowed": 1},
    ]:
        if not base["target"]:
            continue
        STD_RUN_FOR.extend([
            {**base, "tag": FULL_ARCH_TAG},
            {**base, "tag": f"{XY_TAG}-dev{TAG_ARCH}" if XY_TAG else ""},
            {**base, "tag": f"{X_TAG}-dev{TAG_ARCH}" if X_TAG else ""},
        ])
        if XY_TAG:
            STD_RUN_FOR.append({**base, "tag": XY_ARCH_TAG})
        if X_TAG:
            STD_RUN_FOR.append({**base, "tag": X_ARCH_TAG})

    # Generate alias entries for each target. We need both arch and non-arch
    # versions as well as windowed/non-windowed versions to make sure that all
    # necessary aliases are created.
    for prefix, base in (
        (ALIAS_PREFIX, {"target": TARGET}),
        (ALIAS_WPREFIX, {"target": TARGETW, "windowed": 1}),
    ):
        if not prefix:
            continue
        if not base["target"]:
            continue
        if XY_TAG:
            STD_ALIAS.extend([
                {**base, "name": f"{prefix}{XY_TAG}.exe"},
                {**base, "name": f"{prefix}{XY_ARCH_TAG}.exe"},
            ])
        if X_TAG:
            STD_ALIAS.extend([
                {**base, "name": f"{prefix}{X_TAG}.exe"},
                {**base, "name": f"{prefix}{X_ARCH_TAG}.exe"},
            ])

    if SYS_WINVER:
        STD_PEP514.append({
            "kind": "pep514",
            "Key": rf"{COMPANY}\{SYS_WINVER}",
            "DisplayName": f"{DISPLAY_NAME} {DISPLAY_VERSION}",
            "SupportUrl": "https://www.python.org/",
            "SysArchitecture": SYS_ARCH,
            "SysVersion": VER_DOT,
            "Version": FULL_VERSION,
            "InstallPath": {
                "_": "%PREFIX%",
                "ExecutablePath": f"%PREFIX%{TARGET}",
                # WindowedExecutablePath is added below
            },
            "Help": {
                "Online Python Documentation": {
                    "_": f"https://docs.python.org/{VER_DOT}/"
                },
            },
        })

    STD_START.append({
        "kind": "start",
        "Name": f"{DISPLAY_NAME} {VER_DOT}{DISPLAY_SUFFIX}",
        "Items": [
            {
                "Name": f"{DISPLAY_NAME} {VER_DOT}{DISPLAY_SUFFIX}",
                "Target": f"%PREFIX%{TARGET}",
                "Icon": f"%PREFIX%{TARGET}",
            },
            {
                "Name": f"{DISPLAY_NAME} {VER_DOT} Online Documentation",
                "Icon": r"%SystemRoot%\System32\SHELL32.dll",
                "IconIndex": 13,
                "Target": f"https://docs.python.org/{VER_DOT}/",
            },
            # IDLE and local documentation items are added below
        ],
    })

    if TARGETW and STD_PEP514:
        STD_PEP514[0]["InstallPath"]["WindowedExecutablePath"] = f"%PREFIX%{TARGETW}"

    if ns.include_idle:
        STD_START[0]["Items"].append({
            "Name": f"IDLE (Python {VER_DOT}{DISPLAY_SUFFIX})",
            "Target": f"%PREFIX%{TARGETW or TARGET}",
            "Arguments": r'"%PREFIX%Lib\idlelib\idle.pyw"',
            "Icon": r"%PREFIX%Lib\idlelib\Icons\idle.ico",
            "IconIndex": 0,
        })
        STD_START[0]["Items"].append({
            "Name": f"PyDoc (Python {VER_DOT}{DISPLAY_SUFFIX})",
            "Target": f"%PREFIX%{TARGET}",
            "Arguments": "-m pydoc -b",
            "Icon": r"%PREFIX%Lib\idlelib\Icons\idle.ico",
            "IconIndex": 0,
        })
        if STD_PEP514:
            STD_PEP514[0]["InstallPath"]["IdlePath"] = f"%PREFIX%Lib\\idlelib\\idle.pyw"

    if ns.include_html_doc:
        STD_PEP514[0]["Help"]["Main Python Documentation"] = {
            "_": rf"%PREFIX%Doc\html\index.html",
        }
        STD_START[0]["Items"].append({
            "Name": f"{DISPLAY_NAME} {VER_DOT} Manuals{DISPLAY_SUFFIX}",
            "Target": r"%PREFIX%Doc\html\index.html",
        })
    elif ns.include_chm:
        STD_PEP514[0]["Help"]["Main Python Documentation"] = {
            "_": rf"%PREFIX%Doc\{PYTHON_CHM_NAME}",
        }
        STD_START[0]["Items"].append({
            "Name": f"{DISPLAY_NAME} {VER_DOT} Manuals{DISPLAY_SUFFIX}",
            "Target": "%WINDIR%hhc.exe",
            "Arguments": rf"%PREFIX%Doc\{PYTHON_CHM_NAME}",
        })

    STD_UNINSTALL.append({
        "kind": "uninstall",
        # Other settings will pick up sensible defaults
        "Publisher": "Python Software Foundation",
        "HelpLink": f"https://docs.python.org/{VER_DOT}/",
    })

    data = {
        "schema": 1,
        "id": f"{COMPANY.lower()}-{ID_TAG}",
        "sort-version": FULL_VERSION,
        "company": COMPANY,
        "tag": DISPLAY_TAG,
        "install-for": _not_empty(INSTALL_TAGS),
        "run-for": _not_empty(STD_RUN_FOR, "tag"),
        "alias": _not_empty(STD_ALIAS, "name"),
        "shortcuts": [
            *STD_PEP514,
            *STD_START,
            *STD_UNINSTALL,
        ],
        "display-name": f"{DISPLAY_NAME} {DISPLAY_VERSION}",
        "executable": rf".\{TARGET}",
        "url": f"{URL_BASE}{XYZ_VERSION}/{FILE_PREFIX}{FULL_VERSION}{FILE_SUFFIX}.zip"
    }

    return data
