#
# cl.h - Compression Library typedefs and prototypes
#
#   02/18/92	Original Version by Brian Knittel
#

#
# originalFormat parameter values
#
MAX_NUMBER_OF_ORIGINAL_FORMATS = (32)

# Audio
MONO = (0)
STEREO_INTERLEAVED = (1)

# Video 
# YUV is defined to be the same thing as YCrCb (luma and two chroma components).
# 422 is appended to YUV (or YCrCb) if the chroma is sub-sampled by 2 
#	horizontally, packed as U Y1 V Y2 (byte order).
# 422HC is appended to YUV (or YCrCb) if the chroma is sub-sampled by 2 
#	vertically in addition to horizontally, and is packed the same as 
#	422 except that U & V are not valid on the second line.
#
RGB = (0)
RGBX = (1)
RGBA = (2)
RGB332 = (3)

GRAYSCALE = (4)
Y = (4)
YUV = (5)	
YCbCr = (5)	
YUV422 = (6)	# 4:2:2 sampling
YCbCr422 = (6)	# 4:2:2 sampling
YUV422HC = (7)	# 4:1:1 sampling
YCbCr422HC = (7)	# 4:1:1 sampling

MAX_NUMBER_OF_AUDIO_ALGORITHMS = (32)
MAX_NUMBER_OF_VIDEO_ALGORITHMS = (32)

#
# "compressionScheme" argument values
#
UNCOMPRESSED_AUDIO = (0)
G711_ULAW = (1)
ULAW = (1)
G711_ALAW = (2)
ALAW = (2)
G722 = (3)
    
UNCOMPRESSED = (MAX_NUMBER_OF_AUDIO_ALGORITHMS + 0)
UNCOMPRESSED_VIDEO = (MAX_NUMBER_OF_AUDIO_ALGORITHMS + 0)
RLE = (MAX_NUMBER_OF_AUDIO_ALGORITHMS + 1)
JPEG = (MAX_NUMBER_OF_AUDIO_ALGORITHMS + 2)
MPEG_VIDEO = (MAX_NUMBER_OF_AUDIO_ALGORITHMS + 3)
MVC1 = (MAX_NUMBER_OF_AUDIO_ALGORITHMS + 4)
RTR = (MAX_NUMBER_OF_AUDIO_ALGORITHMS + 5)
RTR1 = (MAX_NUMBER_OF_AUDIO_ALGORITHMS + 5)

#
# Parameters
#
MAX_NUMBER_OF_PARAMS = (256)
# Default Parameters
IMAGE_WIDTH = (0)
IMAGE_HEIGHT = (1) 
ORIGINAL_FORMAT = (2)
INTERNAL_FORMAT = (3)
COMPONENTS = (4)
BITS_PER_COMPONENT = (5)
FRAME_RATE = (6)
COMPRESSION_RATIO = (7)
EXACT_COMPRESSION_RATIO = (8)
FRAME_BUFFER_SIZE = (9) 
COMPRESSED_BUFFER_SIZE = (10)
BLOCK_SIZE = (11)
PREROLL = (12)
UNIQUE = (13)
FRAME_TYPE = (14)
OVERWRITE_MODE = (15)
NUMBER_OF_PARAMS = (16)

# JPEG Specific Parameters
QUALITY_FACTOR = (NUMBER_OF_PARAMS + 0)

# MPEG Specific Parameters
SPEED = (NUMBER_OF_PARAMS + 0)
ACTUAL_FRAME_INDEX = (NUMBER_OF_PARAMS + 1)

# RTR Specific Parameters
QUALITY_LEVEL = (NUMBER_OF_PARAMS + 0)

