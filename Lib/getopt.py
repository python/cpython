# module getopt -- Standard command line processing.

# Function getopt.getopt() has a different interface but provides the
# same functionality as the Unix getopt() function.

# It has two arguments: the first should be argv[1:] (it doesn't want
# the script name), the second the string of option letters as passed
# to Unix getopt() (i.e., a string of allowable option letters, with
# options requiring an argument followed by a colon).

# It raises the exception getopt.error with a string argument if it
# detects an error.

# It returns two items:
# (1)	a list of pairs (option, option_argument) giving the options in
#	the order in which they were specified.  (I'd use a dictionary
#	but applications may depend on option order or multiple
#	occurrences.)  Boolean options have '' as option_argument.
# (2)	the list of remaining arguments (may be empty).

error = 'getopt error'

def getopt(args, options):
    list = []
    while args and args[0][0] = '-' and args[0] <> '-':
    	if args[0] = '--':
    	    args = args[1:]
    	    break
    	optstring, args = args[0][1:], args[1:]
    	while optstring <> '':
    	    opt, optstring = optstring[0], optstring[1:]
    	    if classify(opt, options): # May raise exception as well
    	    	if optstring = '':
    	    	    if not args:
    	    	    	raise error, 'option -' + opt + ' requires argument'
    	    	    optstring, args = args[0], args[1:]
    	    	optarg, optstring = optstring, ''
    	    else:
    	    	optarg = ''
    	    list.append('-' + opt, optarg)
    return list, args

def classify(opt, options): # Helper to check type of option
    for i in range(len(options)):
	if opt = options[i] <> ':':
	    return options[i+1:i+2] = ':'
    raise error, 'option -' + opt + ' not recognized'
