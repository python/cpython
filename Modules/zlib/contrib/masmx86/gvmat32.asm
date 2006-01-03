; gvmat32.asm -- Asm portion of the optimized longest_match for 32 bits x86
; Copyright (C) 1995-1996 Jean-loup Gailly and Gilles Vollant.
; File written by Gilles Vollant, by modifiying the longest_match
;  from Jean-loup Gailly in deflate.c
;
;         http://www.zlib.net
;         http://www.winimage.com/zLibDll
;         http://www.muppetlabs.com/~breadbox/software/assembly.html
;
; For Visual C++ 4.x and higher and ML 6.x and higher
;   ml.exe is in directory \MASM611C of Win95 DDK
;   ml.exe is also distributed in http://www.masm32.com/masmdl.htm
;    and in VC++2003 toolkit at http://msdn.microsoft.com/visualc/vctoolkit2003/
;
; this file contain two implementation of longest_match
;
;  longest_match_7fff : written 1996 by Gilles Vollant optimized for 
;            first Pentium. Assume s->w_mask == 0x7fff
;  longest_match_686 : written by Brian raiter (1998), optimized for Pentium Pro
;
;  for using an seembly version of longest_match, you need define ASMV in project
;  There is two way in using gvmat32.asm
;
;  A) Suggested method
;    if you want include both longest_match_7fff and longest_match_686
;    compile the asm file running
;           ml /coff /Zi /Flgvmat32.lst /c gvmat32.asm
;    and include gvmat32c.c in your project
;    if you have an old cpu (386,486 or first Pentium) and s->w_mask==0x7fff,
;        longest_match_7fff will be used
;    if you have a more modern CPU (Pentium Pro, II and higher)
;        longest_match_686 will be used
;    on old cpu with s->w_mask!=0x7fff, longest_match_686 will be used,
;        but this is not a sitation you'll find often
;
;  B) Alternative
;    if you are not interresed in old cpu performance and want the smaller
;       binaries possible
;
;    compile the asm file running
;           ml /coff /Zi /c /Flgvmat32.lst /DNOOLDPENTIUMCODE gvmat32.asm
;    and do not include gvmat32c.c in your project (ou define also 
;              NOOLDPENTIUMCODE)
;
; note : as I known, longest_match_686 is very faster than longest_match_7fff
;        on pentium Pro/II/III, faster (but less) in P4, but it seem
;        longest_match_7fff can be faster (very very litte) on AMD Athlon64/K8
;
; see below : zlib1222add must be adjuster if you use a zlib version < 1.2.2.2

;uInt longest_match_7fff(s, cur_match)
;    deflate_state *s;
;    IPos cur_match;                             /* current match */

    NbStack         equ     76
    cur_match       equ     dword ptr[esp+NbStack-0]
    str_s           equ     dword ptr[esp+NbStack-4]
; 5 dword on top (ret,ebp,esi,edi,ebx)
    adrret          equ     dword ptr[esp+NbStack-8]
    pushebp         equ     dword ptr[esp+NbStack-12]
    pushedi         equ     dword ptr[esp+NbStack-16]
    pushesi         equ     dword ptr[esp+NbStack-20]
    pushebx         equ     dword ptr[esp+NbStack-24]

    chain_length    equ     dword ptr [esp+NbStack-28]
    limit           equ     dword ptr [esp+NbStack-32]
    best_len        equ     dword ptr [esp+NbStack-36]
    window          equ     dword ptr [esp+NbStack-40]
    prev            equ     dword ptr [esp+NbStack-44]
    scan_start      equ      word ptr [esp+NbStack-48]
    wmask           equ     dword ptr [esp+NbStack-52]
    match_start_ptr equ     dword ptr [esp+NbStack-56]
    nice_match      equ     dword ptr [esp+NbStack-60]
    scan            equ     dword ptr [esp+NbStack-64]

    windowlen       equ     dword ptr [esp+NbStack-68]
    match_start     equ     dword ptr [esp+NbStack-72]
    strend          equ     dword ptr [esp+NbStack-76]
    NbStackAdd      equ     (NbStack-24)

    .386p

    name    gvmatch
    .MODEL  FLAT



