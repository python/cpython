Issue #19771: Also in runpy and the "-m" option, omit the irrelevant
message ". . . is a package and cannot be directly executed" if the package
could not even be initialized (e.g. due to a bad ``*.pyc`` file).