Issue #24454: Regular expression match object groups are now
accessible using __getitem__.  "mo[x]" is equivalent to
"mo.group(x)".