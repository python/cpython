#pragma once

#include <MixedMode.h>
#include <Dialogs.h>
#include <Files.h>
#include <Windows.h>
#include <AppleEvents.h>
#include <StandardFile.h>

#if defined(powerc) || defined (__powerc)
#pragma options align=mac68k
#endif

typedef struct
{
	FSSpec	spec;	//	designates file on disk
	long	key;	//	reserved for future expansion

	char	tree;	//	0 for absolute, 1 for project, 2 for system
	Boolean	found;	//	FALSE if file couldn't be located; if so, all other info is moot

	OSType	type;	//	file type of found file
	OSType	crtr;	//	signature of found file's creator
	
	short	spare0;	//	reserved for future expansion
	long	spare1;
} ProjectEntry;

enum
{
	kNeitherTree,
	kProjectTree,
	kSystemTree
};

enum
{
	kTHINKCProject,
	kTHINKPascalProject,
	kCodeWarriorProject
};

//	masks for the "flags" argument to new-convention interfaces

#define xfWindowOpen		0x00000001
#define xfWindowChangeable	0x00000002
#define xfHasSelection		0x00000004
#define xfUseDefaults		0x00000008
#define xfIsBBEditLite		0x00000040
#define xfIsBBEditDemo		0x00000080

typedef struct
{
	FSSpec	spec;
	OSType	key;
	
	short	error_kind;
	long	line_number;
	
	Str255	message;
} ErrorEntry;

typedef enum
{
	kNote	= 0,
	kError,
	kWarning
} ErrorKind;

#define kCurrentExternalVersion		5	//	current version of callbacks

//	Universal callback interfaces

#if USESROUTINEDESCRIPTORS

#define ExtensionUPPInfo (kPascalStackBased \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(ExternalCallbackBlock *))) \
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(WindowPtr))))

#define NewExtensionUPPInfo (kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(OSErr))) \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(ExternalCallbackBlock *))) \
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(WindowPtr))) \
							| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(long))) \
							| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(AppleEvent *))) \
							| STACK_ROUTINE_PARAMETER(5, SIZE_CODE(sizeof(AppleEvent *))))

#define GetWindowContentsUPPInfo (kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(Handle))) \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(WindowPtr))))
							
#define GetSelectionUPPInfo (kPascalStackBased \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(long *))) \
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(long *))) \
							| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(long *))))
							
#define SetSelectionUPPInfo (kPascalStackBased \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(long))) \
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(long))) \
							| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(long))))

#define GetDocInfoUPPInfo (kPascalStackBased \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(WindowPtr))) \
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(unsigned char *))) \
							| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(short *))) \
							| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(long *))))

#define GetModDateUPPInfo (kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(long))) \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(WindowPtr))))
							
#define CopyUPPInfo (kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(Handle))))
							
#define PasteUPPInfo (kPascalStackBased \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(Handle))))
							
#define GetLastLineUPPInfo (kPascalStackBased | RESULT_SIZE(SIZE_CODE(sizeof(long))))

#define GetLineNumberUPPInfo (kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(long))) \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(long))))
							
#define GetLineStartUPPInfo (kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(long))) \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(long))))
							
#define GetLineEndUPPInfo (kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(long))) \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(long))))

#define GetLinePosUPPInfo (kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(long))) \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(long))))

#define InsertUPPInfo (kPascalStackBased \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(char *))) \
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(long))))
							
#define DeleteUPPInfo (kPascalStackBased)

#define SetWindowContentsUPPInfo (kPascalStackBased \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(WindowPtr))) \
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(Handle))))
							
#define ContentsChangedUPPInfo (kPascalStackBased \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(WindowPtr))))
							
#define GetFileTextUPPInfo (kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(Handle))) \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(short))) \
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(long))) \
							| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(unsigned char *))) \
							| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(Boolean *))))
							
#define GetFolderUPPInfo (kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(Boolean))) \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(unsigned char *))) \
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(short *))) \
							| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(long *))))
							
#define OpenSeveralUPPInfo (kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(Boolean))) \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(Boolean))) \
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(short *))) \
							| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(StandardFileReply ***))))
							
#define CenterDialogUPPInfo (kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(DialogPtr))) \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(short))))
							
#define StandardFilterUPPInfo uppModalFilterProcInfo
							
#define FrameDialogItemUPPInfo uppUserItemProcInfo

