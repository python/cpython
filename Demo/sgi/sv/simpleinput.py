import sv, SV
import gl, DEVICE

def main():
	gl.foreground()
	gl.prefsize(SV.PAL_XMAX, SV.PAL_YMAX)
	win = gl.winopen('video test')
	v = sv.OpenVideo()
	params = [SV.VIDEO_MODE, SV.COMP, SV.BROADCAST, SV.PAL]
	v.SetParam(params)
	v.BindGLWindow(win, SV.IN_REPLACE)
	gl.qdevice(DEVICE.ESCKEY)
	gl.qdevice(DEVICE.WINQUIT)
	gl.qdevice(DEVICE.WINSHUT)
	while 1:
		dev, val = gl.qread()
		if dev in (DEVICE.ESCKEY, DEVICE.WINSHUT, DEVICE.WINQUIT):
			v.CloseVideo()
			gl.winclose(win)
			return

main()
