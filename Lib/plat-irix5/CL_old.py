#
# cl.h - Compression Library typedefs and prototypes
#
#   01/07/92	Cleanup by Brian Knittel
#   02/18/92	Original Version by Brian Knittel
#

#
# originalFormat parameter values
#
MAX_NUMBER_OF_ORIGINAL_FORMATS = 32

# Audio
MONO = 0
STEREO_INTERLEAVED = 1

# Video 
# YUV is defined to be the same thing as YCrCb (luma and two chroma components).
# 422 is appended to YUV (or YCrCb) if the chroma is sub-sampled by 2 
#	horizontally, packed as U Y1 V Y2 (byte order).
# 422HC is appended to YUV (or YCrCb) if the chroma is sub-sampled by 2 
#	vertically in addition to horizontally, and is packed the same as 
#	422 except that U & V are not valid on the second line.
#
RGB = 0
RGBX = 1
RGBA = 2
RGB332 = 3

GRAYSCALE = 4
Y = 4
YUV = 5	
YCbCr = 5	
YUV422 = 6				# 4:2:2 sampling
YCbCr422 = 6				# 4:2:2 sampling
YUV422HC = 7				# 4:1:1 sampling
YCbCr422HC = 7				# 4:1:1 sampling
YUV422DC = 7				# 4:1:1 sampling
YCbCr422DC = 7				# 4:1:1 sampling

BEST_FIT = -1	

def BytesPerSample(s):
	if s in (MONO, YUV):
		return 2
	elif s == STEREO_INTERLEAVED:
		return 4
	else:
		return 0

def BytesPerPixel(f):
	if f in (RGB, YUV):
		return 3
	elif f in (RGBX, RGBA):
		return 4
	elif f in (RGB332, GRAYSCALE):
		return 1
	else:
		return 2

def AudioFormatName(f):
	if f == MONO:
		return 'MONO'
	elif f == STEREO_INTERLEAVED:
		return 'STEREO_INTERLEAVED'
	else:
		return 'Not a valid format'

def VideoFormatName(f):
	if f == RGB:
		return 'RGB'
	elif f == RGBX:
		return 'RGBX'
	elif f == RGBA:
		return 'RGBA'
	elif f == RGB332:
		return 'RGB332'
	elif f == GRAYSCALE:
		return 'GRAYSCALE'
	elif f == YUV:
		return 'YUV'
	elif f == YUV422:
		return 'YUV422'
	elif f == YUV422DC:
		return 'YUV422DC'
	else:
		return 'Not a valid format'

MAX_NUMBER_OF_AUDIO_ALGORITHMS = 32
MAX_NUMBER_OF_VIDEO_ALGORITHMS = 32

#
# Algorithm types
#
AUDIO = 0
VIDEO = 1

def AlgorithmNumber(scheme):
	return scheme & 0x7fff
def AlgorithmType(scheme):
	return (scheme >> 15) & 1
def Algorithm(type, n):
	return n | ((type & 1) << 15)

#
# "compressionScheme" argument values
#
UNKNOWN_SCHEME = -1

UNCOMPRESSED_AUDIO = Algorithm(AUDIO, 0)
G711_ULAW = Algorithm(AUDIO, 1)
ULAW = Algorithm(AUDIO, 1)
G711_ALAW = Algorithm(AUDIO, 2)
ALAW = Algorithm(AUDIO, 2)
AWARE_MPEG_AUDIO = Algorithm(AUDIO, 3)
AWARE_MULTIRATE = Algorithm(AUDIO, 4)
    
UNCOMPRESSED = Algorithm(VIDEO, 0)
UNCOMPRESSED_VIDEO = Algorithm(VIDEO, 0)
RLE = Algorithm(VIDEO, 1)
JPEG = Algorithm(VIDEO, 2)
MPEG_VIDEO = Algorithm(VIDEO, 3)
MVC1 = Algorithm(VIDEO, 4)
RTR = Algorithm(VIDEO, 5)
RTR1 = Algorithm(VIDEO, 5)

