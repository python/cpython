"""Pure Python curses-like terminal capability queries."""

from dataclasses import dataclass, field
import errno
import os
from pathlib import Path
import re
import struct


# Terminfo constants
MAGIC16 = 0o432  # Magic number for 16-bit terminfo format
MAGIC32 = 0o1036  # Magic number for 32-bit terminfo format

# Special values for absent/cancelled capabilities
ABSENT_BOOLEAN = -1
ABSENT_NUMERIC = -1
CANCELLED_NUMERIC = -2
ABSENT_STRING = None
CANCELLED_STRING = None

# Standard string capability names from ncurses Caps file
# This matches the order used by ncurses when compiling terminfo
_STRING_CAPABILITY_NAMES = {
    "cbt": 0,
    "bel": 1,
    "cr": 2,
    "csr": 3,
    "tbc": 4,
    "clear": 5,
    "el": 6,
    "ed": 7,
    "hpa": 8,
    "cmdch": 9,
    "cup": 10,
    "cud1": 11,
    "home": 12,
    "civis": 13,
    "cub1": 14,
    "mrcup": 15,
    "cnorm": 16,
    "cuf1": 17,
    "ll": 18,
    "cuu1": 19,
    "cvvis": 20,
    "dch1": 21,
    "dl1": 22,
    "dsl": 23,
    "hd": 24,
    "smacs": 25,
    "blink": 26,
    "bold": 27,
    "smcup": 28,
    "smdc": 29,
    "dim": 30,
    "smir": 31,
    "invis": 32,
    "prot": 33,
    "rev": 34,
    "smso": 35,
    "smul": 36,
    "ech": 37,
    "rmacs": 38,
    "sgr0": 39,
    "rmcup": 40,
    "rmdc": 41,
    "rmir": 42,
    "rmso": 43,
    "rmul": 44,
    "flash": 45,
    "ff": 46,
    "fsl": 47,
    "is1": 48,
    "is2": 49,
    "is3": 50,
    "if": 51,
    "ich1": 52,
    "il1": 53,
    "ip": 54,
    "kbs": 55,
    "ktbc": 56,
    "kclr": 57,
    "kctab": 58,
    "kdch1": 59,
    "kdl1": 60,
    "kcud1": 61,
    "krmir": 62,
    "kel": 63,
    "ked": 64,
    "kf0": 65,
    "kf1": 66,
    "kf10": 67,
    "kf2": 68,
    "kf3": 69,
    "kf4": 70,
    "kf5": 71,
    "kf6": 72,
    "kf7": 73,
    "kf8": 74,
    "kf9": 75,
    "khome": 76,
    "kich1": 77,
    "kil1": 78,
    "kcub1": 79,
    "kll": 80,
    "knp": 81,
    "kpp": 82,
    "kcuf1": 83,
    "kind": 84,
    "kri": 85,
    "khts": 86,
    "kcuu1": 87,
    "rmkx": 88,
    "smkx": 89,
    "lf0": 90,
    "lf1": 91,
    "lf10": 92,
    "lf2": 93,
    "lf3": 94,
    "lf4": 95,
    "lf5": 96,
    "lf6": 97,
    "lf7": 98,
    "lf8": 99,
    "lf9": 100,
    "rmm": 101,
    "smm": 102,
    "nel": 103,
    "pad": 104,
    "dch": 105,
    "dl": 106,
    "cud": 107,
    "ich": 108,
    "indn": 109,
    "il": 110,
    "cub": 111,
    "cuf": 112,
    "rin": 113,
    "cuu": 114,
    "pfkey": 115,
    "pfloc": 116,
    "pfx": 117,
    "mc0": 118,
    "mc4": 119,
    "mc5": 120,
    "rep": 121,
    "rs1": 122,
    "rs2": 123,
    "rs3": 124,
    "rf": 125,
    "rc": 126,
    "vpa": 127,
    "sc": 128,
    "ind": 129,
    "ri": 130,
    "sgr": 131,
    "hts": 132,
    "wind": 133,
    "ht": 134,
    "tsl": 135,
    "uc": 136,
    "hu": 137,
    "iprog": 138,
    "ka1": 139,
    "ka3": 140,
    "kb2": 141,
    "kc1": 142,
    "kc3": 143,
    "mc5p": 144,
    "rmp": 145,
    "acsc": 146,
    "pln": 147,
    "kcbt": 148,
    "smxon": 149,
    "rmxon": 150,
    "smam": 151,
    "rmam": 152,
    "xonc": 153,
    "xoffc": 154,
    "enacs": 155,
    "smln": 156,
    "rmln": 157,
    "kbeg": 158,
    "kcan": 159,
    "kclo": 160,
    "kcmd": 161,
    "kcpy": 162,
    "kcrt": 163,
    "kend": 164,
    "kent": 165,
    "kext": 166,
    "kfnd": 167,
    "khlp": 168,
    "kmrk": 169,
    "kmsg": 170,
    "kmov": 171,
    "knxt": 172,
    "kopn": 173,
    "kopt": 174,
    "kprv": 175,
    "kprt": 176,
    "krdo": 177,
    "kref": 178,
    "krfr": 179,
    "krpl": 180,
    "krst": 181,
    "kres": 182,
    "ksav": 183,
    "kspd": 184,
    "kund": 185,
    "kBEG": 186,
    "kCAN": 187,
    "kCMD": 188,
    "kCPY": 189,
    "kCRT": 190,
    "kDC": 191,
    "kDL": 192,
    "kslt": 193,
    "kEND": 194,
    "kEOL": 195,
    "kEXT": 196,
    "kFND": 197,
    "kHLP": 198,
    "kHOM": 199,
    "kIC": 200,
    "kLFT": 201,
    "kMSG": 202,
    "kMOV": 203,
    "kNXT": 204,
    "kOPT": 205,
    "kPRV": 206,
    "kPRT": 207,
    "kRDO": 208,
    "kRPL": 209,
    "kRIT": 210,
    "kRES": 211,
    "kSAV": 212,
    "kSPD": 213,
    "kUND": 214,
    "rfi": 215,
    "kf11": 216,
    "kf12": 217,
    "kf13": 218,
    "kf14": 219,
    "kf15": 220,
    "kf16": 221,
    "kf17": 222,
    "kf18": 223,
    "kf19": 224,
    "kf20": 225,
    "kf21": 226,
    "kf22": 227,
    "kf23": 228,
    "kf24": 229,
    "kf25": 230,
    "kf26": 231,
    "kf27": 232,
    "kf28": 233,
    "kf29": 234,
    "kf30": 235,
    "kf31": 236,
    "kf32": 237,
    "kf33": 238,
    "kf34": 239,
    "kf35": 240,
    "kf36": 241,
    "kf37": 242,
    "kf38": 243,
    "kf39": 244,
    "kf40": 245,
    "kf41": 246,
    "kf42": 247,
    "kf43": 248,
    "kf44": 249,
    "kf45": 250,
    "kf46": 251,
    "kf47": 252,
    "kf48": 253,
    "kf49": 254,
    "kf50": 255,
    "kf51": 256,
    "kf52": 257,
    "kf53": 258,
    "kf54": 259,
    "kf55": 260,
    "kf56": 261,
    "kf57": 262,
    "kf58": 263,
    "kf59": 264,
    "kf60": 265,
    "kf61": 266,
    "kf62": 267,
    "kf63": 268,
    "el1": 269,
    "mgc": 270,
    "smgl": 271,
    "smgr": 272,
    "fln": 273,
    "sclk": 274,
    "dclk": 275,
    "rmclk": 276,
    "cwin": 277,
    "wingo": 278,
    "hup": 279,
    "dial": 280,
    "qdial": 281,
    "tone": 282,
    "pulse": 283,
    "hook": 284,
    "pause": 285,
    "wait": 286,
    "u0": 287,
    "u1": 288,
    "u2": 289,
    "u3": 290,
    "u4": 291,
    "u5": 292,
    "u6": 293,
    "u7": 294,
    "u8": 295,
    "u9": 296,
    "op": 297,
    "oc": 298,
    "initc": 299,
    "initp": 300,
    "scp": 301,
    "setf": 302,
    "setb": 303,
    "cpi": 304,
    "lpi": 305,
    "chr": 306,
    "cvr": 307,
    "defc": 308,
    "swidm": 309,
    "sdrfq": 310,
    "sitm": 311,
    "slm": 312,
    "smicm": 313,
    "snlq": 314,
    "snrmq": 315,
    "sshm": 316,
    "ssubm": 317,
    "ssupm": 318,
    "sum": 319,
    "rwidm": 320,
    "ritm": 321,
    "rlm": 322,
    "rmicm": 323,
    "rshm": 324,
    "rsubm": 325,
    "rsupm": 326,
    "rum": 327,
    "mhpa": 328,
    "mcud1": 329,
    "mcub1": 330,
    "mcuf1": 331,
    "mvpa": 332,
    "mcuu1": 333,
    "porder": 334,
    "mcud": 335,
    "mcub": 336,
    "mcuf": 337,
    "mcuu": 338,
    "scs": 339,
    "smgb": 340,
    "smgbp": 341,
    "smglp": 342,
    "smgrp": 343,
    "smgt": 344,
    "smgtp": 345,
    "sbim": 346,
    "scsd": 347,
    "rbim": 348,
    "rcsd": 349,
    "subcs": 350,
    "supcs": 351,
    "docr": 352,
    "zerom": 353,
    "csnm": 354,
    "kmous": 355,
    "minfo": 356,
    "reqmp": 357,
    "getm": 358,
    "setaf": 359,
    "setab": 360,
    "pfxl": 361,
    "devt": 362,
    "csin": 363,
    "s0ds": 364,
    "s1ds": 365,
    "s2ds": 366,
    "s3ds": 367,
    "smglr": 368,
    "smgtb": 369,
    "birep": 370,
    "binel": 371,
    "bicr": 372,
    "colornm": 373,
    "defbi": 374,
    "endbi": 375,
    "setcolor": 376,
    "slines": 377,
    "dispc": 378,
    "smpch": 379,
    "rmpch": 380,
    "smsc": 381,
    "rmsc": 382,
    "pctrm": 383,
    "scesc": 384,
    "scesa": 385,
    "ehhlm": 386,
    "elhlm": 387,
    "elohlm": 388,
    "erhlm": 389,
    "ethlm": 390,
    "evhlm": 391,
    "sgr1": 392,
    "slength": 393,
    "OTi2": 394,
    "OTrs": 395,
    "OTnl": 396,
    "OTbc": 397,
    "OTko": 398,
    "OTma": 399,
    "OTG2": 400,
    "OTG3": 401,
    "OTG1": 402,
    "OTG4": 403,
    "OTGR": 404,
    "OTGL": 405,
    "OTGU": 406,
    "OTGD": 407,
    "OTGH": 408,
    "OTGV": 409,
    "OTGC": 410,
    "meml": 411,
    "memu": 412,
    "box1": 413,
}