;  all the +zlib1222add offsets are due to the addition of fields
;  in zlib in the deflate_state structure since the asm code was first written
;  (if you compile with zlib 1.0.4 or older, use "zlib1222add equ (-4)").
;  (if you compile with zlib between 1.0.5 and 1.2.2.1, use "zlib1222add equ 0").
;  if you compile with zlib 1.2.2.2 or later , use "zlib1222add equ 8").

    zlib1222add         equ     8

;  Note : these value are good with a 8 bytes boundary pack structure
    dep_chain_length    equ     74h+zlib1222add
    dep_window          equ     30h+zlib1222add
    dep_strstart        equ     64h+zlib1222add
    dep_prev_length     equ     70h+zlib1222add
    dep_nice_match      equ     88h+zlib1222add
    dep_w_size          equ     24h+zlib1222add
    dep_prev            equ     38h+zlib1222add
    dep_w_mask          equ     2ch+zlib1222add
    dep_good_match      equ     84h+zlib1222add
    dep_match_start     equ     68h+zlib1222add
    dep_lookahead       equ     6ch+zlib1222add


_TEXT                   segment

IFDEF NOUNDERLINE
   IFDEF NOOLDPENTIUMCODE
            public  longest_match
            public  match_init
   ELSE            
            public  longest_match_7fff
            public  cpudetect32
            public  longest_match_686
   ENDIF
ELSE
   IFDEF NOOLDPENTIUMCODE
            public  _longest_match
            public  _match_init
   ELSE
            public  _longest_match_7fff
            public  _cpudetect32
            public  _longest_match_686
   ENDIF
ENDIF

    MAX_MATCH           equ     258
    MIN_MATCH           equ     3
    MIN_LOOKAHEAD       equ     (MAX_MATCH+MIN_MATCH+1)



IFNDEF NOOLDPENTIUMCODE
IFDEF NOUNDERLINE
longest_match_7fff   proc near
ELSE
_longest_match_7fff  proc near
ENDIF

    mov     edx,[esp+4]



    push    ebp
    push    edi
    push    esi
    push    ebx

    sub     esp,NbStackAdd

; initialize or check the variables used in match.asm.
    mov     ebp,edx

; chain_length = s->max_chain_length
; if (prev_length>=good_match) chain_length >>= 2
    mov     edx,[ebp+dep_chain_length]
    mov     ebx,[ebp+dep_prev_length]
    cmp     [ebp+dep_good_match],ebx
    ja      noshr
    shr     edx,2
noshr:
; we increment chain_length because in the asm, the --chain_lenght is in the beginning of the loop
    inc     edx
    mov     edi,[ebp+dep_nice_match]
    mov     chain_length,edx
    mov     eax,[ebp+dep_lookahead]
    cmp     eax,edi
; if ((uInt)nice_match > s->lookahead) nice_match = s->lookahead;
    jae     nolookaheadnicematch
    mov     edi,eax
nolookaheadnicematch:
; best_len = s->prev_length
    mov     best_len,ebx

; window = s->window
    mov     esi,[ebp+dep_window]
    mov     ecx,[ebp+dep_strstart]
    mov     window,esi

    mov     nice_match,edi
; scan = window + strstart
    add     esi,ecx
    mov     scan,esi
; dx = *window
    mov     dx,word ptr [esi]
; bx = *(window+best_len-1)
    mov     bx,word ptr [esi+ebx-1]
    add     esi,MAX_MATCH-1
; scan_start = *scan
    mov     scan_start,dx
; strend = scan + MAX_MATCH-1
    mov     strend,esi
; bx = scan_end = *(window+best_len-1)

;    IPos limit = s->strstart > (IPos)MAX_DIST(s) ?
;        s->strstart - (IPos)MAX_DIST(s) : NIL;

    mov     esi,[ebp+dep_w_size]
    sub     esi,MIN_LOOKAHEAD
; here esi = MAX_DIST(s)
    sub     ecx,esi
    ja      nodist
    xor     ecx,ecx
nodist:
    mov     limit,ecx

; prev = s->prev
    mov     edx,[ebp+dep_prev]
    mov     prev,edx

;
    mov     edx,dword ptr [ebp+dep_match_start]
    mov     bp,scan_start
    mov     eax,cur_match
    mov     match_start,edx

    mov     edx,window
    mov     edi,edx
    add     edi,best_len
    mov     esi,prev
    dec     edi
; windowlen = window + best_len -1
    mov     windowlen,edi

    jmp     beginloop2
    align   4

