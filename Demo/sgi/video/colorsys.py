#
# Module color - do color conversions
#

def rgb_to_yiq(r,g,b):
    y = 0.30*r + 0.59*g + 0.11*b
    i = 0.60*r - 0.28*g - 0.32*b
    q = 0.21*r - 0.52*g + 0.31*b
    return (y,i,q)
def yiq_to_rgb(y,i,q):
    r = y + 0.948262*i + 0.624013*q
    g = y - 0.276066*i - 0.639810*q
    b = y - 1.105450*i + 1.729860*q
    if r < 0.0: r = 0.0
    if g < 0.0: g = 0.0
    if b < 0.0: b = 0.0
    if r > 1.0: r = 1.0
    if g > 1.0: g = 1.0
    if b > 1.0: b = 1.0
    return (r,g,b)
