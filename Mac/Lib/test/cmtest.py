"""cmtest - List all components in the system"""

import Cm
import Res
import sys

def getstr255(r):
	"""Get string from str255 resource"""
	if not r.data: return ''
	len = ord(r.data[0])
	return r.data[1:1+len]

def getinfo(c):
	"""Return (type, subtype, creator, fl1, fl2, name, description) for component"""
	h1 = Res.Resource('')
	h2 = Res.Resource('')
	h3 = Res.Resource('')
	type, subtype, creator, fl1, fl2 = c.GetComponentInfo(h1, h2, h3)
	name = getstr255(h1)
	description = getstr255(h2)
	return type, subtype, creator, fl1, fl2, name, description
	
def getallcomponents():
	"""Return list with info for all components, sorted"""
	any = ('\0\0\0\0', '\0\0\0\0', '\0\0\0\0', 0, 0)
	c = None
	rv = []
	while 1:
		try:
			c = Cm.FindNextComponent(c, any)
		except Cm.Error:
			break
		rv.append(getinfo(c))
	rv.sort()
	return rv
	
def main():
	"""Print info for all components"""
	info = getallcomponents()
	for type, subtype, creator, f1, f2, name, description in info:
		print '%4.4s %4.4s %4.4s %s 0x%x 0x%x'%(type, subtype, creator, name, f1, f2)
		print '              ', description
	sys.exit(1)

main()