; here, in the loop
;       eax = ax = cur_match
;       ecx = limit
;        bx = scan_end
;        bp = scan_start
;       edi = windowlen (window + best_len -1)
;       esi = prev


;// here; chain_length <=16
normalbeg0add16:
    add     chain_length,16
    jz      exitloop
normalbeg0:
    cmp     word ptr[edi+eax],bx
    je      normalbeg2noroll
rcontlabnoroll:
; cur_match = prev[cur_match & wmask]
    and     eax,7fffh
    mov     ax,word ptr[esi+eax*2]
; if cur_match > limit, go to exitloop
    cmp     ecx,eax
    jnb     exitloop
; if --chain_length != 0, go to exitloop
    dec     chain_length
    jnz     normalbeg0
    jmp     exitloop

normalbeg2noroll:
; if (scan_start==*(cur_match+window)) goto normalbeg2
    cmp     bp,word ptr[edx+eax]
    jne     rcontlabnoroll
    jmp     normalbeg2

contloop3:
    mov     edi,windowlen

; cur_match = prev[cur_match & wmask]
    and     eax,7fffh
    mov     ax,word ptr[esi+eax*2]
; if cur_match > limit, go to exitloop
    cmp     ecx,eax
jnbexitloopshort1:
    jnb     exitloop
; if --chain_length != 0, go to exitloop


; begin the main loop
beginloop2:
    sub     chain_length,16+1
; if chain_length <=16, don't use the unrolled loop
    jna     normalbeg0add16

do16:
    cmp     word ptr[edi+eax],bx
    je      normalbeg2dc0

maccn   MACRO   lab
    and     eax,7fffh
    mov     ax,word ptr[esi+eax*2]
    cmp     ecx,eax
    jnb     exitloop
    cmp     word ptr[edi+eax],bx
    je      lab
    ENDM

rcontloop0:
    maccn   normalbeg2dc1

rcontloop1:
    maccn   normalbeg2dc2

rcontloop2:
    maccn   normalbeg2dc3

rcontloop3:
    maccn   normalbeg2dc4

rcontloop4:
    maccn   normalbeg2dc5

rcontloop5:
    maccn   normalbeg2dc6

rcontloop6:
    maccn   normalbeg2dc7

rcontloop7:
    maccn   normalbeg2dc8

rcontloop8:
    maccn   normalbeg2dc9

rcontloop9:
    maccn   normalbeg2dc10

rcontloop10:
    maccn   short normalbeg2dc11

rcontloop11:
    maccn   short normalbeg2dc12

rcontloop12:
    maccn   short normalbeg2dc13

rcontloop13:
    maccn   short normalbeg2dc14

rcontloop14:
    maccn   short normalbeg2dc15

rcontloop15:
    and     eax,7fffh
    mov     ax,word ptr[esi+eax*2]
    cmp     ecx,eax
    jnb     exitloop

    sub     chain_length,16
    ja      do16
    jmp     normalbeg0add16

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

normbeg MACRO   rcontlab,valsub
; if we are here, we know that *(match+best_len-1) == scan_end
    cmp     bp,word ptr[edx+eax]
; if (match != scan_start) goto rcontlab
    jne     rcontlab
; calculate the good chain_length, and we'll compare scan and match string
    add     chain_length,16-valsub
    jmp     iseq
    ENDM


normalbeg2dc11:
    normbeg rcontloop11,11

normalbeg2dc12:
    normbeg short rcontloop12,12

normalbeg2dc13:
    normbeg short rcontloop13,13

normalbeg2dc14:
    normbeg short rcontloop14,14

normalbeg2dc15:
    normbeg short rcontloop15,15

normalbeg2dc10:
    normbeg rcontloop10,10

normalbeg2dc9:
    normbeg rcontloop9,9

normalbeg2dc8:
    normbeg rcontloop8,8

normalbeg2dc7:
    normbeg rcontloop7,7

normalbeg2dc6:
    normbeg rcontloop6,6

normalbeg2dc5:
    normbeg rcontloop5,5

normalbeg2dc4:
    normbeg rcontloop4,4

normalbeg2dc3:
    normbeg rcontloop3,3

normalbeg2dc2:
    normbeg rcontloop2,2

normalbeg2dc1:
    normbeg rcontloop1,1

normalbeg2dc0:
    normbeg rcontloop0,0


; we go in normalbeg2 because *(ushf*)(match+best_len-1) == scan_end

