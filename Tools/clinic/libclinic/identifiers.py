import re
from .errors import ClinicError


is_legal_c_identifier = re.compile("^[A-Za-z_][A-Za-z0-9_]*$").match


def is_legal_py_identifier(identifier: str) -> bool:
    return all(is_legal_c_identifier(field) for field in identifier.split("."))


# Identifiers that are okay in Python but aren't a good idea in C.
# So if they're used Argument Clinic will add "_value" to the end
# of the name in C.
_c_keywords = frozenset("""
asm auto break case char const continue default do double
else enum extern float for goto if inline int long
register return short signed sizeof static struct switch
typedef typeof union unsigned void volatile while
""".strip().split()
)


def ensure_legal_c_identifier(identifier: str) -> str:
    # For now, just complain if what we're given isn't legal.
    if not is_legal_c_identifier(identifier):
        raise ClinicError(f"Illegal C identifier: {identifier}")
    # But if we picked a C keyword, pick something else.
    if identifier in _c_keywords:
        return identifier + "_value"
    return identifier
