#
# Module color - do color conversions
#

ONE_THIRD=1.0/3.0
ONE_SIXTH=1.0/6.0
TWO_THIRD=2.0/3.0

def rgb_to_yiq(r,g,b):
    y = 0.3*r + 0.59*g + 0.11*b
    i = 0.6*r - 0.28*g - 0.32*b
    q = 0.21*r- 0.52*g + 0.31*b
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

def _v(m1,m2,hue):
    if hue >= 1.0: hue = hue - 1.0
    if hue <  0.0: hue = hue + 1.0
    if hue < ONE_SIXTH:
	return m1 + (m2-m1)*hue*6.0
    if hue < 0.5:
	return m2
    if hue < TWO_THIRD:
	return m1 + (m2-m1)*(TWO_THIRD-hue)*6.0
    return m1

def rgb_to_hls(r,g,b):
    maxc = max(r,g,b)
    minc = min(r,g,b)
    l = (minc+maxc)/2.0
    if minc == maxc:
	return 0.0, l, 0.0
    if l <= 0.5:
	s = (maxc-minc)/(maxc+minc)
    else:
	s = (maxc-minc)/(2-maxc-minc)
    rc = (maxc-r)/(maxc-minc)
    gc = (maxc-g)/(maxc-minc)
    bc = (maxc-b)/(maxc-minc)
    if r == maxc:
	h = bc-gc
    elif g == maxc:
	h = 2.0+rc-bc
    else:
	h = 4.0+gc-rc
    h = h/6.0
    if h < 0.0:
	h = h + 1.0
    return h,l,s
def hls_to_rgb(h,l,s):
    if s == 0.0:
	return l,l,l
    if l <= 0.5:
	m2 = l * (1.0+s)
    else:
	m2 = l+s-(l*s)
    m1 = 2.0*l - m2
    return (_v(m1,m2,h+ONE_THIRD), _v(m1,m2,h), _v(m1,m2,h-ONE_THIRD))

def rgb_to_hsv(r,g,b):
    maxc = max(r,g,b)
    minc = min(r,g,b)
    v = maxc
    if minc == maxc:
	return 0.0, 0.0, v
    s = (maxc-minc)/maxc
    rc = (maxc-r)/(maxc-minc)
    gc = (maxc-g)/(maxc-minc)
    bc = (maxc-b)/(maxc-minc)
    if r == maxc:
	h = bc-gc
    elif g == maxc:
	h = 2.0+rc-bc
    else:
	h = 4.0+gc-rc
    h = h/6.0
    if h < 0.0:
	h = h + 1.0
    return h,s,v
def hsv_to_rgb(h,s,v):
    if s == 0.0:
	return v,v,v
    i = int(h*6.0)
    f = (h*6.0)-i
    p = v*(1.0-s)
    q = v*(1.0-s*f)
    t = v*(1.0-s*(1.0-f))
    if i in (0,6): return v,t,p
    if i == 1: return q,v,p
    if i == 2: return p,v,t
    if i == 3: return p,q,v
    if i == 4: return t,p,v
    if i == 5: return v,p,q
    print i, h, f
    print h, s, v
    raise 'Bad color'
