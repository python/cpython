REPS = 8192

try:
    eval("2+2+" * REPS + "+3.14159265")
except SyntaxError, msg:
    print "Caught SyntaxError for long expression:", msg
else:
    print "Long expression did not raise SyntaxError"

## This test prints "s_push: parser stack overflow" on stderr,
    ## which seems to confuse the test harness
##try:
##    eval("(2+" * REPS + "0" + ")" * REPS)
##except SyntaxError:
##    pass
##else:
##    print "Deeply nested expression did not raised SyntaxError"

