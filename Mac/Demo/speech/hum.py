#
# Hum - The singing macintosh
#
import macspeech
import sys
import string

dict = { 'A':57, 'A#':58, 'B':59, 'C':60, 'C#':61, 'D':62, 'D#':63,
		 'E':64, 'F':65, 'F#':66, 'G':67, 'G#':68}

vd = macspeech.GetIndVoice(2)
vc = vd.NewChannel()
print 'Input strings of notes, as in A B C C# D'
while 1:
	print 'S(tr)ing-',
	str = sys.stdin.readline()
	if not str:
		break
	str = string.split(str[:-1])
	data = []
	for s in str:
		if not dict.has_key(s):
			print 'No such note:', s
		else:
			data.append(dict[s])
	print data
	for d in data:
		vc.SetPitch(float(d))
		vc.SpeakText('la')
		while macspeech.Busy():
			pass
