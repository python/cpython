import tokenize
from pprint import pprint
from io import BytesIO

def t(s):
  pprint(list(tokenize.tokenize(BytesIO(s.encode()).readline)))


a = r'f"abc\
def"'

t(a)
