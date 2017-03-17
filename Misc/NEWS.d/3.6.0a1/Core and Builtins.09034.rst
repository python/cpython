Issue #24965: Implement PEP 498 "Literal String Interpolation". This
allows you to embed expressions inside f-strings, which are
converted to normal strings at run time. Given x=3, then
f'value={x}' == 'value=3'. Patch by Eric V. Smith.