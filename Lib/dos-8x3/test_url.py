# Minimal test of the quote function
import urllib

chars = 'abcdefghijklmnopqrstuvwxyz'\
        '\337\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356' \
        '\357\360\361\362\363\364\365\366\370\371\372\373\374\375\376\377' \
        'ABCDEFGHIJKLMNOPQRSTUVWXYZ' \
        '\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317' \
        '\320\321\322\323\324\325\326\330\331\332\333\334\335\336'

expected = 'abcdefghijklmnopqrstuvwxyz%df%e0%e1%e2%e3%e4%e5%e6%e7%e8%e9%ea%eb%ec%ed%ee%ef%f0%f1%f2%f3%f4%f5%f6%f8%f9%fa%fb%fc%fd%fe%ffABCDEFGHIJKLMNOPQRSTUVWXYZ%c0%c1%c2%c3%c4%c5%c6%c7%c8%c9%ca%cb%cc%cd%ce%cf%d0%d1%d2%d3%d4%d5%d6%d8%d9%da%db%dc%dd%de'

test = urllib.quote(chars)
assert test == expected, "urllib.quote problem"
test2 = urllib.unquote(expected)
assert test2 == chars

in1 = "abc/def"
out1_1 = "abc/def"
out1_2 = "abc%2fdef"

assert urllib.quote(in1) == out1_1, "urllib.quote problem"
assert urllib.quote(in1, '') == out1_2, "urllib.quote problem"

in2 = "abc?def"
out2_1 = "abc%3fdef"
out2_2 = "abc?def"

assert urllib.quote(in2) == out2_1, "urllib.quote problem"
assert urllib.quote(in2, '?') == out2_2, "urllib.quote problem"


