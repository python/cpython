# Note:  this file must not be named pydoc.pyw, lest it just end up
# importing itself (Python began allowing import of .pyw files
# between 2.2a1 and 2.2a2).
import pydoc

if __name__ == '__main__':
   pydoc.gui()
