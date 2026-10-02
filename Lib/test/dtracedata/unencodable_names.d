python$target:::function-entry,
python$target:::function-return
/arg0 == 0 || arg1 == 0/
{
    printf("%d\t%s:%d\n", timestamp, probename, arg2);
}

python$target:::import-find-load-start
/arg0 == 0/
{
    printf("%d\t%s\n", timestamp, probename);
}

python$target:::import-find-load-done
/arg0 == 0/
{
    printf("%d\t%s:%d\n", timestamp, probename, arg1);
}
