from warnings import warnpy3k
warnpy3k("the SV module has been removed in Python 3.0", stacklevel=2)
del warnpy3k

NTSC_XMAX = 640
NTSC_YMAX = 480
PAL_XMAX = 768
PAL_YMAX = 576
BLANKING_BUFFER_SIZE = 2

MAX_SOURCES = 2

# mode parameter for Bind calls
IN_OFF = 0                              # No Video
IN_OVER = 1                             # Video over graphics
IN_UNDER = 2                            # Video under graphics
IN_REPLACE = 3                          # Video replaces entire win

# mode parameters for LoadMap calls.  Specifies buffer, always 256 entries
INPUT_COLORMAP = 0                      # tuples of 8-bit RGB
CHROMA_KEY_MAP = 1                      # tuples of 8-bit RGB
COLOR_SPACE_MAP = 2                     # tuples of 8-bit RGB
GAMMA_MAP = 3                           # tuples of 24-bit red values

# mode parameters for UseExclusive calls
INPUT = 0
OUTPUT = 1
IN_OUT = 2

# Format constants for the capture routines
RGB8_FRAMES = 0                         # noninterleaved 8 bit 3:2:3 RBG fields
RGB32_FRAMES = 1                        # 32-bit 8:8:8 RGB frames
YUV411_FRAMES = 2                       # interleaved, 8:2:2 YUV format
YUV411_FRAMES_AND_BLANKING_BUFFER = 3

#
# sv.SetParam is passed variable length argument lists,
# consisting of <name, value> pairs.   The following
# constants identify argument names.
#
_NAME_BASE = 1000
SOURCE = (_NAME_BASE + 0)
SOURCE1 = 0
SOURCE2 = 1
SOURCE3 = 2
COLOR = (_NAME_BASE + 1)
DEFAULT_COLOR = 0
USER_COLOR = 1
MONO = 2
OUTPUTMODE = (_NAME_BASE + 2)
LIVE_OUTPUT = 0
STILL24_OUT = 1
FREEZE = (_NAME_BASE + 3)
DITHER = (_NAME_BASE + 4)
OUTPUT_FILTER = (_NAME_BASE + 5)
HUE = (_NAME_BASE + 6)
GENLOCK = (_NAME_BASE + 7)
GENLOCK_OFF = 0
GENLOCK_ON = 1
GENLOCK_HOUSE = 2
BROADCAST = (_NAME_BASE + 8)
NTSC = 0
PAL = 1
VIDEO_MODE = (_NAME_BASE + 9)
COMP = 0
SVIDEO = 1
INPUT_BYPASS = (_NAME_BASE + 10)
FIELDDROP = (_NAME_BASE + 11)
SLAVE = (_NAME_BASE + 12)
APERTURE_FACTOR = (_NAME_BASE + 13)
AFACTOR_0 = 0
AFACTOR_QTR = 1
AFACTOR_HLF = 2
AFACTOR_ONE = 3
CORING = (_NAME_BASE + 14)
COR_OFF = 0
COR_1LSB = 1
COR_2LSB = 2
COR_3LSB = 3
APERTURE_BANDPASS = (_NAME_BASE + 15)
ABAND_F0 = 0
ABAND_F1 = 1
ABAND_F2 = 2
ABAND_F3 = 3
PREFILTER = (_NAME_BASE + 16)
CHROMA_TRAP = (_NAME_BASE + 17)
CK_THRESHOLD = (_NAME_BASE + 18)
PAL_SENSITIVITY = (_NAME_BASE + 19)
GAIN_CONTROL = (_NAME_BASE + 20)
GAIN_SLOW = 0
GAIN_MEDIUM = 1
GAIN_FAST = 2
GAIN_FROZEN = 3
AUTO_CKILL = (_NAME_BASE + 21)
VTR_MODE = (_NAME_BASE + 22)
VTR_INPUT = 0
CAMERA_INPUT = 1
LUMA_DELAY = (_NAME_BASE + 23)
VNOISE = (_NAME_BASE + 24)
VNOISE_NORMAL = 0
VNOISE_SEARCH = 1
VNOISE_AUTO = 2
VNOISE_BYPASS = 3
CHCV_PAL = (_NAME_BASE + 25)
CHCV_NTSC = (_NAME_BASE + 26)
CCIR_LEVELS = (_NAME_BASE + 27)
STD_CHROMA = (_NAME_BASE + 28)
DENC_VTBYPASS = (_NAME_BASE + 29)
FAST_TIMECONSTANT = (_NAME_BASE + 30)
GENLOCK_DELAY = (_NAME_BASE + 31)
PHASE_SYNC = (_NAME_BASE + 32)
VIDEO_OUTPUT = (_NAME_BASE + 33)
CHROMA_PHASEOUT = (_NAME_BASE + 34)
CHROMA_CENTER = (_NAME_BASE + 35)
YUV_TO_RGB_INVERT = (_NAME_BASE + 36)
SOURCE1_BROADCAST = (_NAME_BASE + 37)
SOURCE1_MODE = (_NAME_BASE + 38)
SOURCE2_BROADCAST = (_NAME_BASE + 39)
SOURCE2_MODE = (_NAME_BASE + 40)
SOURCE3_BROADCAST = (_NAME_BASE + 41)
SOURCE3_MODE = (_NAME_BASE + 42)
SIGNAL_STD = (_NAME_BASE + 43)
NOSIGNAL = 2
SIGNAL_COLOR = (_NAME_BASE + 44)
