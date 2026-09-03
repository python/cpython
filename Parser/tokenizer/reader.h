#ifndef Py_TOKENIZER_READER_H
#define Py_TOKENIZER_READER_H

struct tok_state;

void _PyTok_ReaderFree(struct tok_state *);
int _PyTok_ReaderUnderflow(struct tok_state *);

#endif
