"""cfmfile - Interface to code fragments on file"""
import struct
import Res
import macfs
import string

Error = 'cfmfile.Error'

READ = 1
WRITE = 2
smAllScripts = -3
BUFSIZE = 0x100000

class FragmentInfo:
	"""Information on a single fragment"""
	def __init__(self):
		self.arch = 'pwpc'
		self.current_version = 0
		self.oldest_version = 0
		self.stacksize = 0
		self.libdir = 0
		self.fragtype = 1
		self.location = 1
		self.offset = 0
		self.length = 0
		self.res_0 = 0
		self.res_1 = 0
		self.name = ''
		self.ifp = None
		
	def load(self, data):
		if len(data) < 43:
			raise Error, 'Not enough data in cfrg resource'
		self.arch = data[:4]
		self.update, self.current_version, self.oldest_version, \
			self.stacksize, self.libdir, self.fragtype, self.location, \
			self.offset, self.length, self.res_0, self.res_1, length = \
			struct.unpack("llllhbbllllh", data[4:42])
		namelen = ord(data[42])
		self.name = data[43:43+namelen]
		if len(self.name) != namelen:
			raise Error, 'Not enough data in cfrg resource'
		return length
		
	def save(self):
		length = (43+len(self.name)+3) & ~3
		data = self.arch + struct.pack("llllhbbllllh", self.update, \
			self.current_version, self.oldest_version, self.stacksize, \
			self.libdir, self.fragtype, self.location, self.offset, \
			self.length, self.res_0, self.res_1, length)
		data = data + chr(len(self.name)) + self.name
		data = data + ('\0'*(length-len(data)))
		return data
		
	def copydata(self, ofp):
		"""Copy fragment data to a new file, updating self.offset"""
		if self.location != 1:
			raise Error, 'Can only copy kOnDiskFlat (data fork) fragments'
		if not self.ifp:
			raise Error, 'No source file for fragment'
		# Find out real length (if zero)
		if self.length == 0:
			self.ifp.seek(0, 2)
			self.length = self.ifp.tell()
		# Position input file and record new offset from output file
		self.ifp.seek(self.offset)
		self.offset = ofp.tell()
		l = self.length
		while l:
			if l > BUFSIZE:
				ofp.write(self.ifp.read(BUFSIZE))
				l = l - BUFSIZE
			else:
				ofp.write(self.ifp.read(l))
				l = 0
		self.ifp = ofp
		
	def setfile(self, ifp):
		self.ifp = ifp
		
class FragmentResource:

	def __init__(self, data):
		self.load(data)

	def load(self, data):
		r0, r1, version, r3, r4, r5, r6, nfrag = struct.unpack("llllllll", data[:32])
		if version != 1:
			raise Error, 'Unsupported cfrg version number %d'%version
		data = data[32:]
		self.fragments = []
		for i in range(nfrag):
			f = FragmentInfo()
			len = f.load(data)
			data = data[len:]
			self.fragments.append(f)
		if data:
			raise Error, 'Spurious data after fragment descriptions'
			
	def save(self):
		data = struct.pack("llllllll", 0, 0, 1, 0, 0, 0, 0, len(self.fragments))
		for f in self.fragments:
			data = data+f.save()
		return data
			
	def setfile(self, ifp):
		for f in self.fragments:
			f.setfile(ifp)
			
	def copydata(self, ofp):
		for f in self.fragments:
			f.copydata(ofp)
			
	def getfragments(self):
		return self.fragments
		
	def addfragments(self, fragments):
		self.fragments = self.fragments + fragments

class ResourceCollection:
	def __init__(self, fhandle):
		self.reslist = []
		self.fhandle = fhandle
		oldresfile = Res.CurResFile()
		Res.UseResFile(fhandle)
		Res.SetResLoad(0)
		ntypes = Res.Count1Types()
		for itype in range(1, 1+ntypes):
			type = Res.Get1IndType(itype)
			nresources = Res.Count1Resources(type)
			for ires in range(1, 1+nresources):
				res = Res.Get1IndResource(type, ires)
				id, type, name = res.GetResInfo()
				self.reslist.append((type, id))
		Res.SetResLoad(1)
		Res.UseResFile(oldresfile)
			
	def contains(self, type, id):
		return (type, id) in self.reslist
		
	def getresource(self, type, id):
		oldresfile = Res.CurResFile()
		Res.UseResFile(self.fhandle)
		Res.SetResLoad(1)
		resource = Res.Get1Resource(type, id)
		Res.UseResFile(oldresfile)
		return resource
		
	def saveresto(self, type, id, fhandle):
		oldresfile = Res.CurResFile()
		resource = self.getresource(type, id)
		id, type, name = resource.GetResInfo()
		resource.DetachResource()
		Res.UseResFile(fhandle)
		resource.AddResource(type, id, name)
		Res.UseResFile(oldresfile)
		
	def getreslist(self):
		return self.reslist
		
class CfmFile(ResourceCollection, FragmentResource):
	
	def __init__(self, fsspec):
		rfork = Res.FSpOpenResFile(fsspec, READ)
		dfork = open(fsspec.as_pathname(), 'rb')
		ResourceCollection.__init__(self, rfork)
		cfrg_resource = self.getresource('cfrg', 0)
		FragmentResource.__init__(self, cfrg_resource.data)
		self.setfile(dfork)

def mergecfmfiles(inputs, output):
	# Convert inputs/outputs to fsspecs
	inputs = map(None, inputs)
	for i in range(len(inputs)):
		if type(inputs[i]) == type(''):
			inputs[i] = macfs.FSSpec(inputs[i])
	if type(output) == type(''):
		output = macfs.FSSpec(output)
		
	input_list = []
	for i in inputs:
		input_list.append(CfmFile(i))
		
	# Create output file, if needed
	creator, tp = inputs[0].GetCreatorType()
	try:
		Res.FSpCreateResFile(output, creator, tp, smAllScripts)
	except Res.Error:
		pass
		
	# Copy fragments
	dfork = open(output.as_pathname(), 'wb')
	for i in input_list:
		i.copydata(dfork)
	dfork.close()
		
	# Merge cfrg's
	for i in input_list[1:]:
		input_list[0].addfragments(i.getfragments())
		
	old_res_file = Res.CurResFile()
	rfork = Res.FSpOpenResFile(output, WRITE)
	Res.UseResFile(rfork)
	
	# Write cfrg
	data = input_list[0].save()
	cfrg_resource = Res.Resource(data)
	cfrg_resource.AddResource('cfrg', 0, '')
	resources_done = [('cfrg', 0)]
	
	# Write other resources
	for i in input_list:
		todo = i.getreslist()
		for tp, id in todo:
			if (tp, id) in resources_done:
				continue
			i.saveresto(tp, id, rfork)
			resources_done.append(tp, id)
			
def main():
	list = []
	while 1:
		fss, ok = macfs.PromptGetFile("Next input file:", "shlb", "APPL")
		if not ok: break
		list.append(fss)
	if not list:
		sys.exit(0)
	output, ok = macfs.StandardPutFile("Output file:")
	if not ok:
		sys.exit(0)
	mergecfmfiles(list, output)
	
if __name__ == '__main__':
	main()
	
			
