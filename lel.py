import tokenize
import io
import pprint

data = """\
if False:\n    # NL\n    \n    True = False # NEWLINE\n
"""
b = io.BytesIO(data.encode())
pprint.pprint(list(tokenize.tokenize(b.readline)))
print()
print()
b = io.BytesIO(data.encode())
pprint.pprint(list(tokenize.tokenize2(b.readline)))
print()
print()
pprint.pprint(list(tokenize._generate_tokens_from_c_tokenizer(data)))
