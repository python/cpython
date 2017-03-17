Issue #25583: Avoid incorrect errors raised by os.makedirs(exist_ok=True)
when the OS gives priority to errors such as EACCES over EEXIST.