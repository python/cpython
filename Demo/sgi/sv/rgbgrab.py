import sys
import sv, SV
import gl, GL, DEVICE
import time

def main():
	v = sv.OpenVideo()
	# Determine maximum window size based on signal standard
	param = [SV.BROADCAST, 0]
	v.GetParam(param)
	if param[1] == SV.PAL:
		width = SV.PAL_XMAX
		height = SV.PAL_YMAX
	elif param[1] == SV.NTSC:
		width = SV.NTSC_XMAX
		height = SV.NTSC_YMAX
	else:
		print 'Unknown video standard', param[1]
		sys.exit(1)

	# Initially all windows are half size
	grabwidth, grabheight = width/2, height/2

	# Open still window
	gl.foreground()
	gl.prefsize(grabwidth, grabheight)
	still_win = gl.winopen('Grabbed frame')
	gl.keepaspect(width, height)
	gl.maxsize(width, height)
	gl.winconstraints()
	gl.RGBmode()
	gl.gconfig()
	gl.clear()
	gl.pixmode(GL.PM_SIZE, 8)

	# Open live window
	gl.foreground()
	gl.prefsize(grabwidth, grabheight)
	live_win = gl.winopen('Live video')
	gl.keepaspect(width, height)
	gl.maxsize(width, height)
	gl.winconstraints()

	# Bind live video
	v.SetSize(gl.getsize())
	v.BindGLWindow(live_win, SV.IN_REPLACE)

	print 'Use leftmouse to grab frame'

	gl.qdevice(DEVICE.LEFTMOUSE)
	gl.qdevice(DEVICE.WINQUIT)
	gl.qdevice(DEVICE.WINSHUT)
	gl.qdevice(DEVICE.ESCKEY)
	frame = None
	while 1:
		dev, val = gl.qread()
		if dev == DEVICE.LEFTMOUSE and val == 0:
			w, h, fields = v.CaptureOneFrame(SV.RGB8_FRAMES, \
				grabwidth, grabheight)
			frame = sv.InterleaveFields(1, fields, w, h)
			gl.winset(still_win)
			gl.lrectwrite(0, 0, w - 1, h - 1, frame)
			gl.winset(live_win)
		if dev in (DEVICE.ESCKEY, DEVICE.WINQUIT, DEVICE.WINSHUT):
			v.CloseVideo()
			gl.winclose(live_win)
			gl.winclose(still_win)
			break
		if dev == DEVICE.REDRAW and val == still_win:
			gl.winset(still_win)
			gl.reshapeviewport()
			gl.clear()
			grabwidth, grabheight = gl.getsize()
			if frame:
				gl.lrectwrite(0, 0, w - 1, h - 1, frame)
			gl.winset(live_win)
		if dev == DEVICE.REDRAW and val == live_win:
			v.SetSize(gl.getsize())
			v.BindGLWindow(live_win, SV.IN_REPLACE)

main()