# Reverse mapping for standard capabilities
_STRING_NAMES: list[str | None] = [None] * 414  # Standard string capabilities

for name, idx in _STRING_CAPABILITY_NAMES.items():
    if idx < len(_STRING_NAMES):
        _STRING_NAMES[idx] = name


def _get_terminfo_dirs() -> list[Path]:
    """Get list of directories to search for terminfo files.

    Based on ncurses behavior in:
    - ncurses/tinfo/db_iterator.c:_nc_next_db()
    - ncurses/tinfo/read_entry.c:_nc_read_entry()
    """
    dirs = []

    terminfo = os.environ.get("TERMINFO")
    if terminfo:
        dirs.append(terminfo)

    try:
        home = Path.home()
        dirs.append(str(home / ".terminfo"))
    except RuntimeError:
        pass

    # Check TERMINFO_DIRS
    terminfo_dirs = os.environ.get("TERMINFO_DIRS", "")
    if terminfo_dirs:
        for d in terminfo_dirs.split(":"):
            if d:
                dirs.append(d)

    dirs.extend(
        [
            "/usr/share/terminfo",
            "/usr/share/misc/terminfo",
            "/usr/local/share/terminfo",
            "/etc/terminfo",
        ]
    )

    return [Path(d) for d in dirs if Path(d).is_dir()]


