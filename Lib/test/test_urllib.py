# Minimal test of the quote function
from test_support import verify, verbose
import urllib

chars = 'abcdefghijklmnopqrstuvwxyz'\
        '\337\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356' \
        '\357\360\361\362\363\364\365\366\370\371\372\373\374\375\376\377' \
        'ABCDEFGHIJKLMNOPQRSTUVWXYZ' \
        '\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317' \
        '\320\321\322\323\324\325\326\330\331\332\333\334\335\336'

expected = 'abcdefghijklmnopqrstuvwxyz' \
           '%DF%E0%E1%E2%E3%E4%E5%E6%E7%E8%E9%EA%EB%EC%ED%EE' \
           '%EF%F0%F1%F2%F3%F4%F5%F6%F8%F9%FA%FB%FC%FD%FE%FF' \
           'ABCDEFGHIJKLMNOPQRSTUVWXYZ' \
           '%C0%C1%C2%C3%C4%C5%C6%C7%C8%C9%CA%CB%CC%CD%CE%CF' \
           '%D0%D1%D2%D3%D4%D5%D6%D8%D9%DA%DB%DC%DD%DE'

test = urllib.quote(chars)
verify(test == expected, "urllib.quote problem 1")
test2 = urllib.unquote(expected)
verify(test2 == chars)

in1 = "abc/def"
out1_1 = "abc/def"
out1_2 = "abc%2Fdef"

verify(urllib.quote(in1) == out1_1, "urllib.quote problem 2")
verify(urllib.quote(in1, '') == out1_2, "urllib.quote problem 3")

in2 = "abc?def"
out2_1 = "abc%3Fdef"
out2_2 = "abc?def"

verify(urllib.quote(in2) == out2_1, "urllib.quote problem 4")
verify(urllib.quote(in2, '?') == out2_2, "urllib.quote problem 5")



in3 = {"p1":"v1","p2":"v2"}
in3list = [("p1", "v1"), ("p2","v2")]
exp3_1 = "p2=v2&p1=v1"
exp3_2 = "p1=v1&p2=v2"
# dict input, only string values
act3 = urllib.urlencode(in3)
verify(act3==exp3_1 or act3==exp3_2, "urllib.urlencode problem 1 dict")
# list input, only string values
act3list = urllib.urlencode(in3list)
verify(act3list==exp3_2, "urllib.urlencode problem 1 list")


in4 = {"p1":["v1","v2"]}
in4list = [("p1", ["v1","v2"])]
exp4 = "p1=v1&p1=v2"
# dict input, list values, doseq==1
act4 = urllib.urlencode(in4,doseq=1)
verify(act4==exp4, "urllib.urlencode problem 2 dict")
# list input, list values, doseq==1
act4list = urllib.urlencode(in4,doseq=1)
verify(act4list==exp4, "urllib.urlencode problem 2 list")


in5 = in4
in5list = in4list
exp5 = "p1=%5B%27v1%27%2C+%27v2%27%5D"
exp5list = "p1=%5B%27v1%27%2C+%27v2%27%5D"
# dict input, list variables, doseq=0
act5 = urllib.urlencode(in5)
verify(act5==exp5, "urllib.urlencode problem 3 dict")
# list input, list variables, doseq=0
act5list = urllib.urlencode(in5list)
verify(act5list==exp5list, "urllib.urlencode problem 3 list")


in6 = {"p1":"v1","p2":"v2"}
in6list = [("p1", "v1"), ("p2","v2")]
exp6_1 = "p2=v2&p1=v1"
exp6_2 = "p1=v1&p2=v2"
# dict input, only string values, doseq==1
act6 = urllib.urlencode(in6,doseq=1)
verify(act6==exp6_1 or act6==exp6_2, "urllib.urlencode problem 4 dict")
# list input, only string values
act6list = urllib.urlencode(in6list,doseq=1)
verify(act6list==exp6_2, "urllib.urlencode problem 4 list")


in7 = "p1=v1&p2=v2"
try:
    act7 = urllib.urlencode(in7)
    print "urllib.urlencode problem 5 string"
except TypeError:
    pass


import UserDict
in8 = UserDict.UserDict()
in8["p1"] = "v1"
in8["p2"] = ["v1", "v2"]
exp8_1 = "p1=v1&p2=v1&p2=v2"
exp8_2 = "p2=v1&p2=v2&p1=v1"
act8 = urllib.urlencode(in8,doseq=1)
verify(act8==exp8_1 or act8==exp8_2, "urllib.urlencode problem 6 UserDict")


import UserString
in9 = UserString.UserString("")
exp9 = ""
act9 = urllib.urlencode(in9,doseq=1)
verify(act9==exp9, "urllib.urlencode problem 7 UserString")
