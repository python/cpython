# Test program

def foo(arg1, arg2):
	bar(arg1+arg2)
	bar(arg1-arg2)
	foo(arg1+1, arg2-1)
	
def bar(arg):
	rv = 10/arg
	print rv
	
foo(0,10)

