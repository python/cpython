# typedef enum CL_OriginalFormat
RGB = 0
RGBX = 1
RGBA = 2
YCrCb444 = 3
YCrCb422 = 4
YCrCb411 = 5
RGB332 = 6
COLORMAP8 = 7
COLORMAP12 = 8
GRAYSCALE = 9
MONO = 10
STEREO_INTERLEAVED = 11
QUAD_INTERLEAVED = 12
SURROUND_SOUND = 13

MAX_NUMBER_OF_AUDIO_ALGORITHMS = 32
MAX_NUMBER_OF_VIDEO_ALGORITHMS = 32

# typedef enum CL_CompressionScheme
UNCOMPRESSED = 0
G711_ULAW = 1
G711_ALAW = 2
G722 = 3
UNCOMPRESSED_VIDEO = MAX_NUMBER_OF_AUDIO_ALGORITHMS
RLE = UNCOMPRESSED_VIDEO + 1
JPEG = RLE + 1
MPEG_VIDEO = JPEG + 1
MVC1 = MPEG_VIDEO + 1


#
# Parameters
#
# typedef enum CL_Parameters
SPEED = 0
ACTUAL_FRAME_INDEX  = 1
COMPRESSION_FORMAT = 2
QUALITY_FACTOR = 3
NUMBER_OF_PARAMS = 4

MAX_NUMBER_OF_PARAMS = 32


#
# Parameter value types
#
# typedef enum CL_ParameterTypes
ENUM_VALUE = 0			# only certain constant values are valid
RANGE_VALUE = 1			# any value in a given range is valid
POINTER = 2			# any legal pointer is valid

# typedef enum AlgorithmType
AUDIO = 0
VIDEO = 1

# typedef enum AlgorithmFunctionality
DECOMPRESSOR = 1
COMPRESSOR = 2
CODEC = 3


#
# error codes
#
BAD_NOT_IMPLEMENTED = 0		# not impimented yet
BAD_NO_BUFFERSPACE = 1		# no space for internal buffers
BAD_QSIZE = 2			# attempt to set an invalid queue size
BAD_BUFFER_NULL = 3		# null buffer pointer
BAD_COUNT_NEG = 4		# negative count
BAD_PVBUFFER = 5		# param/val buffer doesn't make sense
BAD_BUFFERLENGTH_NEG = 6	# negative buffer length
BAD_BUFFERLENGTH_ODD = 7	# odd length parameter/value buffer
BAD_PARAM = 8			# invalid parameter
BAD_COMPRESSION_SCHEME = 9	# compression scheme parameter invalid
BAD_COMPRESSOR_HANDLE = 10	# compression handle parameter invalid
BAD_COMPRESSOR_HANDLE_POINTER = 11 # compression handle pointer invalid
BAD_CALLBACK_FUNCTION = 12	# callback function invalid
BAD_COMPRESSION_FORMAT_POINTER = 13 # compression format parameter invalid
BAD_POINTER_FROM_CALLBACK_FUNCTION = 14 # pointer from callback invalid
JPEG_ERROR = 15			# error from libjpeg
NO_SEMAPHORE = 16		# could not get semaphore
BAD_WIDTH_OR_HEIGHT = 17	# width or height invalid 
BAD_FRAME_COUNT = 18		# frame count invalid 