#define NewDocumentUPPInfo	(kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(WindowPtr))))
							
#define OpenDocumentUPPInfo	(kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(WindowPtr))))
							
#define AllocateUPPInfo		(kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(Handle))) \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(long))) \
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(Boolean))))
							
#define FindPatternUPPInfo	(kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(long))) \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(char *))) \
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(long))) \
							| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(long))) \
							| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(char *))) \
							| STACK_ROUTINE_PARAMETER(5, SIZE_CODE(sizeof(long))) \
							| STACK_ROUTINE_PARAMETER(6, SIZE_CODE(sizeof(Boolean))))

#define ReportOSErrorUPPInfo	(kPascalStackBased)

#define GetPreferenceUPPInfo (kPascalStackBased \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(ResType))) \
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(short))) \
							| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(void *))) \
							| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(short *))))
							
#define SetPreferenceUPPInfo (kPascalStackBased \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(ResType))) \
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(short))) \
							| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(void *))) \
							| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(short *))))

#define StartProgressUPPInfo (kPascalStackBased \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(unsigned char *))) \
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(long))) \
							| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(Boolean))))
							
#define DoProgressUPPInfo	(kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(Boolean))) \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(long))))

#define DoneProgressUPPInfo	(kPascalStackBased)

#define GetProjectListUPPInfo (kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(Boolean))) \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(FSSpec *))) \
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(short *))) \
							| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(short *))) \
							| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(ProjectEntry***))))
							
#define ProjectTextListUPPInfo (kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(Boolean))) \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(FSSpec *))) \
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(Handle *))))

#define PresetUndoUPPInfo	(kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(Boolean))))
							
#define SetUndoUPPInfo		(kPascalStackBased)

#define OpenFileUPPInfo		(kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(Boolean))) \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(FSSpec *))) \
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(WindowPtr *))))

#define PrepareUndoUPPInfo	(kPascalStackBased \
							| RESULT_SIZE(SIZE_CODE(sizeof(Boolean))) \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(long))) \
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(long))) \
							| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(long))) \
							| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(long))))
							
#define CommitUndoUPPInfo	(kPascalStackBased \
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(long))))

#define CreateResultsUPPInfo	(kPascalStackBased \
								| RESULT_SIZE(SIZE_CODE(sizeof(Boolean))) \
								| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(unsigned char *))) \
								| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(short))) \
								| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(Handle))) \
								| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(WindowPtr *))))
							
typedef	UniversalProcPtr	GetWindowContentsProc;
typedef	UniversalProcPtr	GetSelectionProc;
typedef	UniversalProcPtr	SetSelectionProc;
typedef	UniversalProcPtr	GetDocInfoProc;
typedef	UniversalProcPtr	GetModDateProc;
typedef	UniversalProcPtr	CopyProc;
typedef	UniversalProcPtr	PasteProc;
	
typedef	UniversalProcPtr	GetLastLineProc;
typedef	UniversalProcPtr	GetLineNumberProc;
typedef	UniversalProcPtr	GetLineStartProc;
typedef	UniversalProcPtr	GetLineEndProc;
typedef	UniversalProcPtr	GetLinePosProc;
	
typedef	UniversalProcPtr	InsertProc;
typedef	UniversalProcPtr	DeleteProc;
	
typedef	UniversalProcPtr	SetWindowContentsProc;
typedef	UniversalProcPtr	ContentsChangedProc;
	
typedef	UniversalProcPtr	GetFileTextProc;

typedef	UniversalProcPtr	GetFolderProc;
typedef	UniversalProcPtr	OpenSeveralProc;
	
typedef	UniversalProcPtr	CenterDialogProc;
typedef	UniversalProcPtr	StandardFilterProc;
typedef	UniversalProcPtr	FrameDialogItemProc;
	
typedef	UniversalProcPtr	NewDocumentProc;
typedef	UniversalProcPtr	OpenDocumentProc;

typedef	UniversalProcPtr	AllocateProc;
typedef	UniversalProcPtr	FindPatternProc;
	
typedef	UniversalProcPtr	ReportOSErrorProc;
	
typedef	UniversalProcPtr	GetPreferenceProc;
typedef	UniversalProcPtr	SetPreferenceProc;

