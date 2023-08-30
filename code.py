try:
    while 1: 
        pass
except KeyboardInterrupt:
    print("Exception handling works!")

'''
# The single statement inside the try block (while 1: pass) is
 susceptible to being terminated by a KeyboardInterrupt. 
 Therefore, the code in the except block gets performed 
 whenever Ctrl+C is pressed since a KeyboardInterrupt exception is thrown.

'''


try:
    while 1: pass
except KeyboardInterrupt:
    print("Exception handling works!")

'''
Since there is no separate "pass" command, the while loop cannot be halted by a
 KeyboardInterrupt in this situation since the entire loop is inlined.
  The loop instead continues to iterate forever with no breakable functionality.
   This causes the KeyboardInterrupt exception to be thrown when Ctrl+C is pressed,
    but the interpreter to skip the except block and display the traceback instead.

Adding a meaningful operation, like a pass statement, inside the loop will accomplish
 the same thing in both scenarios, and will give the KeyboardInterrupt something to 
 interrupt:
'''
try:
    while 1: 
        pass  # or any other meaningful operation
except KeyboardInterrupt:
    print("Exception handling works!")


'''
This way, in both cases, the KeyboardInterrupt will be caught by the 
except block and the corresponding message will be printed.'''








