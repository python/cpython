import time
import FSProxy

def main():
	t1 = time.time()
	#proxy = FSProxy.FSProxyClient(('voorn.cwi.nl', 4127))
	proxy = FSProxy.FSProxyLocal()
	sumtree(proxy)
	proxy._close()
	t2 = time.time()
	print t2-t1, "seconds"
	raw_input("[Return to exit] ")

def sumtree(proxy):
	print "PWD =", proxy.pwd()
	files = proxy.listfiles()
	proxy.infolist(files)
	subdirs = proxy.listsubdirs()
	for name in subdirs:
		proxy.cd(name)
		sumtree(proxy)
		proxy.back()

main()