typedef	UniversalProcPtr	StartProgressProc;
typedef	UniversalProcPtr	DoProgressProc;
typedef	UniversalProcPtr	DoneProgressProc;
	
typedef	UniversalProcPtr	GetProjectListProc;
typedef	UniversalProcPtr	ProjectTextListProc;
		
typedef	UniversalProcPtr	PresetUndoProc;
typedef	UniversalProcPtr	SetUndoProc;
	
typedef	UniversalProcPtr	OpenFileProc;

typedef UniversalProcPtr	PrepareUndoProc;
typedef UniversalProcPtr	CommitUndoProc;

typedef UniversalProcPtr	CreateResultsProc;

#define CallGetWindowContents(proc, w) \
	(Handle)(CallUniversalProc(proc, GetWindowContentsUPPInfo, (w)))
	
#define CallGetSelection(proc, selStart, selEnd, firstChar) \
	(CallUniversalProc(proc, GetSelectionUPPInfo, (selStart), (selEnd), (firstChar)))
	
#define CallSetSelection(proc, selStart, selEnd, firstChar) \
	(CallUniversalProc(proc, SetSelectionUPPInfo, (selStart), (selEnd), (firstChar)))
	
#define CallGetDocInfo(proc, w, name, vRefNum, dirID) \
	(CallUniversalProc(proc, GetDocInfoUPPInfo, (w), (name), (vRefNum), (dirID)))
	
#define CallGetModDate(proc, w) \
	(CallUniversalProc(proc, GetModDateUPPInfo, (w)))
	
#define CallCopy(proc) \
	(Handle)(CallUniversalProc(proc, CopyUPPInfo))
	
#define CallPaste(proc, h) \
	(CallUniversalProc(proc, PasteUPPInfo, (h)))
	
#define CallGetLastLine(proc) \
	(CallUniversalProc(proc, GetLastLineUPPInfo))
	
#define CallGetLineNumber(proc, sel) \
	(CallUniversalProc(proc, GetLineNumberUPPInfo, (sel)))
	
#define CallGetLineStart(proc, sel) \
	(CallUniversalProc(proc, GetLineStartUPPInfo, (sel)))

#define CallGetLineEnd(proc, sel) \
	(CallUniversalProc(proc, GetLineEndUPPInfo, (sel)))

#define CallGetLinePos(proc, sel) \
	(CallUniversalProc(proc, GetLinePosUPPInfo, (sel)))

#define CallInsert(proc, text, len) \
	(CallUniversalProc(proc, InsertUPPInfo, (text), (len)))
	
#define CallDelete(proc) \
	(CallUniversalProc(proc, DeleteUPPInfo))
	
#define CallSetWindowContents(proc, w, h) \
	(CallUniversalProc(proc, SetWindowContentsUPPInfo, (w), (h)))
	
#define CallContentsChanged(proc, w) \
	(CallUniversalProc(proc, ContentsChangedUPPInfo, (w)))
	
#define CallGetFileText(proc, vRefNum, dirID, name, canDispose) \
	(Handle)(CallUniversalProc(proc, GetFileTextUPPInfo, (vRefNum), (dirID), (name), (canDispose)))
	
#define CallGetFolder(proc, prompt, vRefNum, dirID) \
	(Boolean)(CallUniversalProc(proc, GetFolderUPPInfo, (prompt), (vRefNum), (dirID)))
	
#define CallOpenSeveral(proc, sort, file_count, files) \
	(Boolean)(CallUniversalProc(proc, OpenSeveralUPPInfo, (sort), (file_count), (files)))
	
#define CallCenterDialog(proc, dialogID) \
	(DialogPtr)(CallUniversalProc(proc, CenterDialogUPPInfo, (dialogID)))
	
#define CallStandardFilter(proc, d, event, item) \
	CallModalFilterProc(proc, (d), (event), (item))

#define CallFrameDialogItem(proc, d, item) \
	CallUserItemProc(proc, (d), (item))
	
#define CallNewDocument(proc) \
	(WindowPtr)(CallUniversalProc(proc, NewDocumentUPPInfo))
	
#define CallOpenDocument(proc) \
	(WindowPtr)(CallUniversalProc(proc, OpenDocumentUPPInfo))
	
#define CallAllocate(proc, size, clear) \
	(Handle)(CallUniversalProc(proc, AllocateUPPInfo, (size), (clear)))
	
