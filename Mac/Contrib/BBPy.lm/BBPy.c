#include <AEDataModel.h>

#define DEBUG 0

#define kComponentSignatureString "BBPy.LM"                 
#include <Debugging.h>


#include <BBLMInterface.h>
#include <BBXTInterface.h>
//#include <BBLMTextIterator.h>

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <Sound.h>

#if DEBUG
void debugf_(const char* func,const char* fileName,long line, const char*fmt,...)
{
	va_list arg;
	char msg[256];
	va_start(arg, fmt);
	vsnprintf(msg,256 ,fmt, arg);
    DebugAssert(COMPONENT_SIGNATURE, DEBUG_NO_OPTIONS, kComponentSignatureString ": " , msg, nil, fileName, line, 0 );

	//debug_string(msg);
}
#define debugf(FMT,...)  debugf_( __FUNCTION__,__FILE__, __LINE__,FMT,__VA_ARGS__);
#else
#define debugf(FMT,...) 
#endif

typedef const char *Str; 


enum{
	kPyBBLMStringSubst =  kBBLMFirstUserRunKind
};

#define iswordchar(x) (isalnum(x)||x=='_')


struct runloc{
	bool past_gap;
	long pos;
	long last_start;
	unsigned char*p;
};

char start(struct runloc& r,BBLMParamBlock &pb)
{
	r.past_gap = false;
	r.last_start = pb.fCalcRunParams.fStartOffset;
	r.pos = pb.fCalcRunParams.fStartOffset;
	r.p = ((unsigned char*)pb.fText) + pb.fCalcRunParams.fStartOffset;
	// Adjust for the gap if weÕre not already past it.
	if ((!r.past_gap) && (r.pos >= pb.fTextGapLocation)){
		r.p += pb.fTextGapLength;
		r.past_gap = true;
	}
	return *r.p;

}

char nextchar(struct runloc&r,BBLMParamBlock &pb)
{
	if ( r.pos< pb.fTextLength){
		++r.pos;
		++r.p;
		if ((!r.past_gap) && (r.pos >= pb.fTextGapLocation)){
			r.p += pb.fTextGapLength;
			r.past_gap = true;
		}
		return *r.p;
	}
	else{
		return 0;
	}
}

bool addRun(BBLMRunCode kind, int  start,int len , const BBLMCallbackBlock& bblm_callbacks)
{
	if (len > 0){ // Tie off the code run we were in, unless the length is zero.
		debugf("Run %d %d:%d", kind, start, start+len-1 );
		return bblmAddRun(	&bblm_callbacks, 'Pyth',
							kind, start, len, false);
							
	}
	else{
		return true;
	}
}					

bool addRunBefore (BBLMRunCode kind,struct runloc& r, const BBLMCallbackBlock& bblm_callbacks)
{
	bool more_runs = addRun(kind, r.last_start, r.pos - r.last_start, bblm_callbacks);
	r.last_start =  r.pos;
	return more_runs;
}

bool addRunTo (BBLMRunCode kind, struct runloc& r, const BBLMCallbackBlock& bblm_callbacks)
{
	bool more_runs = addRun(kind, r.last_start, r.pos - r.last_start+1, bblm_callbacks);
	r.last_start =  r.pos+1;
	return more_runs;
}


bool colorstr(	char delim,
				BBLMParamBlock &pb,
				struct runloc &r,
				const BBLMCallbackBlock &bblm_callbacks)
{
	bool tripple = false , pers = false, lookup = false, more_runs = true;
	char c = nextchar(r,pb);

	if (c == delim){
		c = nextchar(r,pb);
		if (c == delim){
			tripple = true;
			c = nextchar(r,pb);
		}  
		else{
			//double
			return addRunBefore(kBBLMRunIsSingleString,r,bblm_callbacks);
		}	
	}
	while (c && more_runs){
		if (pers ){
			if (isalpha(c)){
				more_runs = addRunTo(kPyBBLMStringSubst,r,bblm_callbacks);
			}
			else if (c == '('){
				lookup = true;
			}
		}
		pers = false;
		if (c == delim){
			if (tripple){
				if ((c = nextchar(r,pb))== delim && (c = nextchar(r,pb)) == delim){
					break; // end of tripple-quote.
				}  
			}
			else{
				break; // end of single-quote.
			}
			
		}
		else if (c== '\\'){
			nextchar(r,pb);
		}
		else if (c=='\r'||c=='\n'){
			if (!tripple){	
				break;
			}
		}
		else if (c=='%'){
			more_runs = addRunBefore(kBBLMRunIsSingleString,r,bblm_callbacks);
			pers = true;
		}
		else if (c==')' && lookup){
			more_runs = addRunTo(kPyBBLMStringSubst,r,bblm_callbacks);
			lookup = false;
		}
		c = nextchar(r,pb);
	}
	return more_runs && addRunTo(lookup?kPyBBLMStringSubst:kBBLMRunIsSingleString,r,bblm_callbacks);
}

