"""Hook to allow easy access to site-specific modules.

Scripts or modules that need to use site-specific modules should place

	import site

somewhere near the top of their code.  This will append up to two
site-specific paths ($prefix/lib/site-python and
$exec_prefix/lib/site-python) to the module search path.  ($prefix
and $exec_prefix are configuration parameters, and both default
to /usr/local; they are accessible in Python as sys.prefix and
sys.exec_prefix).

Because of Python's import semantics, it is okay for more than one
module to import site -- only the first one will execute the site
customizations.  The directories are only appended to the path if they
exist and are not already on it.

Sites that wish to provide site-specific modules should place them in
one of the site specific directories; $prefix/lib/site-python is for
Python source code and $exec_prefix/lib/site-python is for dynamically
loadable extension modules (shared libraries).

After these path manipulations, an attempt is made to import a module
named sitecustomize, which can perform arbitrary site-specific
customizations.  If this import fails with an ImportError exception,
it is ignored.

Note that for non-Unix systems, sys.prefix and sys.exec_prefix are
empty, and the path manipulations are skipped; however the import of
sitecustomize is still attempted.

XXX Any suggestions as to how to handle this for non-Unix systems???
"""

import sys, os

for prefix in sys.prefix, sys.exec_prefix:
    if prefix:
	sitedir = os.path.join(prefix, os.path.join("lib", "site-python"))
	if sitedir not in sys.path and os.path.isdir(sitedir):
	    sys.path.append(sitedir)	# Add path component

try:
    import sitecustomize		# Run arbitrary site specific code
except ImportError:
    pass				# No site customization module
