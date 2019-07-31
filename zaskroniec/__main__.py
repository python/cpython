from googletrans import Translator
from lexit import Lexer
from sys import *

translator = Translator()

output = ''

for line in open(argv[1]).read().splitlines():
    class SLexer(Lexer):
        STRING = r'"(\\\"|\\\\|[^"\n])*?"i?'
        ANY = r'([^"])+'
        ignore = r'\s+'

    tokens = list(SLexer.lex(line))

    for t in tokens:
        value = t.value
        if t.type == 'ANY':
            value = translator.translate(t.value.replace(' ', '~ '), dest='en', src='pl').text.replace('\xa0', ' ').replace('~ ', ' ').replace('define', 'def')
        output += value

    output += '\n'

try:
    exec(output)
except Exception as e:
    print('\x1b[91;1mbłąd:\x1b[0m', translator.translate(str(e), dest='pl', src='en').text)
