# Some Common Issues after using 2to3 for code conversion

### 1. python2 "file" is not transpiled to python3 "open" yet
```
Example : 
https://www.travis-ci.org/Python3pkg/AcraNetwork/jobs/235110296

Referece :http://python3porting.com/differences.html
http://python3porting.com/differences.html#file
```
### 2. Configparser to configparser , it seems to be better using following conversion

```
try:
    import Configparser as configparser
except ImportError:
    import configparser
```
### 3. chr and unichar , it seems to be better using following conversion

```
try:
	chr  			# Python2
except NameError: 
	unichr = chr 	# Python3
```
