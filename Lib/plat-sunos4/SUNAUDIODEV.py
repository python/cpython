# Symbolic constants for use with sunaudiodev module
# The names are the same as in audioio.h with the leading AUDIO_
# removed.

# Encoding types, for fields i_encoding and o_encoding

ENCODING_ULAW = 1
ENCODING_ALAW = 2

# Gain ranges for i_gain, o_gain and monitor_gain

MIN_GAIN = 0
MAX_GAIN = 255

# Port names for i_port and o_port

PORT_A = 1
PORT_B = 2
PORT_C = 3
PORT_D = 4

SPEAKER = PORT_A
HEADPHONE = PORT_B

MICROPHONE = PORT_A