normalbeg2:
    mov     edi,window

    cmp     bp,word ptr[edi+eax]
    jne     contloop3                   ; if *(ushf*)match != scan_start, continue

iseq:
; if we are here, we know that *(match+best_len-1) == scan_end
; and (match == scan_start)

    mov     edi,edx
    mov     esi,scan                    ; esi = scan
    add     edi,eax                     ; edi = window + cur_match = match

    mov     edx,[esi+3]                 ; compare manually dword at match+3
    xor     edx,[edi+3]                 ; and scan +3

    jz      begincompare                ; if equal, go to long compare

; we will determine the unmatch byte and calculate len (in esi)
    or      dl,dl
    je      eq1rr
    mov     esi,3
    jmp     trfinval
eq1rr:
    or      dx,dx
    je      eq1

    mov     esi,4
    jmp     trfinval
eq1:
    and     edx,0ffffffh
    jz      eq11
    mov     esi,5
    jmp     trfinval
eq11:
    mov     esi,6
    jmp     trfinval

begincompare:
    ; here we now scan and match begin same
    add     edi,6
    add     esi,6
    mov     ecx,(MAX_MATCH-(2+4))/4     ; scan for at most MAX_MATCH bytes
    repe    cmpsd                       ; loop until mismatch

    je      trfin                       ; go to trfin if not unmatch
; we determine the unmatch byte
    sub     esi,4
    mov     edx,[edi-4]
    xor     edx,[esi]

    or      dl,dl
    jnz     trfin
    inc     esi

    or      dx,dx
    jnz     trfin
    inc     esi

    and     edx,0ffffffh
    jnz     trfin
    inc     esi

trfin:
    sub     esi,scan          ; esi = len
trfinval:
; here we have finised compare, and esi contain len of equal string
    cmp     esi,best_len        ; if len > best_len, go newbestlen
    ja      short newbestlen
; now we restore edx, ecx and esi, for the big loop
    mov     esi,prev
    mov     ecx,limit
    mov     edx,window
    jmp     contloop3

newbestlen:
    mov     best_len,esi        ; len become best_len

    mov     match_start,eax     ; save new position as match_start
    cmp     esi,nice_match      ; if best_len >= nice_match, exit
    jae     exitloop
    mov     ecx,scan
    mov     edx,window          ; restore edx=window
    add     ecx,esi
    add     esi,edx

    dec     esi
    mov     windowlen,esi       ; windowlen = window + best_len-1
    mov     bx,[ecx-1]          ; bx = *(scan+best_len-1) = scan_end

; now we restore ecx and esi, for the big loop :
    mov     esi,prev
    mov     ecx,limit
    jmp     contloop3

exitloop:
; exit : s->match_start=match_start
    mov     ebx,match_start
    mov     ebp,str_s
    mov     ecx,best_len
    mov     dword ptr [ebp+dep_match_start],ebx
    mov     eax,dword ptr [ebp+dep_lookahead]
    cmp     ecx,eax
    ja      minexlo
    mov     eax,ecx
minexlo:
; return min(best_len,s->lookahead)

; restore stack and register ebx,esi,edi,ebp
    add     esp,NbStackAdd

    pop     ebx
    pop     esi
    pop     edi
    pop     ebp
    ret
InfoAuthor:
; please don't remove this string !
; Your are free use gvmat32 in any fre or commercial apps if you don't remove the string in the binary!
    db     0dh,0ah,"GVMat32 optimised assembly code written 1996-98 by Gilles Vollant",0dh,0ah



IFDEF NOUNDERLINE
longest_match_7fff   endp
ELSE
_longest_match_7fff  endp
ENDIF


IFDEF NOUNDERLINE
cpudetect32     proc near
ELSE
_cpudetect32    proc near
ENDIF

    push    ebx

    pushfd                  ; push original EFLAGS
    pop     eax             ; get original EFLAGS
    mov     ecx, eax        ; save original EFLAGS
    xor     eax, 40000h     ; flip AC bit in EFLAGS
    push    eax             ; save new EFLAGS value on stack
    popfd                   ; replace current EFLAGS value
    pushfd                  ; get new EFLAGS
    pop     eax             ; store new EFLAGS in EAX
    xor     eax, ecx        ; can’t toggle AC bit, processor=80386
    jz      end_cpu_is_386  ; jump if 80386 processor
    push    ecx
    popfd                   ; restore AC bit in EFLAGS first

    pushfd
    pushfd
    pop     ecx

    mov     eax, ecx        ; get original EFLAGS
    xor     eax, 200000h    ; flip ID bit in EFLAGS
    push    eax             ; save new EFLAGS value on stack
    popfd                   ; replace current EFLAGS value
    pushfd                  ; get new EFLAGS
    pop     eax             ; store new EFLAGS in EAX
    popfd                   ; restore original EFLAGS
    xor     eax, ecx        ; can’t toggle ID bit,
    je      is_old_486      ; processor=old

    mov     eax,1
    db      0fh,0a2h        ;CPUID