bool colorcomment(BBLMParamBlock &pb,
				struct runloc &r,
				const BBLMCallbackBlock &bblm_callbacks)
{
	while (char c = nextchar(r,pb)){
		if (c=='\r'|| c=='\n'){
			break;
		}
	}
	return addRunTo(kBBLMRunIsLineComment,r,bblm_callbacks);
}

void CalculateRuns(BBLMParamBlock &pb,
			const BBLMCallbackBlock &bblm_callbacks)

{
	const struct rundesc	*state = NULL;
	bool more_runs=true;
		
	struct runloc r;
	
	char c = start(r,pb);
	
	while (c && more_runs){
	loop:
		// Process a char
		if (state==NULL){
			//If we're in the basic 'code' state, check for each interesting char (rundelims[i].start).
			switch (c){
			case '\'':
			case '"': 
				more_runs = addRunBefore(kBBLMRunIsCode,r,bblm_callbacks);
				if (more_runs){
					more_runs = colorstr(c,pb,r,bblm_callbacks);
				}
				break;
			case '#' :
				more_runs = addRunBefore(kBBLMRunIsCode,r,bblm_callbacks);
				if (more_runs){
					more_runs = colorcomment(pb,r,bblm_callbacks); 
				}
				break;
			default:
				break;
			}

		}
		c = nextchar(r,pb);
	}
	if (more_runs){
		addRunBefore(kBBLMRunIsCode,r,bblm_callbacks);
	}
	

}
static void AdjustRange(BBLMParamBlock &params,
						const BBLMCallbackBlock &callbacks)
{	
	DescType language;
	BBLMRunCode kind;
	SInt32 charPos;
	SInt32 length;
	UInt32 index = params.fAdjustRangeParams.fStartIndex;
	
	while(	index > 0 &&
			bblmGetRun(&callbacks, index, language, kind, charPos, length) &&
		 	(kind==kPyBBLMStringSubst||kind==kBBLMRunIsSingleString)){
	 	index--;
	};
	 params.fAdjustRangeParams.fStartIndex = index;
}


// The next couple funcs process the text of a file assumming it's in 1 piece in memory,
// so they may not be called from CalculateRuns.

bool matchword(BBLMParamBlock &pb, const char *pat ,unsigned long *pos)
{	
	const char *asciText = (const char *) (pb.fTextIsUnicode?NULL:pb.fText);

	int i;
	for (i=0; pat[i]; i++){
		if (*pos+i>=pb.fTextLength){
			return false;
		}
		if (asciText[*pos+i] != pat[i]){
			return false;
		}
	}
	if ((*pos+i<pb.fTextLength)&&iswordchar(asciText[*pos+i])){
		return false;
	}
	*pos+=i;
	return true;
}	

int matchindent(BBLMParamBlock &pb, UInt32 *pos)
{	
	const char *asciText = (const char *) (pb.fTextIsUnicode?NULL:pb.fText);
	int indent=0;
		
	while(*pos<pb.fTextLength){
		switch (/*(char)(pb.fTextIsUnicode?uniText[pos]:*/asciText[*pos]/*)*/){
		case ' ':
			++*pos;
			indent++;
			break;	
		case '\t':
			++*pos;		
			indent+=8;
			break;
		case '#':
			return -1;
			break;
		default:
			return indent;
			break;
		}
	}	
}


void eat_line(BBLMParamBlock &pb, unsigned long* pos)
{
	const char *asciText = (const char *) (pb.fTextIsUnicode?NULL:pb.fText);
	while (asciText[*pos]!='\r' && asciText[*pos]!='\n' && *pos<pb.fTextLength) {++*pos;}
	while ((asciText[*pos]=='\r' || asciText[*pos]=='\n') && *pos<pb.fTextLength) {++*pos;}

}

