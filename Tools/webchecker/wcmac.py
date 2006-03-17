import webchecker, sys
webchecker.DEFROOT = "http://www.python.org/python/"
webchecker.MAXPAGE = 50000
webchecker.verbose = 2
sys.argv.append('-x')
webchecker.main()
sys.stdout.write("\nCR to exit: ")
sys.stdout.flush()
sys.stdin.readline()
