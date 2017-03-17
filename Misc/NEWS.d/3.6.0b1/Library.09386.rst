Issue #25761: Improved error reporting about truncated pickle data in
C implementation of unpickler.  UnpicklingError is now raised instead of
AttributeError and ValueError in some cases.