#define CallFindPattern(proc, text, text_len, text_offset, pat, pat_len, case_sens) \
	(CallUniversalProc(proc, FindPatternUPPInfo, (text), (text_len), (text_offset), \
						(pat), (pat_len), (case_sens)))
						
#define CallReportOSError(proc, code) \
	(CallUniversalProc(proc, ReportOSErrorUPPInfo, (code)))
	
#define CallGetPreference(proc, prefType, req_len, buffer, act_len) \
	(CallUniversalProc(proc, GetPreferenceUPPInfo, (prefType), (req_len), (buffer), (act_len)))
	
#define CallSetPreference(proc, prefType, req_len, buffer, act_len) \
	(CallUniversalProc(proc, SetPreferenceUPPInfo, (prefType), (req_len), (buffer), (act_len)))

#define CallStartProgress(proc, str, total, cancel_allowed) \
	(CallUniversalProc(proc, StartProgressUPPInfo, (str), (total), (cancel_allowed)))
	
#define CallDoProgress(proc, done) \
	(Boolean)(CallUniversalProc(proc, DoProgressUPPInfo, (done)))
	
#define CallDoneProgress(proc) \
	(CallUniversalProc(proc, DoneProgressUPPInfo))
	
#define CallGetProjectList(proc, spec, kind, count, entries) \
	(Boolean)(CallUniversalProc(proc, GetProjectListUPPInfo, (spec), (kind), (count), (entries)))
	
#define CallProjectTextList(proc, spec, text) \
	(Boolean)(CallUniversalProc(proc, ProjectTextListUPPInfo, (spec), (text)))
	
#define CallPresetUndo(proc) \
	(Boolean)(CallUniversalProc(proc, PresetUndoUPPInfo))
	
#define CallSetUndo(proc) \
	(CallUniversalProc(proc, SetUndoUPPInfo))
	
#define CallOpenFile(proc, spec, w) \
	(Boolean)(CallUniversalProc(proc, OpenFileUPPInfo, (spec), (w)))

#define CallPrepareUndo(proc, undo_start, undo_end, sel_start, sel_end) \
	(Boolean)(CallUniversalProc(proc, PrepareUndoUPPInfo, (undo_start), (undo_end), \
								(sel_start), (sel_end)))
								
#define CallCommitUndo(proc, new_end) \
	(CallUniversalProc(proc, CommitUndoUPPInfo, (new_end)))

#define CallCreateResults(proc, title, count, results, w) \
	(Boolean)(CallUniversalProc(proc, CreateResultsUPPInfo, (title), (count), (results), (w)))
	
#else

typedef pascal	Handle	(*GetWindowContentsProc)(WindowPtr w);
typedef	pascal	void	(*GetSelectionProc)(long *selStart, long *selEnd, long *firstChar);
typedef	pascal	void	(*SetSelectionProc)(long selStart, long selEnd, long firstChar);
typedef	pascal	void	(*GetDocInfoProc)(WindowPtr w, Str255 fName, short *vRefNum, long *dirID);
typedef	pascal	long	(*GetModDateProc)(WindowPtr w);
typedef	pascal	Handle	(*CopyProc)(void);
typedef	pascal	void	(*PasteProc)(Handle pasteText);
	
typedef	pascal	long	(*GetLastLineProc)(void);
typedef	pascal	long	(*GetLineNumberProc)(long selection);
typedef	pascal	long	(*GetLineStartProc)(long selection);
typedef	pascal	long	(*GetLineEndProc)(long selection);
typedef	pascal	long	(*GetLinePosProc)(long line);
	
typedef	pascal	void	(*InsertProc)(char *text, long len);
typedef	pascal	void	(*DeleteProc)(void);
	
typedef	pascal	void	(*SetWindowContentsProc)(WindowPtr w, Handle h);
typedef	pascal	void	(*ContentsChangedProc)(WindowPtr w);
	
typedef	pascal	Handle		(*GetFileTextProc)(short vRefNum, long dirID, Str255 fName, Boolean *canDispose);

typedef	pascal	Boolean		(*GetFolderProc)(Str255 prompt, short *vRefNum, long *dirID);
typedef	pascal	Boolean		(*OpenSeveralProc)(Boolean sort, short *file_count, StandardFileReply ***files);
	
