#! /usr/bin/env python

# Print the value of all video parameters

import sys
import sv, SV

def main():
	v = sv.OpenVideo()
	for name in dir(SV):
		const = getattr(SV, name)
		if type(const) is type(0):
			sys.stdout.flush()
			params = [const, 0]
			try:
				v.GetParam(params)
			except sv.error, msg:
##				print name, msg
				continue
			print name, params

main()

