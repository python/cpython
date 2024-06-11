import io
import tokenize

src = '''\
a = f"""
    Autorzy, którzy wpisani jako AKTUALNA -- czyli
    Autorzy, którzy jednostkę mają wpisani jako AKTUALNA -- czyli
    Autorzy, którzy tą jednostkę mają wpisani jako AKTUALNA -- czyli       """
'''
tokens = list(tokenize.generate_tokens(io.StringIO(src).readline))

for token in tokens:
    print(token)

# assert tokens[4].start == (2, 68), tokens[4].start
