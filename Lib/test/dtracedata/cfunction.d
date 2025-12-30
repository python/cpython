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
    this->module = arg0 ? copyinstr(arg0) : "";
    printf("%d\t%s:%s:%s\n", timestamp, probename, this->module, copyinstr(arg1));
}

python$target:::function-return
/copyinstr(arg1) == "start"/
{
    self->tracing = 0;
}
