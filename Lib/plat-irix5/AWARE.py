# 
# Copyright (c) 1992 Aware, Inc. All rights reserved.
# 
# Copyright Unpublished, Aware Inc.  All Rights Reserved.
# This software contains proprietary and confidential
# information of Aware, Inc.  Use, disclosure or
# reproduction is prohibited without the prior express
# written consent of Aware, Inc.
# 
#

#
# awareAudio.h - Aware Compression Library Parameter Header
#
#   01/21/92	Original Version by Brian Knittel and Jonathon Devine
#

import CL

# Aware Audio Specific Parameters - For both MPEG Audio and Multirate
CL.CHANNEL_POLICY = CL.NUMBER_OF_PARAMS + 0
CL.NOISE_MARGIN = CL.NUMBER_OF_PARAMS + 1
CL.BITRATE_POLICY = CL.NUMBER_OF_PARAMS + 2

# Additional parameters for MPEG Audio
CL.BITRATE_TARGET = CL.NUMBER_OF_PARAMS + 3
CL.LAYER = CL.NUMBER_OF_PARAMS + 4

# read/write for compression configuration
# read for state during compression/decompression 

#
# Channel Policy
#
AWCMP_STEREO = 1
AWCMP_JOINT_STEREO = 2
AWCMP_INDEPENDENT = 3

# 
# read/write for compression configuration,
# read for state during compression
#
#
# Bit-rate Policy
#
AWCMP_FIXED_RATE = 1
AWCMP_CONST_QUAL = 2
AWCMP_LOSSLESS = 4

#
# Layer values
#
AWCMP_MPEG_LAYER_I = 1
AWCMP_MPEG_LAYER_II = 2