exitcpudetect:
    pop ebx
    ret

end_cpu_is_386:
    mov     eax,0300h
    jmp     exitcpudetect

is_old_486:
    mov     eax,0400h
    jmp     exitcpudetect

IFDEF NOUNDERLINE
cpudetect32     endp
ELSE
_cpudetect32    endp
ENDIF
ENDIF

MAX_MATCH       equ     258
MIN_MATCH       equ     3
MIN_LOOKAHEAD   equ     (MAX_MATCH + MIN_MATCH + 1)
MAX_MATCH_8_     equ     ((MAX_MATCH + 7) AND 0FFF0h)


;;; stack frame offsets

chainlenwmask   equ  esp + 0    ; high word: current chain len
                    ; low word: s->wmask
window      equ  esp + 4    ; local copy of s->window
windowbestlen   equ  esp + 8    ; s->window + bestlen
scanstart   equ  esp + 16   ; first two bytes of string
scanend     equ  esp + 12   ; last two bytes of string
scanalign   equ  esp + 20   ; dword-misalignment of string
nicematch   equ  esp + 24   ; a good enough match size
bestlen     equ  esp + 28   ; size of best match so far
scan        equ  esp + 32   ; ptr to string wanting match

LocalVarsSize   equ 36
;   saved ebx   byte esp + 36
;   saved edi   byte esp + 40
;   saved esi   byte esp + 44
;   saved ebp   byte esp + 48
;   return address  byte esp + 52
deflatestate    equ  esp + 56   ; the function arguments
curmatch    equ  esp + 60

;;; Offsets for fields in the deflate_state structure. These numbers
;;; are calculated from the definition of deflate_state, with the
;;; assumption that the compiler will dword-align the fields. (Thus,
;;; changing the definition of deflate_state could easily cause this
;;; program to crash horribly, without so much as a warning at
;;; compile time. Sigh.)

dsWSize     equ 36+zlib1222add
dsWMask     equ 44+zlib1222add
dsWindow    equ 48+zlib1222add
dsPrev      equ 56+zlib1222add
dsMatchLen  equ 88+zlib1222add
dsPrevMatch equ 92+zlib1222add
dsStrStart  equ 100+zlib1222add
dsMatchStart    equ 104+zlib1222add
dsLookahead equ 108+zlib1222add
dsPrevLen   equ 112+zlib1222add
dsMaxChainLen   equ 116+zlib1222add
dsGoodMatch equ 132+zlib1222add
dsNiceMatch equ 136+zlib1222add


;;; match.asm -- Pentium-Pro-optimized version of longest_match()
;;; Written for zlib 1.1.2
;;; Copyright (C) 1998 Brian Raiter <breadbox@muppetlabs.com>
;;; You can look at http://www.muppetlabs.com/~breadbox/software/assembly.html
;;;
;;; This is free software; you can redistribute it and/or modify it
;;; under the terms of the GNU General Public License.

;GLOBAL _longest_match, _match_init


;SECTION    .text

;;; uInt longest_match(deflate_state *deflatestate, IPos curmatch)

;_longest_match:
IFDEF NOOLDPENTIUMCODE
    IFDEF NOUNDERLINE
    longest_match       proc near
    ELSE
    _longest_match      proc near
    ENDIF
ELSE
    IFDEF NOUNDERLINE
    longest_match_686   proc near
    ELSE
    _longest_match_686  proc near
    ENDIF
ENDIF

;;; Save registers that the compiler may be using, and adjust esp to
;;; make room for our stack frame.

        push    ebp
        push    edi
        push    esi
        push    ebx
        sub esp, LocalVarsSize

;;; Retrieve the function arguments. ecx will hold cur_match
;;; throughout the entire function. edx will hold the pointer to the
;;; deflate_state structure during the function's setup (before
;;; entering the main loop.

        mov edx, [deflatestate]
        mov ecx, [curmatch]

