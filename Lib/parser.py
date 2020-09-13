"""parse.py - The convert library"""

# Returns int from value.
def parseInt(val) -> int:
    return int(val)
    pass

# Returns int if parse is success, returns None if not.
def tryParseInt(val) -> object:
    try:
        return int(val)
        pass
    except:
        return None
        pass
    pass

# Returns float from value.
def parseFloat(val) -> float:
    return float(val)
    pass

# Returns float if parse is success, returns None if not.
def tryParseFloat(val) -> object:
    try:
        return float(val)
        pass
    except:
        return None
        pass
    pass