void addItem(BBLMParamBlock &pb, UInt32 pos, int nest, BBLMFunctionKinds kind,
			const BBLMCallbackBlock *bblm_callbacks)
{
	UInt32 funcstartpos = pos;
	UInt32 funcnamelen=0;
	UInt32 offset=0;
	const char *asciText = (const char *) pb.fText;
	UInt32 index;
	OSErr err;
	
	while (isspace(asciText[pos]) && pos<pb.fTextLength) {++pos;}
	UInt32 fnamestart = pos;
	while ((isalnum(asciText[pos])||asciText[pos]=='_') && pos<pb.fTextLength) {pos++; funcnamelen++;}
	
	err = bblmAddTokenToBuffer(	bblm_callbacks, 
								pb.fFcnParams.fTokenBuffer,
								(void*)&asciText[fnamestart],
								funcnamelen,
								pb.fTextIsUnicode,
								&offset);
	BBLMProcInfo procInfo; 
	procInfo.fFunctionStart = fnamestart;	//	char offset in file of first character of function
	procInfo.fFunctionEnd = pos;	//	char offset of last character of function
	
	procInfo.fSelStart = fnamestart;		//	first character to select when choosing function
	procInfo.fSelEnd = pos;		//	last character to select when choosing function
	
	procInfo.fFirstChar = fnamestart;		//	first character to make visible when choosing function
	
	procInfo.fKind = kind;
	
	procInfo.fIndentLevel = nest;	//	indentation level of token
	procInfo.fFlags = 0;			//	token flags (see BBLMFunctionFlags)
	procInfo.fNameStart = offset;		//	char offset in token buffer of token name
	procInfo.fNameLength = funcnamelen;	//	length of token name
																
	err = bblmAddFunctionToList(bblm_callbacks,	
								pb.fFcnParams.fFcnList,
								procInfo,
								&index);
}



enum{
	maxnest=5
};

void ScanForFunctions(BBLMParamBlock &pb,
			const BBLMCallbackBlock &bblm_callbacks)
{

	const char *asciText = (const char *) (pb.fTextIsUnicode?NULL:pb.fText);
	UniCharPtr uniText = (UniCharPtr) (pb.fTextIsUnicode?pb.fText:NULL);
	
	int indents[maxnest]= {0};
	int nest = 0;
	
	UInt32 pos=0; // current character offset

	
	while (pos<pb.fTextLength){
		
		int indent = matchindent(pb, &pos);
		
		if (indent >= 0){
			for (int i=0; i <= nest; i++){
				if (indent<=indents[i]){
					nest = i;
					indents[nest]=indent;
					goto x;
				}
			}
			indents[++nest]=indent;
			x:
		
			if (matchword(pb,"def",&pos)){
				addItem( pb, pos, nest, kBBLMFunctionMark, &bblm_callbacks);
			}
			else if (matchword(pb, "class", &pos)){
				addItem( pb, pos, nest, kBBLMTypedef, &bblm_callbacks);
			}
		}
		eat_line(pb,&pos);
	}
	
}

OSErr main(	BBLMParamBlock &params,
			const BBLMCallbackBlock &bblm_callbacks,
			const BBXTCallbackBlock &bbxt_callbacks)
{
	OSErr result;

	if ((params.fSignature != kBBLMParamBlockSignature) ||
		(params.fLength < sizeof(BBLMParamBlock)))
	{
		return paramErr;
	}
	
	switch (params.fMessage)
	{
		case kBBLMInitMessage:
		case kBBLMDisposeMessage:
		{
			result = noErr;	// nothing to do
			break;
		}
		
		case kBBLMCalculateRunsMessage:
			CalculateRuns(params, bblm_callbacks);
			result = noErr;
			break;

		case kBBLMScanForFunctionsMessage:
			ScanForFunctions(params, bblm_callbacks);
			result = noErr;
			break;

		case kBBLMAdjustRangeMessage:
			AdjustRange(params, bblm_callbacks);
			result = noErr;
			break;
		
		case kBBLMMapRunKindToColorCodeMessage:
			switch (params.fMapRunParams.fRunKind){
			case kPyBBLMStringSubst:
				params.fMapRunParams.fColorCode = kBBLMSGMLAttributeNameColor;
				params.fMapRunParams.fMapped =	true;
				break;	
			default:
				params.fMapRunParams.fMapped =	false;
			}
			result = noErr;
			break;

		case kBBLMEscapeStringMessage:
		case kBBLMAdjustEndMessage:
		case kBBLMMapColorCodeToColorMessage:
		case kBBLMSetCategoriesMessage:
		case kBBLMMatchKeywordMessage:
		{
			result = userCanceledErr;
			break;
		}
			
		default:
		{
			result = paramErr;
			break;
		}
	}
	return result;	
}