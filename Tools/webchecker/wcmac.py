import webchecker, sys
webchecker.DEFROOT = "http://www.python.org/python/"
webchecker.MAXPAGE = 50000
webchecker.verbose = 2
sys.argv.append('-x')
webchecker.main()
raw_input("\nCR to exit: ")
