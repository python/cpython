"Support for number formatting using the current locale settings"

from _locale import *
import string

#perform the grouping from right to left
def _group(s):
    conv=localeconv()
    grouping=conv['grouping']
    if not grouping:return s
    result=""
    while s and grouping:
	# if grouping is -1, we are done 
	if grouping[0]==CHAR_MAX:
	    break
	# 0: re-use last group ad infinitum
	elif grouping[0]!=0:
	    #process last group
	    group=grouping[0]
	    grouping=grouping[1:]
	if result:
	    result=s[-group:]+conv['thousands_sep']+result
	else:
	    result=s[-group:]
	s=s[:-group]
    if s and result:
	result=s+conv['thousands_sep']+result
    return result

def format(f,val,grouping=0):
    """Formats a value in the same way that the % formatting would use,
    but takes the current locale into account. 
    Grouping is applied if the third parameter is true."""
    result = f % val
    fields = string.splitfields(result,".")
    if grouping:
	fields[0]=_group(fields[0])
    if len(fields)==2:
	return fields[0]+localeconv()['decimal_point']+fields[1]
    elif len(fields)==1:
	return fields[0]
    else:
	raise Error,"Too many decimal points in result string"
    
def str(val):
    """Convert float to integer, taking the locale into account."""
    return format("%.12g",val)

def atof(str,func=string.atof):
    "Parses a string as a float according to the locale settings."
    #First, get rid of the grouping
    s=string.splitfields(str,localeconv()['thousands_sep'])
    str=string.join(s,"")
    #next, replace the decimal point with a dot
    s=string.splitfields(str,localeconv()['decimal_point'])
    str=string.join(s,'.')
    #finally, parse the string
    return func(str)

def atoi(str):
    "Converts a string to an integer according to the locale settings."
    return atof(str,string.atoi)

def test():
    setlocale(LC_ALL,"")
    #do grouping
    s1=format("%d",123456789,1)
    print s1,"is",atoi(s1)
    #standard formatting
    s1=str(3.14)
    print s1,"is",atof(s1)
    

if __name__=='__main__':
    test()
