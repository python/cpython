import Drag
import time
xxxx=1
def decode_hfs(data):
	import struct, macfs
	tp = data[0:4]
	cr = data[4:8]
	flags = struct.unpack("h", data[8:10])
	fss = macfs.RawFSSpec(data[10:])
	return tp, cr, flags, fss
	
def tracker(msg, dragref, window):
	pass
	
def dropper(dragref, window):
	global xxxx
	n = dragref.CountDragItems()
	print 'Drop %d items:'%n
	for i in range(1, n+1):
		refnum = dragref.GetDragItemReferenceNumber(i)
		print '%d (ItemReference 0x%x)'%(i, refnum)
		nf  = dragref.CountDragItemFlavors(refnum)
		print '    %d flavors:'%nf
		for j in range(1, nf+1):
			ftype = dragref.GetFlavorType(refnum, j)
			fflags = dragref.GetFlavorFlags(refnum, ftype)
			print '        "%4.4s" 0x%x'%(ftype, fflags)
			if ftype == 'hfs ':
				datasize = dragref.GetFlavorDataSize(refnum, ftype)
				data = dragref.GetFlavorData(refnum, ftype, datasize, 0)
				print '        datasize', `data`
				xxxx = data
				print '        ->', decode_hfs(data)
				
			
def main():
	print "Drag things onto output window. Press command-. to quit."
	Drag.InstallTrackingHandler(tracker)
	Drag.InstallReceiveHandler(dropper)
	while 1:
		time.sleep(100)
		
main()
