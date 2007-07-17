import time
import sys
import FSProxy

def main():
    t1 = time.time()
    #proxy = FSProxy.FSProxyClient(('voorn.cwi.nl', 4127))
    proxy = FSProxy.FSProxyLocal()
    sumtree(proxy)
    proxy._close()
    t2 = time.time()
    print(t2-t1, "seconds")
    sys.stdout.write("[Return to exit] ")
    sys.stdout.flush()
    sys.stdin.readline()

def sumtree(proxy):
    print("PWD =", proxy.pwd())
    files = proxy.listfiles()
    proxy.infolist(files)
    subdirs = proxy.listsubdirs()
    for name in subdirs:
        proxy.cd(name)
        sumtree(proxy)
        proxy.back()

main()