def _read_terminfo_file(terminal_name: str) -> bytes:
    """Find and read terminfo file for given terminal name.

    Terminfo files are stored in directories using the first character
    of the terminal name as a subdirectory.
    """
    if not isinstance(terminal_name, str):
        raise TypeError("`terminal_name` must be a string")

    if not terminal_name:
        raise ValueError("`terminal_name` cannot be empty")

    first_char = terminal_name[0].lower()
    filename = terminal_name

    for directory in _get_terminfo_dirs():
        path = directory / first_char / filename
        if path.is_file():
            return path.read_bytes()

        # Try with hex encoding of first char (for special chars)
        hex_dir = "%02x" % ord(first_char)
        path = directory / hex_dir / filename
        if path.is_file():
            return path.read_bytes()

    raise FileNotFoundError(errno.ENOENT, os.strerror(errno.ENOENT), filename)


# Hard-coded terminal capabilities for common terminals
# This is a minimal subset needed by PyREPL
_TERMINAL_CAPABILITIES = {
    # ANSI/xterm-compatible terminals
    "ansi": {
        # Bell
        "bel": b"\x07",
        # Cursor movement
        "cub": b"\x1b[%p1%dD",  # Move cursor left N columns
        "cud": b"\x1b[%p1%dB",  # Move cursor down N rows
        "cuf": b"\x1b[%p1%dC",  # Move cursor right N columns
        "cuu": b"\x1b[%p1%dA",  # Move cursor up N rows
        "cub1": b"\x1b[D",  # Move cursor left 1 column
        "cud1": b"\x1b[B",  # Move cursor down 1 row
        "cuf1": b"\x1b[C",  # Move cursor right 1 column
        "cuu1": b"\x1b[A",  # Move cursor up 1 row
        "cup": b"\x1b[%i%p1%d;%p2%dH",  # Move cursor to row, column
        "hpa": b"\x1b[%i%p1%dG",  # Move cursor to column
        # Clear operations
        "clear": b"\x1b[H\x1b[2J",  # Clear screen and home cursor
        "el": b"\x1b[K",  # Clear to end of line
        # Insert/delete
        "dch": b"\x1b[%p1%dP",  # Delete N characters
        "dch1": b"\x1b[P",  # Delete 1 character
        "ich": b"\x1b[%p1%d@",  # Insert N characters
        "ich1": b"\x1b[@",  # Insert 1 character
        # Cursor visibility
        "civis": b"\x1b[?25l",  # Make cursor invisible
        "cnorm": b"\x1b[?25h",  # Make cursor normal (visible)
        # Scrolling
        "ind": b"\n",  # Scroll up one line
        "ri": b"\x1bM",  # Scroll down one line
        # Keypad mode
        "smkx": b"\x1b[?1h\x1b=",  # Enable keypad mode
        "rmkx": b"\x1b[?1l\x1b>",  # Disable keypad mode
        # Padding (not used in modern terminals)
        "pad": b"",
        # Function keys and special keys
        "kdch1": b"\x1b[3~",  # Delete key
        "kcud1": b"\x1b[B",  # Down arrow
        "kend": b"\x1b[F",  # End key
        "kent": b"\x1bOM",  # Enter key
        "khome": b"\x1b[H",  # Home key
        "kich1": b"\x1b[2~",  # Insert key
        "kcub1": b"\x1b[D",  # Left arrow
        "knp": b"\x1b[6~",  # Page down
        "kpp": b"\x1b[5~",  # Page up
        "kcuf1": b"\x1b[C",  # Right arrow
        "kcuu1": b"\x1b[A",  # Up arrow
        # Function keys F1-F20
        "kf1": b"\x1bOP",
        "kf2": b"\x1bOQ",
        "kf3": b"\x1bOR",
        "kf4": b"\x1bOS",
        "kf5": b"\x1b[15~",
        "kf6": b"\x1b[17~",
        "kf7": b"\x1b[18~",
        "kf8": b"\x1b[19~",
        "kf9": b"\x1b[20~",
        "kf10": b"\x1b[21~",
        "kf11": b"\x1b[23~",
        "kf12": b"\x1b[24~",
        "kf13": b"\x1b[25~",
        "kf14": b"\x1b[26~",
        "kf15": b"\x1b[28~",
        "kf16": b"\x1b[29~",
        "kf17": b"\x1b[31~",
        "kf18": b"\x1b[32~",
        "kf19": b"\x1b[33~",
        "kf20": b"\x1b[34~",
    },
    # Dumb terminal - minimal capabilities
    "dumb": {
        "bel": b"\x07",  # Bell
        "cud1": b"\n",  # Move down 1 row (newline)
        "ind": b"\n",  # Scroll up one line (newline)
    },
    # Linux console
    "linux": {
        # Bell
        "bel": b"\x07",
        # Cursor movement
        "cub": b"\x1b[%p1%dD",  # Move cursor left N columns
        "cud": b"\x1b[%p1%dB",  # Move cursor down N rows
        "cuf": b"\x1b[%p1%dC",  # Move cursor right N columns
        "cuu": b"\x1b[%p1%dA",  # Move cursor up N rows
        "cub1": b"\x08",  # Move cursor left 1 column (backspace)
        "cud1": b"\n",  # Move cursor down 1 row (newline)
        "cuf1": b"\x1b[C",  # Move cursor right 1 column
        "cuu1": b"\x1b[A",  # Move cursor up 1 row
        "cup": b"\x1b[%i%p1%d;%p2%dH",  # Move cursor to row, column
        "hpa": b"\x1b[%i%p1%dG",  # Move cursor to column
        # Clear operations
        "clear": b"\x1b[H\x1b[J",  # Clear screen and home cursor (different from ansi!)
        "el": b"\x1b[K",  # Clear to end of line
        # Insert/delete
        "dch": b"\x1b[%p1%dP",  # Delete N characters
        "dch1": b"\x1b[P",  # Delete 1 character
        "ich": b"\x1b[%p1%d@",  # Insert N characters
        "ich1": b"\x1b[@",  # Insert 1 character
        # Cursor visibility
        "civis": b"\x1b[?25l\x1b[?1c",  # Make cursor invisible
        "cnorm": b"\x1b[?25h\x1b[?0c",  # Make cursor normal
        # Scrolling
        "ind": b"\n",  # Scroll up one line
        "ri": b"\x1bM",  # Scroll down one line
        # Keypad mode
        "smkx": b"\x1b[?1h\x1b=",  # Enable keypad mode
        "rmkx": b"\x1b[?1l\x1b>",  # Disable keypad mode
        # Function keys and special keys
        "kdch1": b"\x1b[3~",  # Delete key
        "kcud1": b"\x1b[B",  # Down arrow
        "kend": b"\x1b[4~",  # End key (different from ansi!)
        "khome": b"\x1b[1~",  # Home key (different from ansi!)
        "kich1": b"\x1b[2~",  # Insert key
        "kcub1": b"\x1b[D",  # Left arrow
        "knp": b"\x1b[6~",  # Page down
        "kpp": b"\x1b[5~",  # Page up
        "kcuf1": b"\x1b[C",  # Right arrow
        "kcuu1": b"\x1b[A",  # Up arrow
        # Function keys
        "kf1": b"\x1b[[A",
        "kf2": b"\x1b[[B",
        "kf3": b"\x1b[[C",
        "kf4": b"\x1b[[D",
        "kf5": b"\x1b[[E",
        "kf6": b"\x1b[17~",
        "kf7": b"\x1b[18~",
        "kf8": b"\x1b[19~",
        "kf9": b"\x1b[20~",
        "kf10": b"\x1b[21~",
        "kf11": b"\x1b[23~",
        "kf12": b"\x1b[24~",
        "kf13": b"\x1b[25~",
        "kf14": b"\x1b[26~",
        "kf15": b"\x1b[28~",
        "kf16": b"\x1b[29~",
        "kf17": b"\x1b[31~",
        "kf18": b"\x1b[32~",
        "kf19": b"\x1b[33~",
        "kf20": b"\x1b[34~",
    },
}

