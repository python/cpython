__attribute__((naked))
void
_JIT_ENTRY(void)
{
#ifdef __aarch64__
    asm("mov  x8, #:abs_g0_nc:_JIT_CONTINUE\n"
        "movk x8, #:abs_g1_nc:_JIT_CONTINUE\n"
        "movk x8, #:abs_g2_nc:_JIT_CONTINUE\n"
        "movk x8, #:abs_g3   :_JIT_CONTINUE\n"
        "br   x8\n");
#endif
#ifdef __x86_64__
    asm("movabsq $_JIT_CONTINUE, %rax\n"
        "jmpq                   *%rax\n");
#endif
}