#
# Parameters
#
MAX_NUMBER_OF_PARAMS = 256
# Default Parameters
IMAGE_WIDTH = 0
IMAGE_HEIGHT = 1 
ORIGINAL_FORMAT = 2
INTERNAL_FORMAT = 3
COMPONENTS = 4
BITS_PER_COMPONENT = 5
FRAME_RATE = 6
COMPRESSION_RATIO = 7
EXACT_COMPRESSION_RATIO = 8
FRAME_BUFFER_SIZE = 9 
COMPRESSED_BUFFER_SIZE = 10
BLOCK_SIZE = 11
PREROLL = 12
FRAME_TYPE = 13
ALGORITHM_ID = 14
ALGORITHM_VERSION = 15
ORIENTATION = 16
NUMBER_OF_FRAMES = 17
SPEED = 18
LAST_FRAME_INDEX = 19
NUMBER_OF_PARAMS = 20

# JPEG Specific Parameters
QUALITY_FACTOR = NUMBER_OF_PARAMS + 0

# MPEG Specific Parameters
END_OF_SEQUENCE = NUMBER_OF_PARAMS + 0

# RTR Specific Parameters
QUALITY_LEVEL = NUMBER_OF_PARAMS + 0
ZOOM_X = NUMBER_OF_PARAMS + 1
ZOOM_Y = NUMBER_OF_PARAMS + 2

#
# Parameter value types
#
ENUM_VALUE = 0				# only certain constant values are valid
RANGE_VALUE = 1				# any value in a given range is valid
FLOATING_ENUM_VALUE = 2			# only certain constant floating point values are valid
FLOATING_RANGE_VALUE = 3		# any value in a given floating point range is valid

#
# Algorithm Functionality
#
DECOMPRESSOR = 1
COMPRESSOR = 2
CODEC = 3

#
# Buffer types
#
NONE = 0
FRAME = 1
DATA = 2

#
# Frame types
#
NONE = 0
KEYFRAME = 1
INTRA = 1
PREDICTED = 2
BIDIRECTIONAL = 3

#
# Orientations
#
TOP_DOWN = 0
BOTTOM_UP = 1

#
# SGI Proprietary Algorithm Header Start Code
#
HEADER_START_CODE = 0xc1C0DEC

#
# error codes
#

BAD_NO_BUFFERSPACE =  -2		# no space for internal buffers
BAD_PVBUFFER =  -3			# param/val buffer doesn't make sense
BAD_BUFFERLENGTH_NEG =  -4		# negative buffer length
BAD_BUFFERLENGTH_ODD =  -5		# odd length parameter/value buffer
BAD_PARAM =  -6				# invalid parameter
BAD_COMPRESSION_SCHEME =  -7		# compression scheme parameter invalid
BAD_COMPRESSOR_HANDLE =  -8		# compression handle parameter invalid
BAD_COMPRESSOR_HANDLE_POINTER = -9	# compression handle pointer invalid
BAD_BUFFER_HANDLE = -10			# buffer handle invalid
BAD_BUFFER_QUERY_SIZE = -11		# buffer query size too large
JPEG_ERROR = -12			# error from libjpeg
BAD_FRAME_SIZE = -13			# frame size invalid
PARAM_OUT_OF_RANGE = -14		# parameter out of range
ADDED_ALGORITHM_ERROR = -15		# added algorithm had a unique error
BAD_ALGORITHM_TYPE = -16		# bad algorithm type
BAD_ALGORITHM_NAME = -17		# bad algorithm name
BAD_BUFFERING = -18			# bad buffering calls
BUFFER_NOT_CREATED = -19		# buffer not created
BAD_BUFFER_EXISTS = -20			# buffer already created
BAD_INTERNAL_FORMAT = -21		# invalid internal format
BAD_BUFFER_POINTER = -22		# invalid buffer pointer
FRAME_BUFFER_SIZE_ZERO = -23		# frame buffer has zero size
BAD_STREAM_HEADER = -24			# invalid stream header

BAD_LICENSE = -25			# netls license not valid
AWARE_ERROR = -26			# error from libawcmp
