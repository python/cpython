Issue #27948: In f-strings, only allow backslashes inside the braces
(where the expressions are).  This is a breaking change from the 3.6
alpha releases, where backslashes are allowed anywhere in an
f-string.  Also, require that expressions inside f-strings be
enclosed within literal braces, and not escapes like
f'\x7b"hi"\x7d'.