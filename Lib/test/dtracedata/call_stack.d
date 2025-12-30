self int indent;

python$target:::function-entry
/copyinstr(arg1) == "start"/
{
    self->trace = 1;
}

python$target:::function-entry
/self->trace/
{
    printf("%d\t%*s:", timestamp, 15, probename);
    printf("%*s", self->indent, "");
    printf("%s:%s:%d:%s\n", basename(copyinstr(arg0)), copyinstr(arg1), arg2,
           copyinstr(arg3));
    self->indent++;
}

python$target:::function-return
/self->trace/
{
    self->indent--;
    printf("%d\t%*s:", timestamp, 15, probename);
    printf("%*s", self->indent, "");
    printf("%s:%s:%d:%s\n", basename(copyinstr(arg0)), copyinstr(arg1), arg2,
           copyinstr(arg3));
}

python$target:::function-return
/copyinstr(arg1) == "start"/
{
    self->trace = 0;
}
