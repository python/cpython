python$target:::line
/(copyinstr(arg1)=="test_line")/
{
    printf("%d\t%s:%s:%s:%d\n", timestamp,
        probename, basename(copyinstr(arg0)),
        copyinstr(arg1), arg2);
}
