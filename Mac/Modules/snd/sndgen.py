# Generated from 'D:Development:THINK C:Mac #includes:Apple #includes:Sound.h'

f = SndMethod(OSErr, 'SndDoCommand',
    (SndChannelPtr, 'chan', InMode),
    (SndCommand_ptr, 'cmd', InMode),
    (Boolean, 'noWait', InMode),
)
sndmethods.append(f)

f = SndMethod(OSErr, 'SndDoImmediate',
    (SndChannelPtr, 'chan', InMode),
    (SndCommand_ptr, 'cmd', InMode),
)
sndmethods.append(f)

f = SndFunction(OSErr, 'SndNewChannel',
    (SndChannelPtr, 'chan', OutMode),
    (short, 'synth', InMode),
    (long, 'init', InMode),
    (SndCallBackProcPtr, 'userRoutine', InMode),
)
functions.append(f)

f = SndMethod(OSErr, 'SndPlay',
    (SndChannelPtr, 'chan', InMode),
    (SndListHandle, 'sndHdl', InMode),
    (Boolean, 'async', InMode),
)
sndmethods.append(f)

f = SndFunction(OSErr, 'SndControl',
    (short, 'id', InMode),
    (SndCommand, 'cmd', OutMode),
)
functions.append(f)

f = SndFunction(void, 'SetSoundVol',
    (short, 'level', InMode),
)
functions.append(f)

f = SndFunction(void, 'GetSoundVol',
    (short, 'level', OutMode),
)
functions.append(f)

f = SndFunction(NumVersion, 'SndSoundManagerVersion',
)
functions.append(f)

f = SndMethod(OSErr, 'SndStartFilePlay',
    (SndChannelPtr, 'chan', InMode),
    (short, 'fRefNum', InMode),
    (short, 'resNum', InMode),
    (long, 'bufferSize', InMode),
    (FakeType('0'), 'theBuffer', InMode),
    (AudioSelectionPtr, 'theSelection', InMode),
    (ProcPtr, 'theCompletion', InMode),
    (Boolean, 'async', InMode),
)
sndmethods.append(f)

f = SndMethod(OSErr, 'SndPauseFilePlay',
    (SndChannelPtr, 'chan', InMode),
)
sndmethods.append(f)

f = SndMethod(OSErr, 'SndStopFilePlay',
    (SndChannelPtr, 'chan', InMode),
    (Boolean, 'async', InMode),
)
sndmethods.append(f)

f = SndMethod(OSErr, 'SndChannelStatus',
    (SndChannelPtr, 'chan', InMode),
    (short, 'theLength', InMode),
    (SCStatus, 'theStatus', OutMode),
)
sndmethods.append(f)

f = SndFunction(OSErr, 'SndManagerStatus',
    (short, 'theLength', InMode),
    (SMStatus, 'theStatus', OutMode),
)
functions.append(f)

f = SndFunction(void, 'SndGetSysBeepState',
    (short, 'sysBeepState', OutMode),
)
functions.append(f)

f = SndFunction(OSErr, 'SndSetSysBeepState',
    (short, 'sysBeepState', InMode),
)
functions.append(f)

f = SndFunction(NumVersion, 'MACEVersion',
)
functions.append(f)

f = SndFunction(void, 'Comp3to1',
    (InOutBuffer, 'buffer', InOutMode),
    (InOutBuf128, 'state', InOutMode),
    (unsigned_long, 'numChannels', InMode),
    (unsigned_long, 'whichChannel', InMode),
)
functions.append(f)

f = SndFunction(void, 'Exp1to3',
    (InOutBuffer, 'buffer', InOutMode),
    (InOutBuf128, 'state', InOutMode),
    (unsigned_long, 'numChannels', InMode),
    (unsigned_long, 'whichChannel', InMode),
)
functions.append(f)

f = SndFunction(void, 'Comp6to1',
    (InOutBuffer, 'buffer', InOutMode),
    (InOutBuf128, 'state', InOutMode),
    (unsigned_long, 'numChannels', InMode),
    (unsigned_long, 'whichChannel', InMode),
)
functions.append(f)

f = SndFunction(void, 'Exp1to6',
    (InOutBuffer, 'buffer', InOutMode),
    (InOutBuf128, 'state', InOutMode),
    (unsigned_long, 'numChannels', InMode),
    (unsigned_long, 'whichChannel', InMode),
)
functions.append(f)