;;; uInt wmask = s->w_mask;
;;; unsigned chain_length = s->max_chain_length;
;;; if (s->prev_length >= s->good_match) {
;;;     chain_length >>= 2;
;;; }

        mov eax, [edx + dsPrevLen]
        mov ebx, [edx + dsGoodMatch]
        cmp eax, ebx
        mov eax, [edx + dsWMask]
        mov ebx, [edx + dsMaxChainLen]
        jl  LastMatchGood
        shr ebx, 2
LastMatchGood:

;;; chainlen is decremented once beforehand so that the function can
;;; use the sign flag instead of the zero flag for the exit test.
;;; It is then shifted into the high word, to make room for the wmask
;;; value, which it will always accompany.

        dec ebx
        shl ebx, 16
        or  ebx, eax
        mov [chainlenwmask], ebx

;;; if ((uInt)nice_match > s->lookahead) nice_match = s->lookahead;

        mov eax, [edx + dsNiceMatch]
        mov ebx, [edx + dsLookahead]
        cmp ebx, eax
        jl  LookaheadLess
        mov ebx, eax
LookaheadLess:  mov [nicematch], ebx

;;; register Bytef *scan = s->window + s->strstart;

        mov esi, [edx + dsWindow]
        mov [window], esi
        mov ebp, [edx + dsStrStart]
        lea edi, [esi + ebp]
        mov [scan], edi

;;; Determine how many bytes the scan ptr is off from being
;;; dword-aligned.

        mov eax, edi
        neg eax
        and eax, 3
        mov [scanalign], eax

;;; IPos limit = s->strstart > (IPos)MAX_DIST(s) ?
;;;     s->strstart - (IPos)MAX_DIST(s) : NIL;

        mov eax, [edx + dsWSize]
        sub eax, MIN_LOOKAHEAD
        sub ebp, eax
        jg  LimitPositive
        xor ebp, ebp
LimitPositive:

;;; int best_len = s->prev_length;

        mov eax, [edx + dsPrevLen]
        mov [bestlen], eax

;;; Store the sum of s->window + best_len in esi locally, and in esi.

        add esi, eax
        mov [windowbestlen], esi

;;; register ush scan_start = *(ushf*)scan;
;;; register ush scan_end   = *(ushf*)(scan+best_len-1);
;;; Posf *prev = s->prev;

        movzx   ebx, word ptr [edi]
        mov [scanstart], ebx
        movzx   ebx, word ptr [edi + eax - 1]
        mov [scanend], ebx
        mov edi, [edx + dsPrev]

;;; Jump into the main loop.

        mov edx, [chainlenwmask]
        jmp short LoopEntry

align 4

;;; do {
;;;     match = s->window + cur_match;
;;;     if (*(ushf*)(match+best_len-1) != scan_end ||
;;;         *(ushf*)match != scan_start) continue;
;;;     [...]
;;; } while ((cur_match = prev[cur_match & wmask]) > limit
;;;          && --chain_length != 0);
;;;
;;; Here is the inner loop of the function. The function will spend the
;;; majority of its time in this loop, and majority of that time will
;;; be spent in the first ten instructions.
;;;
;;; Within this loop:
;;; ebx = scanend
;;; ecx = curmatch
;;; edx = chainlenwmask - i.e., ((chainlen << 16) | wmask)
;;; esi = windowbestlen - i.e., (window + bestlen)
;;; edi = prev
;;; ebp = limit

LookupLoop:
        and ecx, edx
        movzx   ecx, word ptr [edi + ecx*2]
        cmp ecx, ebp
        jbe LeaveNow
        sub edx, 00010000h
        js  LeaveNow
LoopEntry:  movzx   eax, word ptr [esi + ecx - 1]
        cmp eax, ebx
        jnz LookupLoop
        mov eax, [window]
        movzx   eax, word ptr [eax + ecx]
        cmp eax, [scanstart]
        jnz LookupLoop

;;; Store the current value of chainlen.

        mov [chainlenwmask], edx

