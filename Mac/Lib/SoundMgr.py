#Constants for the MacOS Sound Manager

nullCmd = 0
initCmd = 1
freeCmd = 2
quietCmd = 3
flushCmd = 4
reInitCmd = 5
	
waitCmd = 10
pauseCmd = 11
resumeCmd = 12
callBackCmd = 13

syncCmd = 14
emptyCmd = 15

tickleCmd = 20
requestNextCmd = 21
howOftenCmd = 22
wakeUpCmd = 23
availableCmd = 24
versionCmd = 25
totalLoadCmd = 26
loadCmd = 27

scaleCmd = 30
tempoCmd = 31

freqDurationCmd = 40
restCmd = 41
freqCmd = 42
ampCmd = 43
timbreCmd = 44
getAmpCmd = 45

waveTableCmd = 60
phaseCmd = 61

soundCmd = 80
bufferCmd = 81
rateCmd = 82
continueCmd = 83
doubleBufferCmd = 84
getRateCmd = 85

sizeCmd = 90
convertCmd = 91

stdQLength = 128
dataOffsetFlag = 0x8000

waveInitChannelMask = 0x07
waveInitChannel0 = 0x04
waveInitChannel1 = 0x05
waveInitChannel2 = 0x06
waveInitChannel3 = 0x07

stdSH = 0x00 # Standard sound header encode value
extSH = 0xFF # Extended sound header encode value
cmpSH = 0xFE # Compressed sound header encode value

initSRate22k = 0x20 # 22k sampling rate - sampleSynth only
initSRate44k = 0x30 # 44k sampling rate - sampleSynth only
