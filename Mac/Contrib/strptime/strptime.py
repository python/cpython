""" strptime version 1.3, Time-stamp: <96/09/23 21:22:24 flognat>
               The reverse of strftime.

    Copyright (C) 1996 Andy Eskilsson, flognat@fukt.hk-r.se

    This is free software; unrestricted redistribution is allowed under the
    terms of the GPL.  For full details of the license conditions of this
    software, see the GNU General Public License.

    And here comes the documentation:

    Throw a string and a format specification at strptime and if everything
    is ok you will get a tuple containing 9 items that are compatible with
    pythons time-module.

    interface:
       strptime(inputstring, formatstring)

       Little errorchecking... so you'd better know what you are doing.

    example:
       from strptime import *
       mktime(strptime("26/6 1973", "%d/%m %Y"))

    And voila you have the second when the author of this function was born.

    The supported format identifiers are:
        %a weekday in short text-form, e.g. Mon
	%A weekday in long text-form, e.g. Monday
	%b month in short text-form, e.g. Jul
	%B month in long text-form e.g. July
	%c the format specified by DateAndTimeRepresentation
	%d the day in month in numeric form, e.g. 24
	%H hour in 24 hour form
	%j julian day (day of year)
	%m month in numeric format
	%M minute
	%S second
	%T Time in '%H:%M:%S'-format
	%w weekday, 0=monday
	%x date in format represented by DateRepresentation
	%X time in format represented by TimeRepresentation
	%y year in short form
	%Y year in long form
	%% %-sign

    I have done some thinking here (*REALLY*) and it is possible to configure
    this module so it uses other languages by adding their names to the
    dictionaries first in the file, and setting the variable LANGUAGE.

    For your exercise I have inserted the swedish names ;-)

    The lfind, name, complex, numbers and parse functions are for internal
    use, called by strptime.

    Uh.. oh yeah.. if you want to get in touch with me.. I am reachable
    at flognat@fukt.hk-r.se, the newest version of this file can probably
    be found somewhere close to http://www.fukt.hk-r.se/~flognat

    If you like it, send a postcard to Andy Eskilsson
                                       Kämnärsv. 3b228
				       S-226 46 Lund
				       Sweden

    Uhm be gentle with the bug-reports, its the first time for me ;-)

    """

import string

LongDayNames={ 'English' : [ 'Monday', 'Tuesday', 'Wednesday',
			     'Thursday', 'Friday', 'Saturday', 'Sunday'],
	       'Swedish' : [ 'Måndag', 'Tisdag', 'Onsdag', 'Torsdag',
			     'Fredag', 'Lördag', 'Söndag']}
