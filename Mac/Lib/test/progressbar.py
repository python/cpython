# Progress dialog

from addpack import addpack
addpack('Tools')
addpack('bgen')
addpack('dlg')
addpack('evt')

from Dlg import GetNewDialog, ModalDialog, SetDialogItemText

count = 0

def filter(d, e):
	r = 1
	print "Filter(%s, %s) -> %s" % (`d`, `e`, `r`)
	return r

def main():
	d = GetNewDialog(256, -1)
	tp, h, rect = d.GetDialogItem(2)
	SetDialogItemText(h, "Progress...")
	for i in range(100):
		if i%10 == 0:
			str = "Progress...%d" % i
			SetDialogItemText(h, str)
			ModalDialog(filter)
		for j in range(100): pass

if __name__ == '__main__':
	main()

