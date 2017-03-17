lib2to3.pgen3.driver.load_grammar() now creates a stable cache file
between runs given the same Grammar.txt input regardless of the hash
randomization setting.