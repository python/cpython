Issue #26198: ValueError is now raised instead of TypeError on buffer
overflow in parsing "es#" and "et#" format units.  SystemError is now raised
instead of TypeError on programmical error in parsing format string.