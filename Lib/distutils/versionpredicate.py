"""Module for parsing and testing package version predicate strings.
"""
import re
import version
import operator

re_validPackage = re.compile(r"(?i)^\s*([a-z_]\w*(?:\.[a-z_]\w*)*)(.*)")
# (package) (rest)

re_paren = re.compile(r"^\s*\((.*)\)\s*$") # (list) inside of parentheses
re_splitComparison = re.compile(r"^\s*(<=|>=|<|>|!=|==)\s*([^\s,]+)\s*$")
# (comp) (version)

def splitUp(pred):
   """Parse a single version comparison.
   Return (comparison string, StrictVersion)
   """
   res = re_splitComparison.match(pred)
   if not res:
       raise ValueError, "Bad package restriction syntax:  " + pred
   comp, verStr = res.groups()
   return (comp, version.StrictVersion(verStr))

compmap = {"<": operator.lt, "<=": operator.le, "==": operator.eq,
           ">": operator.gt, ">=": operator.ge, "!=": operator.ne}

class VersionPredicate:
   """Parse and test package version predicates.

   >>> v = VersionPredicate("pyepat.abc (>1.0, <3333.3a1, !=1555.1b3)")
   >>> print v
   pyepat.abc (> 1.0, < 3333.3a1, != 1555.1b3)
   >>> v.satisfied_by("1.1")
   True
   >>> v.satisfied_by("1.4")
   True
   >>> v.satisfied_by("1.0")
   False
   >>> v.satisfied_by("4444.4")
   False
   >>> v.satisfied_by("1555.1b3")
   False
   >>> v = VersionPredicate("pat( ==  0.1  )  ")
   >>> v.satisfied_by("0.1")
   True
   >>> v.satisfied_by("0.2")
   False
   >>> v = VersionPredicate("p1.p2.p3.p4(>=1.0, <=1.3a1, !=1.2zb3)")
   Traceback (most recent call last):
   ...
   ValueError: invalid version number '1.2zb3'

   """

   def __init__(self, versionPredicateStr):
       """Parse a version predicate string.
       """
       # Fields:
       #    name:  package name
       #    pred:  list of (comparison string, StrictVersion)

       versionPredicateStr = versionPredicateStr.strip()
       if not versionPredicateStr:
           raise ValueError, "Empty package restriction"
       match = re_validPackage.match(versionPredicateStr)
       if not match:
           raise ValueError, "Bad package name in " + versionPredicateStr
       self.name, paren = match.groups()
       paren = paren.strip()
       if paren:
           match = re_paren.match(paren)
           if not match:
               raise ValueError, "Expected parenthesized list: " + paren
           str = match.groups()[0]
           self.pred = [splitUp(aPred) for aPred in str.split(",")]
           if not self.pred:
               raise ValueError("Empty Parenthesized list in %r" 
                                % versionPredicateStr )
       else:
           self.pred=[]

   def __str__(self):
       if self.pred:
           seq = [cond + " " + str(ver) for cond, ver in self.pred]
           return self.name + " (" + ", ".join(seq) + ")"
       else:
           return self.name

   def satisfied_by(self, version):
       """True if version is compatible with all the predicates in self.
          The parameter version must be acceptable to the StrictVersion
          constructor.  It may be either a string or StrictVersion.
       """
       for cond, ver in self.pred:
           if not compmap[cond](version, ver):
               return False
       return True


def check_provision(value):
    m = re.match("[a-zA-Z_]\w*(\.[a-zA-Z_]\w*)*(\s*\([^)]+\))?$", value)
    if not m:
        raise ValueError("illegal provides specification: %r" % value)