typedef	pascal	DialogPtr	(*CenterDialogProc)(short dialogID);
typedef	pascal	Boolean		(*StandardFilterProc)(DialogPtr d, EventRecord *event, short *item);
typedef	pascal	void		(*FrameDialogItemProc)(DialogPtr d, short item);
	
typedef	pascal	WindowPtr	(*NewDocumentProc)(void);
typedef	pascal	WindowPtr	(*OpenDocumentProc)(void);

typedef	pascal	Handle		(*AllocateProc)(long size, Boolean clear);
typedef	pascal	long		(*FindPatternProc)(char *text, long text_len, long text_offset, 
										char *pat, long pat_len,
										Boolean case_sensitive);
	
typedef	pascal	void		(*ReportOSErrorProc)(short code);
	
typedef	pascal	void		(*GetPreferenceProc)(ResType prefType, short req_len, void *buffer, short *act_len);
typedef	pascal	void		(*SetPreferenceProc)(ResType prefType, short req_len, void *buffer, short *act_len);

typedef	pascal	void		(*StartProgressProc)(Str255 str, long total, Boolean cancel_allowed);
typedef	pascal	Boolean		(*DoProgressProc)(long done);
typedef	pascal	void		(*DoneProgressProc)(void);
	
typedef	pascal	Boolean		(*GetProjectListProc)(FSSpec *spec, short *kind, short *count, ProjectEntry ***entries);
typedef	pascal	Boolean		(*ProjectTextListProc)(FSSpec *spec, Handle *text);
		
typedef	pascal	Boolean		(*PresetUndoProc)(void);
typedef	pascal	void		(*SetUndoProc)(void);
	
typedef	pascal	Boolean		(*OpenFileProc)(FSSpec *spec, WindowPtr *w);

typedef pascal	Boolean		(*PrepareUndoProc)(long undo_start, long undo_end,
												long sel_start, long sel_end);
typedef pascal	void		(*CommitUndoProc)(long new_end);

typedef pascal	Boolean		(*CreateResultsProc)(Str255 title, short count, Handle results, WindowPtr *w);

#define CallGetWindowContents(proc, w) \
	((proc))((w))
	
#define CallGetSelection(proc, selStart, selEnd, firstChar) \
	((proc))((selStart), (selEnd), (firstChar))
	
#define CallSetSelection(proc, selStart, selEnd, firstChar) \
	((proc))((selStart), (selEnd), (firstChar))
	
#define CallGetDocInfo(proc, w, name, vRefNum, dirID) \
	((proc))((w), (name), (vRefNum), (dirID))
	
#define CallGetModDate(proc, w) \
	((proc))((w))
	
#define CallCopy(proc) \
	((proc))()
	
#define CallPaste(proc, h) \
	((proc))((h))
	
#define CallGetLastLine(proc) \
	((proc))()
	
#define CallGetLineNumber(proc, sel) \
	((proc))((sel))
	
#define CallGetLineStart(proc, sel) \
	((proc))((sel))
	
#define CallGetLineEnd(proc, sel) \
	((proc))((sel))

#define CallGetLinePos(proc, sel) \
	((proc))((sel))

#define CallInsert(proc, text, len) \
	((proc))((text), (len))
	
#define CallDelete(proc) \
	((proc))()
	
#define CallSetWindowContents(proc, w, h) \
	((proc))((w), (h))
	
#define CallContentsChanged(proc, w) \
	((proc))((w))
	
#define CallGetFileText(proc, vRefNum, dirID, name, canDispose) \
	((proc))((vRefNum), (dirID), (name), (canDispose))
	
#define CallGetFolder(proc, prompt, vRefNum, dirID) \
	((proc))((prompt), (vRefNum), (dirID))
	
#define CallOpenSeveral(proc, sort, file_count, files) \
	((proc))((sort), (file_count), (files))
	
#define CallCenterDialog(proc, dialogID) \
	((proc))((dialogID))
	
#define CallStandardFilter(proc, d, event, item) \
	((proc))((d), (event), (item))

#define CallFrameDialogItem(proc, d, item) \
	((proc))((d), (item))
	
#define CallNewDocument(proc) \
	((proc))()
	
#define CallOpenDocument(proc) \
	((proc))()
	
#define CallAllocate(proc, size, clear) \
	((proc))((size), (clear))
	
