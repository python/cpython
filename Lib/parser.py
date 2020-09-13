"""parser.py - The convert library"""

from Lib.pygb import *

# Returns int from value.
def parseInt(val):
    return int(val)
    pass

# Returns int if parse is success, returns None if not.
def tryParseInt(val):
    try:
        return int(val)
        pass
    except:
        return None
        pass
    pass

# Returns float from value.
def parseFloat(val):
    return float(val)
    pass

# Returns float if parse is success, returns None if not.
def tryParseFloat(val):
    try:
        return float(val)
        pass
    except:
        return None
        pass
    pass

# Parse color from strcode.
def parseColor(strcode = "255"):
    strcode = "255" if strcode is None or strcode == "" else strcode
    parts = strcode.split(",")
    count = parts.__len__()

    color = Color()
    if(count > 0):
        A = tryParseInt(parts[0])
        A = MAXRGB if A is None else A
        color.A = A
        pass
    if(count > 1):
        R = tryParseInt(parts[1])
        R = MINRGB if R is None else R
        color.R = R
        pass
    if(count > 2):
        G = tryParseInt(parts[2])
        G = MINRGB if G is None else G
        color.G = G
        pass
    if(count > 3):
        B = tryParseInt(parts[3])
        B = MINRGB if B is None else B
        color.B = B
        pass
    return color
    pass

# Returns as shell color code by color object.
def toSHCCode(color):
    if isinstance(color, Color) == False:
        raise TypeError("This type is not 'Color'!")
    return f'\033[38;2;{color.R};{color.G};{color.B}m'
    pass
