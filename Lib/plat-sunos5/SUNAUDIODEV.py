# Symbolic constants for use with sunaudiodev module
# The names are the same as in audioio.h with the leading AUDIO_
# removed.

# Not all values are supported on all releases of SunOS.

# Encoding types, for fields i_encoding and o_encoding

ENCODING_NONE = 0                       # no encoding assigned
ENCODING_ULAW = 1                       # u-law encoding
ENCODING_ALAW = 2                       # A-law encoding
ENCODING_LINEAR = 3                     # Linear PCM encoding

# Gain ranges for i_gain, o_gain and monitor_gain

MIN_GAIN = 0                            # minimum gain value
MAX_GAIN = 255                          # maximum gain value

# Balance values for i_balance and o_balance

LEFT_BALANCE = 0                        # left channel only
MID_BALANCE = 32                        # equal left/right channel
RIGHT_BALANCE = 64                      # right channel only
BALANCE_SHIFT = 3

# Port names for i_port and o_port

PORT_A = 1
PORT_B = 2
PORT_C = 3
PORT_D = 4

SPEAKER = 0x01                          # output to built-in speaker
HEADPHONE = 0x02                        # output to headphone jack
LINE_OUT = 0x04                         # output to line out

MICROPHONE = 0x01                       # input from microphone
LINE_IN = 0x02                          # input from line in
CD = 0x04                               # input from on-board CD inputs
INTERNAL_CD_IN = CD                     # input from internal CDROM
