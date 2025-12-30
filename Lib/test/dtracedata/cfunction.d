self int tracing;

python$target:::function-entry
/copyinstr(arg1) == "start"/
{
    self->tracing = 1;
}

python$target:::cfunction-entry,
python$target:::cfunction-return
/self->tracing/
{
    this->module = arg3 ? copyinstr(arg3) : "";
    printf("%d\t%s:%s:%d:%s:%s\n", timestamp, probename,
           copyinstr(arg0), arg2, this->module, copyinstr(arg1));
}

python$target:::function-return
/copyinstr(arg1) == "start"/
{
    self->tracing = 0;
}
