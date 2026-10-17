python$target:::function-entry
/copyinstr(arg1) == "start"/
{
    self->trace = 1;
}

python$target:::gc-start,
python$target:::gc-done
/self->trace/
{
    printf("%d\t%s:%ld\n", timestamp, probename, arg0);
}

python$target:::function-return
/copyinstr(arg1) == "start"/
{
    self->trace = 0;
}