#define CallFindPattern(proc, text, text_len, text_offset, pat, pat_len, case_sens) \
	((proc))((text), (text_len), (text_offset), (pat), (pat_len), (case_sens))
						
#define CallReportOSError(proc, code) \
	((proc))((code))
	
#define CallGetPreference(proc, prefType, req_len, buffer, act_len) \
	((proc))((prefType), (req_len), (buffer), (act_len))
	
#define CallSetPreference(proc, prefType, req_len, buffer, act_len) \
	((proc))((prefType), (req_len), (buffer), (act_len))

#define CallStartProgress(proc, str, total, cancel_allowed) \
	((proc))((str), (total), (cancel_allowed))
	
#define CallDoProgress(proc, done) \
	((proc))((done))
	
#define CallDoneProgress(proc) \
	((proc))()
	
#define CallGetProjectList(proc, spec, kind, count, entries) \
	((proc))((spec), (kind), (count), (entries))
	
#define CallProjectTextList(proc, spec, text) \
	((proc))((spec), (text))
	
#define CallPresetUndo(proc) \
	((proc))()
	
#define CallSetUndo(proc) \
	((proc))()
	
#define CallOpenFile(proc, spec, w) \
	((proc))((spec), (w))

#define CallPrepareUndo(proc, undo_start, undo_end, sel_start, sel_end) \
	((proc))((undo_start), (undo_end), (sel_start), (sel_end))
	
#define CallCommitUndo(proc, new_end) \
	((proc))((new_end))

#define CallCreateResults(proc, title, count, results, w) \
	((proc))((title), (count), (results), (w))
	
#endif


typedef struct
{
	short	version;
	
	//	version 1 callbacks
	
	GetWindowContentsProc	GetWindowContents;
	GetSelectionProc	GetSelection;
	SetSelectionProc	SetSelection;
	GetDocInfoProc	GetDocInfo;
	GetModDateProc	GetModDate;
	CopyProc	Copy;
	PasteProc	Paste;
	
	//	version 2 callbacks
	
	/*	Text-Editing stuff */
	GetLastLineProc	GetLastLine;
	GetLineNumberProc	GetLineNumber;
	GetLineStartProc	GetLineStart;
	GetLineEndProc	GetLineEnd;
	GetLinePosProc	GetLinePos;
	
	InsertProc	Insert;
	DeleteProc	Delete;
	
	/*	Getting and Setting window text */
	SetWindowContentsProc	SetWindowContents;
	ContentsChangedProc	ContentsChanged;
	
	/*	Reading file text */
	GetFileTextProc	GetFileText;

	/*	Direct user-interface calls */
	GetFolderProc	GetFolder;
	OpenSeveralProc	OpenSeveral;
	
	CenterDialogProc	CenterDialog;
	StandardFilterProc	StandardFilter;
	FrameDialogItemProc	FrameDialogItem;
	
	NewDocumentProc	NewDocument;
	OpenDocumentProc	OpenDocument;

	/*	Utility Routines */
	AllocateProc	Allocate;
	FindPatternProc	FindPattern;
	
	ReportOSErrorProc	ReportOSError;
	
	/*	Preference routines */
	GetPreferenceProc	GetPreference;
	SetPreferenceProc	SetPreference;

	/*	Progress routines */
	StartProgressProc	StartProgress;
	DoProgressProc	DoProgress;
	DoneProgressProc	DoneProgress;
	
	//	Version 3 callbacks
	GetProjectListProc	GetProjectList;
	ProjectTextListProc	ProjectTextList;
	
	//	version 4 callbacks
	
	PresetUndoProc	PresetUndo;
	SetUndoProc	SetUndo;
	
	OpenFileProc	OpenFile;
	
	//	version 5 callbacks
	
	PrepareUndoProc	PrepareUndo;
	CommitUndoProc	CommitUndo;
	
	CreateResultsProc	CreateResults;
	
} ExternalCallbackBlock;

#if defined(powerc) || defined (__powerc)
#pragma options align=reset
#endif

/*	
	'main' for a BBXT is declared:
	
pascal void main(ExternalCallbackBlock *callbacks, WindowPtr w);		[C]

	The 'new' calling convention, which passes more information
	and allows scriptability, is this:
	
pascal OSErr main(ExternalCallbackBlock *callbacks, WindowPtr w, long flags, AppleEvent *event, AppleEvent *reply);
*/
