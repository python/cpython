# Constants defined in <gl.h>

#**************************************************************************
#*									  *
#* 		 Copyright (C) 1984, Silicon Graphics, Inc.		  *
#*									  *
#*  These coded instructions, statements, and computer programs  contain  *
#*  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
#*  are protected by Federal copyright law.  They  may  not be disclosed  *
#*  to  third  parties  or copied or duplicated in any form, in whole or  *
#*  in part, without the prior written consent of Silicon Graphics, Inc.  *
#*									  *
#**************************************************************************

# Graphics Libary constants

# Booleans
TRUE = 1
FALSE = 0

# maximum X and Y screen coordinates 
XMAXSCREEN = 1279
YMAXSCREEN = 1023
XMAXMEDIUM = 1023		# max for medium res monitor 
YMAXMEDIUM = 767
XMAX170 = 645		# max for RS-170 
YMAX170 = 484
XMAXPAL = 779		# max for PAL 
YMAXPAL = 574

# various hardware/software limits 
ATTRIBSTACKDEPTH = 10
VPSTACKDEPTH = 8
MATRIXSTACKDEPTH = 32
NAMESTACKDEPTH = 1025
STARTTAG = -2
ENDTAG = -3
CPOSX_INVALID = -(2*XMAXSCREEN)

# names for colors in color map loaded by greset 
BLACK = 0
RED = 1
GREEN = 2
YELLOW = 3
BLUE = 4
MAGENTA = 5
CYAN = 6
WHITE = 7

# popup colors 
PUP_CLEAR = 0
PUP_COLOR = 1
PUP_BLACK = 2
PUP_WHITE = 3

# defines for drawmode 
NORMALDRAW = 0
PUPDRAW = 1
OVERDRAW = 2
UNDERDRAW = 3
CURSORDRAW = 4

# defines for defpattern 
PATTERN_16 = 16
PATTERN_32 = 32
PATTERN_64 = 64

PATTERN_16_SIZE = 16
PATTERN_32_SIZE = 64
PATTERN_64_SIZE = 256

# defines for readsource 
SRC_AUTO = 0
SRC_FRONT = 1
SRC_BACK = 2
SRC_ZBUFFER = 3
SRC_PUP = 4
SRC_OVER = 5
SRC_UNDER = 6
SRC_FRAMEGRABBER = 7

# defines for blendfunction 
BF_ZERO = 0
BF_ONE = 1
BF_DC = 2
BF_SC = 2
BF_MDC = 3
BF_MSC = 3
BF_SA = 4
BF_MSA = 5
BF_DA = 6
BF_MDA = 7

# defines for zfunction 
ZF_NEVER = 0
ZF_LESS = 1
ZF_EQUAL = 2
ZF_LEQUAL = 3
ZF_GREATER = 4
ZF_NOTEQUAL = 5
ZF_GEQUAL = 6
ZF_ALWAYS = 7

# defines for zsource 
ZSRC_DEPTH = 0
ZSRC_COLOR = 1

# defines for pntsmooth 
SMP_OFF = 0
SMP_ON = 1

# defines for linesmooth 
SML_OFF = 0
SML_ON = 1

# defines for setpup 
PUP_NONE = 0
PUP_GREY = 1

# defines for glcompat 
GLC_OLDPOLYGON = 0
GLC_ZRANGEMAP = 1

# defines for curstype 
C16X1 = 0
C16X2 = 1
C32X1 = 2
C32X2 = 3
CCROSS = 4

# defines for shademodel 
FLAT = 0
GOURAUD = 1

# defines for logicop 
### LO_ZERO = 0x0
### LO_AND = 0x1
### LO_ANDR = 0x2
### LO_SRC = 0x3
### LO_ANDI = 0x4
### LO_DST = 0x5
### LO_XOR = 0x6
### LO_OR = 0x7
### LO_NOR = 0x8
### LO_XNOR = 0x9
### LO_NDST = 0xa
### LO_ORR = 0xb
### LO_NSRC = 0xc
### LO_ORI = 0xd
### LO_NAND = 0xe
### LO_ONE = 0xf


#
# START defines for getgdesc 
#

GD_XPMAX = 0
GD_YPMAX = 1
GD_XMMAX = 2
GD_YMMAX = 3
GD_ZMIN = 4
GD_ZMAX = 5
GD_BITS_NORM_SNG_RED = 6
GD_BITS_NORM_SNG_GREEN = 7
GD_BITS_NORM_SNG_BLUE = 8
GD_BITS_NORM_DBL_RED = 9
GD_BITS_NORM_DBL_GREEN = 10
GD_BITS_NORM_DBL_BLUE = 11
GD_BITS_NORM_SNG_CMODE = 12
GD_BITS_NORM_DBL_CMODE = 13
GD_BITS_NORM_SNG_MMAP = 14
GD_BITS_NORM_DBL_MMAP = 15
GD_BITS_NORM_ZBUFFER = 16
GD_BITS_OVER_SNG_CMODE = 17
GD_BITS_UNDR_SNG_CMODE = 18
GD_BITS_PUP_SNG_CMODE = 19
GD_BITS_NORM_SNG_ALPHA = 21 
GD_BITS_NORM_DBL_ALPHA = 22
GD_BITS_CURSOR = 23
GD_OVERUNDER_SHARED = 24
GD_BLEND = 25
GD_CIFRACT = 26
GD_CROSSHAIR_CINDEX = 27
GD_DITHER = 28
GD_LINESMOOTH_CMODE = 30
GD_LINESMOOTH_RGB = 31
GD_LOGICOP = 33
GD_NSCRNS = 35
GD_NURBS_ORDER = 36
GD_NBLINKS = 37
GD_NVERTEX_POLY = 39
GD_PATSIZE_64 = 40
GD_PNTSMOOTH_CMODE = 41
GD_PNTSMOOTH_RGB = 42
GD_PUP_TO_OVERUNDER = 43
GD_READSOURCE = 44
GD_READSOURCE_ZBUFFER = 48
GD_STEREO = 50
GD_SUBPIXEL_LINE = 51
GD_SUBPIXEL_PNT = 52
GD_SUBPIXEL_POLY = 53
GD_TRIMCURVE_ORDER = 54
GD_WSYS = 55
GD_ZDRAW_GEOM = 57
GD_ZDRAW_PIXELS = 58
GD_SCRNTYPE = 61
GD_TEXTPORT = 62
GD_NMMAPS = 63
GD_FRAMEGRABBER = 64
GD_TIMERHZ = 66
GD_DBBOX = 67
GD_AFUNCTION = 68
GD_ALPHA_OVERUNDER = 69
GD_BITS_ACBUF = 70
GD_BITS_ACBUF_HW = 71
GD_BITS_STENCIL = 72
GD_CLIPPLANES = 73
GD_FOGVERTEX = 74
GD_LIGHTING_TWOSIDE = 76
GD_POLYMODE = 77
GD_POLYSMOOTH = 78
GD_SCRBOX = 79
GD_TEXTURE = 80