# Map common TERM values to capability sets
_TERM_ALIASES = {
    "xterm": "ansi",
    "xterm-color": "ansi",
    "xterm-256color": "ansi",
    "screen": "ansi",
    "screen-256color": "ansi",
    "tmux": "ansi",
    "tmux-256color": "ansi",
    "vt100": "ansi",
    "vt220": "ansi",
    "rxvt": "ansi",
    "rxvt-unicode": "ansi",
    "rxvt-unicode-256color": "ansi",
    "unknown": "dumb",
}


@dataclass
class TermInfo:
    terminal_name: str | bytes | None
    fallback: bool = True

    _names: list[str] = field(default_factory=list)
    _booleans: list[int] = field(default_factory=list)
    _numbers: list[int] = field(default_factory=list)
    _strings: list[bytes | None] = field(default_factory=list)
    _capabilities: dict[str, bytes] = field(default_factory=dict)

    def __post_init__(self) -> None:
        """Initialize terminal capabilities for the given terminal type.

        Based on ncurses implementation in:
        - ncurses/tinfo/lib_setup.c:setupterm() and _nc_setupterm()
        - ncurses/tinfo/lib_setup.c:TINFO_SETUP_TERM()

        This version first attempts to read terminfo database files like ncurses,
        then, if `fallback` is True, falls back to hardcoded capabilities for
        common terminal types.
        """
        # If termstr is None or empty, try to get from environment
        if not self.terminal_name:
            self.terminal_name = os.environ.get("TERM") or "ANSI"

        if isinstance(self.terminal_name, bytes):
            self.terminal_name = self.terminal_name.decode("ascii")

        try:
            self._parse_terminfo_file(self.terminal_name)
        except (OSError, ValueError):
            if not self.fallback:
                raise

            term_type = _TERM_ALIASES.get(
                self.terminal_name, self.terminal_name
            )
            if term_type not in _TERMINAL_CAPABILITIES:
                term_type = "dumb"
            self._capabilities = _TERMINAL_CAPABILITIES[term_type].copy()

    def _parse_terminfo_file(self, terminal_name: str) -> None:
        """Parse a terminfo file.

        Based on ncurses implementation in:
        - ncurses/tinfo/read_entry.c:_nc_read_termtype()
        - ncurses/tinfo/read_entry.c:_nc_read_file_entry()
        """
        data = _read_terminfo_file(terminal_name)
        too_short = f"TermInfo file for {terminal_name!r} too short"
        offset = 0
        if len(data) < 12:
            raise ValueError(too_short)

        magic = struct.unpack("<H", data[offset : offset + 2])[0]
        if magic == MAGIC16:
            number_format = "<h"  # 16-bit signed
            number_size = 2
        elif magic == MAGIC32:
            number_format = "<i"  # 32-bit signed
            number_size = 4
        else:
            raise ValueError(
                f"TermInfo file for {terminal_name!r} uses unknown magic"
            )

        # Parse header
        name_size = struct.unpack("<h", data[2:4])[0]
        bool_count = struct.unpack("<h", data[4:6])[0]
        num_count = struct.unpack("<h", data[6:8])[0]
        str_count = struct.unpack("<h", data[8:10])[0]
        str_size = struct.unpack("<h", data[10:12])[0]

        offset = 12

        # Read terminal names
        if offset + name_size > len(data):
            raise ValueError(too_short)
        names = data[offset : offset + name_size - 1].decode(
            "ascii", errors="ignore"
        )
        offset += name_size

        # Read boolean capabilities
        if offset + bool_count > len(data):
            raise ValueError(too_short)
        booleans = list(data[offset : offset + bool_count])
        offset += bool_count

        # Align to even byte boundary for numbers
        if offset % 2:
            offset += 1

        # Read numeric capabilities
        numbers = []
        for i in range(num_count):
            if offset + number_size > len(data):
                raise ValueError(too_short)
            num = struct.unpack(
                number_format, data[offset : offset + number_size]
            )[0]
            numbers.append(num)
            offset += number_size

        # Read string offsets
        string_offsets = []
        for i in range(str_count):
            if offset + 2 > len(data):
                raise ValueError(too_short)
            off = struct.unpack("<h", data[offset : offset + 2])[0]
            string_offsets.append(off)
            offset += 2

        # Read string table
        if offset + str_size > len(data):
            raise ValueError(too_short)
        string_table = data[offset : offset + str_size]

        # Extract strings from string table
        strings: list[bytes | None] = []
        for off in string_offsets:
            if off < 0:
                strings.append(CANCELLED_STRING)
            elif off < len(string_table):
                # Find null terminator
                end = off
                while end < len(string_table) and string_table[end] != 0:
                    end += 1
                if end <= len(string_table):
                    strings.append(string_table[off:end])
                else:
                    strings.append(ABSENT_STRING)
            else:
                strings.append(ABSENT_STRING)

        self._names = names.split("|")
        self._booleans = booleans
        self._numbers = numbers
        self._strings = strings

    def get(self, cap: str) -> bytes | None:
        """Get terminal capability string by name.

        Based on ncurses implementation in:
        - ncurses/tinfo/lib_ti.c:tigetstr()

        The ncurses version searches through compiled terminfo data structures.
        This version first checks parsed terminfo data, then falls back to
        hardcoded capabilities.
        """
        if not isinstance(cap, str):
            raise TypeError(f"`cap` must be a string, not {type(cap)}")

        if self._capabilities:
            # Fallbacks populated, use them
            return self._capabilities.get(cap)

        # Look up in standard capabilities first
        if cap in _STRING_CAPABILITY_NAMES:
            index = _STRING_CAPABILITY_NAMES[cap]
            if index < len(self._strings):
                return self._strings[index]

        # Note: we don't support extended capabilities since PyREPL doesn't
        # need them.
        return None


