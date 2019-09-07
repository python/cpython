#  datetime.timestamp() fails when the 'datetime' is close to the Epoch
#  Here is the test runs on win10 os, python3.7-3.9. My time is 'utc +8'

from datetime import datetime

def test(year,month,day,hour=0,minute=0,second=0):
    t = datetime(year,month,day,hour)
    try:
        print(t.timestamp())
    except Exception as e:
        print(e)
    return 

test(1970,1,1)
test(1970,1,1,8)
test(1970,1,2,7,59,59)  
test(1970,1,2,8)  # this time is just okay