# return value for inquiries when there is no limit
GD_NOLIMIT = 2

# return values for GD_WSYS
GD_WSYS_NONE = 0
GD_WSYS_4S = 1

# return values for GD_SCRNTYPE
GD_SCRNTYPE_WM = 0
GD_SCRNTYPE_NOWM = 1

# 
# END defines for getgdesc 
#


# 
# START NURBS interface definitions 
#

# NURBS Rendering Properties 
N_PIXEL_TOLERANCE = 1
N_CULLING = 2
N_DISPLAY = 3
N_ERRORCHECKING = 4
N_SUBDIVISIONS = 5
N_S_STEPS = 6
N_T_STEPS = 7
N_TILES = 8

N_SHADED = 1.0 	

# ---------------------------------------------------------------------------
# FLAGS FOR NURBS SURFACES AND CURVES 			
# 
# Bit: 9876 5432 1 0 
#     |tttt|nnnn|f|r| :    r - 1 bit = 1 if rational coordinate exists
# 	               :    f - 1 bit = 1 if rational coordinate is before rest 
# 	               :              = 0 if rational coordinate is after rest 
# 	 	       : nnnn - 4 bits for number of coordinates
# 		       : tttt - 4 bits for type of data (color, position, etc.)
# 
# NURBS data type
# N_T_ST	 	0	 parametric space data
# N_T_XYZ		1	 model space data
# 
# rational or non-rational data and position in memory 
# N_NONRATIONAL	0	 non-rational data
# N_RATAFTER		1	 rational data with rat coord after rest
# N_RATBEFORE		3	 rational data with rat coord before rest
# 
# N_MKFLAG(a,b,c) ((a<<6) | (b<<2) | c)
# 	
# ---------------------------------------------------------------------------
# 
N_ST = 0x8	# N_MKFLAG( N_T_ST, 2, N_NONRATIONAL ) 
N_STW = 0xd	# N_MKFLAG( N_T_ST, 3, N_RATAFTER ) 
N_WST = 0xf	# N_MKFLAG( N_T_ST, 3, N_RATBEFORE ) 
N_XYZ = 0x4c	# N_MKFLAG( N_T_XYZ, 3, N_NONRATIONAL ) 
N_XYZW = 0x51	# N_MKFLAG( N_T_XYZ, 4, N_RATAFTER ) 
N_WXYZ = 0x53	# N_MKFLAG( N_T_XYZ, 4, N_RATBEFORE ) 

# 
# END NURBS interface definitions 
# 


# 
# START lighting model defines 
# 

LMNULL = 0.0

# MATRIX modes	
MSINGLE = 0
MPROJECTION = 1
MVIEWING = 2

# LIGHT constants 
MAXLIGHTS = 8
MAXRESTRICTIONS = 4

# MATERIAL properties 
DEFMATERIAL = 0
EMISSION = 1
AMBIENT = 2
DIFFUSE = 3
SPECULAR = 4
SHININESS = 5
COLORINDEXES = 6
ALPHA = 7

# LIGHT properties 
DEFLIGHT = 100
LCOLOR = 101
POSITION = 102

# LIGHTINGMODEL properties 
DEFLMODEL = 200
LOCALVIEWER = 201
ATTENUATION = 202

# TARGET constants 
MATERIAL = 1000
LIGHT0 = 1100
LIGHT1 = 1101
LIGHT2 = 1102
LIGHT3 = 1103
LIGHT4 = 1104
LIGHT5 = 1105
LIGHT6 = 1106
LIGHT7 = 1107
LMODEL = 1200

# lmcolor modes 
LMC_COLOR = 0
LMC_EMISSION = 1
LMC_AMBIENT = 2
LMC_DIFFUSE = 3
LMC_SPECULAR = 4
LMC_AD = 5
LMC_NULL = 6

# 
# END lighting model defines 
# 


# 
# START distributed graphics library defines 
# 

DGLSINK = 0	# sink connection	
DGLLOCAL = 1	# local connection	
DGLTSOCKET = 2	# tcp socket connection
DGL4DDN = 3	# 4DDN (DECnet)	

# 
# END distributed graphics library defines 
# 
