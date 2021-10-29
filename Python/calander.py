import calendar
import datetime
from datetime import date

L=[]
s=" "
D=(" is a Day ")
date=str(input("Enter the date 'DD MM YEAR'(for example:- 25 12 2020) : "))
def remove(string): 
    return string.replace(" ", "") 
string = (date)
RS=(remove(string) )
def split(word): 
    return list(word) 
word = (RS)
SP=(split(word))
yr=(SP[4::])
for x in range (len(yr)):
    L.append((int(yr[x])))
s=""
for i in L:
    i=str(i)
    s=s+i
year=int(s)
    
print(calendar.calendar(year))
print(" ")
day, month, year = date.split(' ')
day_name = datetime.date(int(year), int(month), int(day))
Day=(day_name.strftime("%A"))
if Day == "Monday" :
    print("Monday" ,D)
elif Day == "Tuesday":
    print("Tuesday",D)
elif Day == "Wednesday":
    print("Wednesday",D)
elif Day == "Thursday":
    print("Thursday",D)
elif Day == "Friday":
    print("Friday",D)
elif Day == "Saturday":
    print("Saturday",D)
else :
    print("Sunday",D)
      

      
