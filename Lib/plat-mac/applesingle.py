# applesingle - a module to decode AppleSingle files
import struct
import MacOS
import sys

Error="applesingle.Error"

verbose=0

# File header format: magic, version, unused, number of entries
AS_HEADER_FORMAT="ll16sh"
AS_HEADER_LENGTH=26
# The flag words for AppleSingle
AS_MAGIC=0x00051600
AS_VERSION=0x00020000

# Entry header format: id, offset, length
AS_ENTRY_FORMAT="lll"
AS_ENTRY_LENGTH=12

# The id values
AS_DATAFORK=1
AS_RESOURCEFORK=2
AS_IGNORE=(3,4,5,6,8,9,10,11,12,13,14,15)

def decode(input, output, resonly=0):
	if type(input) == type(''):
		input = open(input, 'rb')
	# Should we also test for FSSpecs or FSRefs?
	header = input.read(AS_HEADER_LENGTH)
	try:
		magic, version, dummy, nentry = struct.unpack(AS_HEADER_FORMAT, header)
	except ValueError, arg:
		raise Error, "Unpack header error: %s"%arg
	if verbose:
		print 'Magic:   0x%8.8x'%magic
		print 'Version: 0x%8.8x'%version
		print 'Entries: %d'%nentry
	if magic != AS_MAGIC:
		raise Error, 'Unknown AppleSingle magic number 0x%8.8x'%magic
	if version != AS_VERSION:
		raise Error, 'Unknown AppleSingle version number 0x%8.8x'%version
	if nentry <= 0:
		raise Error, "AppleSingle file contains no forks"
	headers = [input.read(AS_ENTRY_LENGTH) for i in range(nentry)]
	didwork = 0
	for hdr in headers:
		try:
			id, offset, length = struct.unpack(AS_ENTRY_FORMAT, hdr)
		except ValueError, arg:
			raise Error, "Unpack entry error: %s"%arg
		if verbose:
			print 'Fork %d, offset %d, length %d'%(id, offset, length)
		input.seek(offset)
		if length == 0:
			data = ''
		else:
			data = input.read(length)
		if len(data) != length:
			raise Error, 'Short read: expected %d bytes got %d'%(length, len(data))
		if id == AS_DATAFORK:
			if verbose:
				print '  (data fork)'
			if not resonly:
				didwork = 1
				fp = open(output, 'wb')
				fp.write(data)
				fp.close()
		elif id == AS_RESOURCEFORK:
			didwork = 1
			if verbose:
				print '  (resource fork)'
			if resonly:
				fp = open(output, 'wb')
			else:
				fp = MacOS.openrf(output, 'wb')
			fp.write(data)
			fp.close()
		elif id in AS_IGNORE:
			if verbose:
				print '  (ignored)'
		else:
			raise Error, 'Unknown fork type %d'%id
	if not didwork:
		raise Error, 'No useful forks found'

def _test():
	if len(sys.argv) < 3 or sys.argv[1] == '-r' and len(sys.argv) != 4:
		print 'Usage: applesingle.py [-r] applesinglefile decodedfile'
		sys.exit(1)
	if sys.argv[1] == '-r':
		resonly = 1
		del sys.argv[1]
	else:
		resonly = 0
	decode(sys.argv[1], sys.argv[2], resonly=resonly)
	
if __name__ == '__main__':
	_test()
	