# #define	clTypeIsFloat(v)	(*(float *)&(v))
# #define	clTypeIsLong(v)		(*(long *)&(v))
# 
# RATIO_1 = (65536.0)
# #define clFloatToRatio(f)	((long)((float)(f) * RATIO_1))
# #define clRatioToFloat(f)	((float)(f) / RATIO_1)
# RATIO_SHIFT = (16)
# #define clRatioMul(m, r)	((m) * (r))
# #define clRatioToLong(r)	((r) >> RATIO_SHIFT)
# #define clLongToRatio(r)	((r) << RATIO_SHIFT)

#
# Parameter value types
#
ENUM_VALUE = (0) # only certain constant values are valid
RANGE_VALUE = (1) # any value in a given range is valid
FLOATING_ENUM_VALUE = (2) # only certain constant floating point values are valid
FLOATING_RANGE_VALUE = (3) # any value in a given floating point range is valid
POINTER = (4) # any legal pointer is valid

#
# Algorithm types
#
AUDIO = (0)
VIDEO = (1)

#
# Algorithm Functionality
#
DECOMPRESSOR = (1)
COMPRESSOR = (2)
CODEC = (3)

#
# Buffer types
#
NONE = (0)
FRAME = (1)
DATA = (2)

#
# error codes
#
BAD_NOT_IMPLEMENTED = ( -1) # not impimented yet
BAD_NO_BUFFERSPACE = ( -2) # no space for internal buffers
BAD_BUFFER_NULL = ( -3) # null buffer pointer
BAD_COUNT_NEG = ( -4) # negative count
BAD_PVBUFFER = ( -5) # param/val buffer doesn't make sense
BAD_BUFFERLENGTH_NEG = ( -6) # negative buffer length
BAD_BUFFERLENGTH_ODD = ( -7) # odd length parameter/value buffer
BAD_PARAM = ( -8) # invalid parameter
BAD_COMPRESSION_SCHEME = ( -9) # compression scheme parameter invalid
BAD_COMPRESSOR_HANDLE = (-10) # compression handle parameter invalid
BAD_COMPRESSOR_HANDLE_POINTER = (-11) # compression handle pointer invalid
BAD_BUFFER_HANDLE = (-12) # callback function invalid
BAD_ALGORITHM_INFO = (-13) # algorithm info invalid
BAD_CL_BAD_WIDTH_OR_HEIGHT = (-14) # compressor width or height invalid
BAD_POINTER_FROM_CALLBACK_FUNCTION = (-15) # pointer from callback invalid
JPEG_ERROR = (-16) # error from libjpeg
NO_SEMAPHORE = (-17) # could not get semaphore
BAD_WIDTH_OR_HEIGHT = (-18) # width or height invalid
BAD_FRAME_COUNT = (-19) # frame count invalid
BAD_FRAME_INDEX = (-20) # frame index invalid
BAD_FRAME_BUFFER = (-21) # frame buffer pointer invalid
BAD_FRAME_SIZE = (-22) # frame size invalid
BAD_DATA_BUFFER = (-23) # data buffer pointer invalid
BAD_DATA_SIZE = (-24) # data buffer size invalid
BAD_TOTAL_NUMBER_OF_FRAMES = (-25) # total number of frames invalid
BAD_IMAGE_FORMAT = (-26) # image format invalid
BAD_BITS_PER_COMPONENT = (-27) # bits per component invalid
BAD_FRAME_RATE = (-28) # frame rate invalid
BAD_INSUFFICIENT_DATA_FROM_CALLBACK_FUNCTION = (-29) # insufficient data from callback invalid
PARAM_OUT_OF_RANGE = (-30) # parameter out of range
ADDED_ALGORITHM_ERROR = (-31) # added algorithm had a unique error
BAD_ALGORITHM_TYPE = (-32) # bad algorithm type
BAD_ALGORITHM_NAME = (-33) # bad algorithm name
BAD_FRAME_INDEXING = (-34) # bad frame indexing
BAD_BUFFERING = (-35) # bad buffering calls
BUFFER_NOT_CREATED = (-36) # buffer not created
BAD_BUFFER_EXISTS = (-37) # buffer already created
