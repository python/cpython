import sys
import sv, SV
import gl, GL, DEVICE

def main():
	format = SV.RGB8_FRAMES
	framerate = 25
	queuesize = 16
	samplingrate = 2

	v = sv.OpenVideo()
	# Determine maximum window size based on signal standard
	param = [SV.BROADCAST, 0]
	v.GetParam(param)
	if param[1] == SV.PAL:
		width = SV.PAL_XMAX
		height = SV.PAL_YMAX
		framefreq = 25
	else:
		width = SV.NTSC_XMAX
		height = SV.NTSC_YMAX
		framefreq = 30

	# Allow resizing window if capturing RGB frames, which can be scaled
	if format == SV.RGB8_FRAMES:
		gl.keepaspect(width, height)
		gl.maxsize(width, height)
		gl.stepunit(8, 6)
		gl.minsize(120, 90)
	else:
		if format == SV.YUV411_FRAMES_AND_BLANKING_BUFFER:
			height = height + SV.BLANKING_BUFFER_SIZE
		gl.prefposition(300, 300+width-1, 100, 100+height-1)

	# Open the window
	gl.foreground()
	win = gl.winopen('Continuous Capture')
	gl.RGBmode()
	gl.gconfig()
	if format == SV.RGB8_FRAMES:
		width, height = gl.getsize()
		gl.pixmode(GL.PM_SIZE, 8)
	else:
		gl.pixmode(GL.PM_SIZE, 32)

	svci = (format, width, height, queuesize, samplingrate)
	[svci]

	svci = v.InitContinuousCapture(svci)
	width, height = svci[1:3]
	[svci]

	hz = gl.getgdesc(GL.GD_TIMERHZ)
	gl.noise(DEVICE.TIMER0, hz / framerate)
	gl.qdevice(DEVICE.TIMER0)
	gl.qdevice(DEVICE.WINQUIT)
	gl.qdevice(DEVICE.WINSHUT)
	gl.qdevice(DEVICE.ESCKEY)

	ndisplayed = 0
	lastfieldID = 0

	while 1:
		dev, val = gl.qread()
		if dev == DEVICE.REDRAW:
			oldw = width
			oldh = height
			width, height = gl.getsize()
			if oldw != width or oldh != height:
				v.EndContinuousCapture()
				gl.viewport(0, width-1, 0, height-1)
				svci = (svci[0], width, height) + svci[3:]
				svci = v.InitContinuousCapture(svci)
				width, height = svci[1:3]
				[svci]
				if ndisplayed:
					print 'lost',
					print fieldID/(svci[4]*2) - ndisplayed,
					print 'frames'
				ndisplayed = 0
		elif dev == DEVICE.TIMER0:
			try:
				captureData, fieldID = v.GetCaptureData()
			except sv.error, val:
				if val <> 'no data available':
					print val
				continue
			if fieldID - lastfieldID <> 2*samplingrate:
				print lastfieldID, fieldID
			lastfieldID = fieldID
			if svci[0] == SV.RGB8_FRAMES:
				rgbbuf = captureData.InterleaveFields(1)
			else:
				rgbbuf = captureData.YUVtoRGB(1)
			captureData.UnlockCaptureData()
			gl.lrectwrite(0, 0, width-1, height-1, rgbbuf)
			ndisplayed = ndisplayed + 1
		elif dev in (DEVICE.ESCKEY, DEVICE.WINQUIT, DEVICE.WINSHUT):
			v.EndContinuousCapture()
			v.CloseVideo()
			gl.winclose(win)
			print fieldID, ndisplayed, svci[4]
			print 'lost', fieldID/(svci[4]*2) - ndisplayed,
			print 'frames'
			return

main()
