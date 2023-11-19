typedef  void* (py_evaluator)(void* ts, void* f, int t);

void*
trampoline(void *ts, void *f,
           int throwflag, py_evaluator evaluator)
{
     return evaluator(ts, f, throwflag);
}