;;; Point edi to the string under scrutiny, and esi to the string we
;;; are hoping to match it up with. In actuality, esi and edi are
;;; both pointed (MAX_MATCH_8 - scanalign) bytes ahead, and edx is
;;; initialized to -(MAX_MATCH_8 - scanalign).

        mov esi, [window]
        mov edi, [scan]
        add esi, ecx
        mov eax, [scanalign]
        mov edx, 0fffffef8h; -(MAX_MATCH_8)
        lea edi, [edi + eax + 0108h] ;MAX_MATCH_8]
        lea esi, [esi + eax + 0108h] ;MAX_MATCH_8]

;;; Test the strings for equality, 8 bytes at a time. At the end,
;;; adjust edx so that it is offset to the exact byte that mismatched.
;;;
;;; We already know at this point that the first three bytes of the
;;; strings match each other, and they can be safely passed over before
;;; starting the compare loop. So what this code does is skip over 0-3
;;; bytes, as much as necessary in order to dword-align the edi
;;; pointer. (esi will still be misaligned three times out of four.)
;;;
;;; It should be confessed that this loop usually does not represent
;;; much of the total running time. Replacing it with a more
;;; straightforward "rep cmpsb" would not drastically degrade
;;; performance.

LoopCmps:
        mov eax, [esi + edx]
        xor eax, [edi + edx]
        jnz LeaveLoopCmps
        mov eax, [esi + edx + 4]
        xor eax, [edi + edx + 4]
        jnz LeaveLoopCmps4
        add edx, 8
        jnz LoopCmps
        jmp short LenMaximum
LeaveLoopCmps4: add edx, 4
LeaveLoopCmps:  test    eax, 0000FFFFh
        jnz LenLower
        add edx,  2
        shr eax, 16
LenLower:   sub al, 1
        adc edx, 0

;;; Calculate the length of the match. If it is longer than MAX_MATCH,
;;; then automatically accept it as the best possible match and leave.

        lea eax, [edi + edx]
        mov edi, [scan]
        sub eax, edi
        cmp eax, MAX_MATCH
        jge LenMaximum

;;; If the length of the match is not longer than the best match we
;;; have so far, then forget it and return to the lookup loop.

        mov edx, [deflatestate]
        mov ebx, [bestlen]
        cmp eax, ebx
        jg  LongerMatch
        mov esi, [windowbestlen]
        mov edi, [edx + dsPrev]
        mov ebx, [scanend]
        mov edx, [chainlenwmask]
        jmp LookupLoop

;;;         s->match_start = cur_match;
;;;         best_len = len;
;;;         if (len >= nice_match) break;
;;;         scan_end = *(ushf*)(scan+best_len-1);

LongerMatch:    mov ebx, [nicematch]
        mov [bestlen], eax
        mov [edx + dsMatchStart], ecx
        cmp eax, ebx
        jge LeaveNow
        mov esi, [window]
        add esi, eax
        mov [windowbestlen], esi
        movzx   ebx, word ptr [edi + eax - 1]
        mov edi, [edx + dsPrev]
        mov [scanend], ebx
        mov edx, [chainlenwmask]
        jmp LookupLoop

;;; Accept the current string, with the maximum possible length.

LenMaximum: mov edx, [deflatestate]
        mov dword ptr [bestlen], MAX_MATCH
        mov [edx + dsMatchStart], ecx

;;; if ((uInt)best_len <= s->lookahead) return (uInt)best_len;
;;; return s->lookahead;

LeaveNow:
        mov edx, [deflatestate]
        mov ebx, [bestlen]
        mov eax, [edx + dsLookahead]
        cmp ebx, eax
        jg  LookaheadRet
        mov eax, ebx
LookaheadRet:

;;; Restore the stack and return from whence we came.

        add esp, LocalVarsSize
        pop ebx
        pop esi
        pop edi
        pop ebp

        ret
; please don't remove this string !
; Your can freely use gvmat32 in any free or commercial app if you don't remove the string in the binary!
    db     0dh,0ah,"asm686 with masm, optimised assembly code from Brian Raiter, written 1998",0dh,0ah


IFDEF NOOLDPENTIUMCODE
    IFDEF NOUNDERLINE
    longest_match       endp
    ELSE
    _longest_match      endp
    ENDIF

    IFDEF NOUNDERLINE
    match_init      proc near
                    ret
    match_init      endp
    ELSE
    _match_init     proc near
                    ret
    _match_init     endp
    ENDIF    
ELSE
    IFDEF NOUNDERLINE
    longest_match_686   endp
    ELSE
    _longest_match_686  endp
    ENDIF
ENDIF

_TEXT   ends
end
