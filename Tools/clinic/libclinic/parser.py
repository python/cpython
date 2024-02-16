import re


RE_FULLNAME = re.compile(r"\s*([\w.]+)\s*")
RE_C_BASENAME = re.compile(r"\bas\b\s*(?:([^-=\s]+)\s*)?")
RE_CLONE = re.compile(r"=\s*(?:([^-=\s]+)\s*)?")
RE_RETURNS = re.compile(r"->\s*(.*)")