ShortDayNames={ 'English' : [ 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun'],
		'Swedish' : [ 'Mån', 'Tis', 'Ons', 'Tor', 'Fre', 'Lör', 'Sön']}

LongMonthNames={ 'English' : ['none', 'January', 'February', 'March', 'April',
			      'May', 'June', 'July', 'August', 'September',
			      'October', 'November', 'December'],
		 'Swedish' : ['none', 'Januari', 'Februari', 'Mars', 'April',
			      'Maj', 'Juni', 'Juli', 'Augusti','September',
			      'Oktober', 'November', 'December'] }
ShortMonthNames={ 'English' : ['none', 'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun',
			       'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'],
		  'Swedish' : ['none', 'Jan', 'Feb', 'Mar', 'Apr', 'Maj', 'Jun',
			       'Jul', 'Aug', 'Sep', 'Okt', 'Nov', 'Dec']}
DateAndTimeRepresentation={ 'English' : '%a %b %d %H:%m:%S %Y',
			    'Swedish' : '%a %d %b %Y %H:%m:%S' }

DateRepresentation = { 'English' : '%m/%d/%y',
		       'Swedish' : '%d/%m/%y'}

TimeRepresentation = { 'English' : '%H:%M:%S',
		       'Swedish' : '%H:%M:%S'}

LANGUAGE='English'

BadFormatter='An illegal formatter was given'

#Check if strinf begins with substr
def lfind(str, substr):
   return string.lower(str[:len(substr)])==string.lower(substr)

#atoms consisting of other atoms
def complex(str, format, base):
   code=format[:1]
   if code=='c': 
      string=DateAndTimeRepresentation[LANGUAGE]
   elif code=='T':
      string='%H:%M:%S'
   elif code=='x':
      string=DateRepresentation[LANGUAGE]
   elif code=='X':
      string=TimeRepresentation[LANGUAGE]

   return parse(str, string, base)

#string based names
def names(str, format, base):
   code=format[:1]
   if code=='a':
      selection=ShortDayNames[LANGUAGE]
      result='weekd'
   elif code=='A':
      selection=LongDayNames[LANGUAGE]
      result='weekd'
   elif code=='b':
      selection=ShortMonthNames[LANGUAGE]
      result='month'
   elif code=='B':
      selection=LongMonthNames[LANGUAGE]
      result='month'
   
   match=None
   for i in selection:
      if lfind(str, i):
	 match=i
	 break

   base[result]=selection.index(match)
   return len(match)

#numeric stuff
def numeric(str, format, base):
   code=format[:1]
   if code=='d': result='day'
   elif code=='H': result='hour'
   elif code=='j': result='juliand'
   elif code=='m': result='month'
   elif code=='M': result='min'
   elif code=='S': result='sec'
   elif code=='w': result='weekd'
   elif code=='y': result='shortYear'
   elif code=='Y': result='year'

   i=0
   while str[i] in string.whitespace: i=i+1
   j=i
   if len(format)>1:
      while not str[j] in string.whitespace and str[j]!=format[1]: j=j+1
   else:
      try:
	 while not str[j] in string.whitespace: j=j+1
      except IndexError:
	 pass

   # hmm could check exception here, but what could I add?
   base[result]=string.atoi(str[i:j])
   
   return j

parseFuns={ 'a':names, 'A':names, 'b':names, 'B':names, 'c':complex, 'd':numeric,
	    'H':numeric, 'j':numeric, 'm':numeric, 'M':numeric, 'S':numeric,
	    'T':complex, 'w':numeric, 'x':complex, 'y':numeric, 'Y':numeric}

# Well split up in atoms, reason to why this is separated from atrptime
# is to be able to reparse complex atoms
def parse(str, format, base):
   atoms=string.split(format, '%')
   charCounter=0
   atomCounter=0

   # Hey I am laazy and think that the format is exactly what the string is!
   charCounter=charCounter+len(atoms[atomCounter])
   atomCounter=atomCounter+1

   while atomCounter < len(atoms) and charCounter < len(str):
      atom=atoms[atomCounter]

      if atom=='': # escaped
	 charCounter=charCounter+1
	 atomCounter=atomCounter+1
	 charCounter=charCounter+len(atoms[atomCounter])
      else:
	 try:
	    parsefunction=parseFuns[atom[:1]]
	 except KeyError:
	    raise BadFormatter, atom[:1]
	 grabbed=apply(parsefunction, (str[charCounter:], atom, base))
	 charCounter=charCounter+grabbed+len(atom)-1

      atomCounter=atomCounter+1

   return charCounter

# Ok here we go, tadaaa --> STRPTIME <-- at last..
def strptime(str, format):
   """Converts str specified by format to tuple useable by the time module"""
   returnTime={}
   returnTime['year']=0
   returnTime['shortYear']=None
   returnTime['month']=0
   returnTime['day']=0
   returnTime['hour']=0
   returnTime['min']=0
   returnTime['sec']=0
   returnTime['weekd']=0
   returnTime['juliand']=0
   returnTime['dst']=0

   parse(str, format, returnTime)

   if returnTime['shortYear']!=None:
      returnTime['year']=returnTime['shortYear']+1900

   return (returnTime['year'], returnTime['month'], returnTime['day'],
	   returnTime['hour'], returnTime['min'], returnTime['sec'],
	   returnTime['weekd'], returnTime['juliand'], returnTime['dst'])

# just for my convenience
def strpdebug():
   import pdb
   pdb.run('strptime("% Tue 3 Feb", "%% %a %d %b")')

def test():
   from time import *
   a=asctime(localtime(time()))
   print a
   b=strptime(a, '%a %b %d %H:%M:%S %Y')
   print asctime(b)
   print strptime("%% % Tue 3 Feb", "%%%% %% %a %d %b")
   print strptime('Thu, 12 Sep 1996 19:42:06 GMT', '%a, %d %b %Y %T GMT')
   print strptime('Thu, 12 Sep 1996 19:42:06 GMT', '%a, %d %b %Y %T')
   print strptime('Thu, 12 Sep 1996 19:42:06', '%a, %d %b %Y %T')
   
if __name__ == '__main__':
   test()