def tparm(cap_bytes: bytes, *params: int) -> bytes:
    """Parameterize a terminal capability string.

    Based on ncurses implementation in:
    - ncurses/tinfo/lib_tparm.c:tparm()
    - ncurses/tinfo/lib_tparm.c:tparam_internal()

    The ncurses version implements a full stack-based interpreter for
    terminfo parameter strings. This pure Python version implements only
    the subset of parameter substitution operations needed by PyREPL:
    - %i (increment parameters for 1-based indexing)
    - %p[1-9]%d (parameter substitution)
    - %p[1-9]%{n}%+%d (parameter plus constant)
    """
    if not isinstance(cap_bytes, bytes):
        raise TypeError(f"`cap` must be bytes, not {type(cap_bytes)}")

    result = cap_bytes

    # %i - increment parameters (1-based instead of 0-based)
    increment = b"%i" in result
    if increment:
        result = result.replace(b"%i", b"")

    # Replace %p1%d, %p2%d, etc. with actual parameter values
    for i in range(len(params)):
        pattern = b"%%p%d%%d" % (i + 1)
        if pattern in result:
            value = params[i]
            if increment:
                value += 1
            result = result.replace(pattern, str(value).encode("ascii"))

    # Handle %p1%{1}%+%d (parameter plus constant)
    # Used in some cursor positioning sequences
    pattern_re = re.compile(rb"%p(\d)%\{(\d+)\}%\+%d")
    matches = list(pattern_re.finditer(result))
    for match in reversed(matches):  # reversed to maintain positions
        param_idx = int(match.group(1))
        constant = int(match.group(2))
        value = params[param_idx] + constant
        result = (
            result[: match.start()]
            + str(value).encode("ascii")
            + result[match.end() :]
        )

    return result
