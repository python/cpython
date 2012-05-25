"""This will be the home for the policy that hooks in the new
code that adds all the email6 features.
"""

from email._policybase import Policy, compat32, Compat32

# XXX: temporarily derive everything from compat32.

default = compat32
strict = default.clone(raise_on_defect=True)
SMTP = default.clone(linesep='\r\n')
HTTP = default.clone(linesep='\r\n', max_line_length=None)
