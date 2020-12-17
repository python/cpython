/*
 * lexical analyzer
 * This file is #included by regcomp.c.
 *
 * Copyright (c) 1998, 1999 Henry Spencer.  All rights reserved.
 *
 * Development of this software was funded, in part, by Cray Research Inc.,
 * UUNET Communications Services Inc., Sun Microsystems Inc., and Scriptics
 * Corporation, none of whom are responsible for the results.  The author
 * thanks all of them.
 *
 * Redistribution and use in source and binary forms -- with or without
 * modification -- are permitted for any purpose, provided that
 * redistributions in source form retain this entire copyright notice and
 * indicate the origin and nature of any modifications.
 *
 * I'd appreciate being given credit for this package in the documentation of
 * software which uses it, but that is not a requirement.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * HENRY SPENCER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* scanning macros (know about v) */
#define	ATEOS()		(v->now >= v->stop)
#define	HAVE(n)		(v->stop - v->now >= (n))
#define	NEXT1(c)	(!ATEOS() && *v->now == CHR(c))
#define	NEXT2(a,b)	(HAVE(2) && *v->now == CHR(a) && *(v->now+1) == CHR(b))
#define	NEXT3(a,b,c) \
	(HAVE(3) && *v->now == CHR(a) && \
		*(v->now+1) == CHR(b) && \
		*(v->now+2) == CHR(c))
#define	SET(c)		(v->nexttype = (c))
#define	SETV(c, n)	(v->nexttype = (c), v->nextvalue = (n))
#define	RET(c)		return (SET(c), 1)
#define	RETV(c, n)	return (SETV(c, n), 1)
#define	FAILW(e)	return (ERR(e), 0)	/* ERR does SET(EOS) */
#define	LASTTYPE(t)	(v->lasttype == (t))

/* lexical contexts */
#define	L_ERE	1	/* mainline ERE/ARE */
#define	L_BRE	2	/* mainline BRE */
#define	L_Q	3	/* REG_QUOTE */
#define	L_EBND	4	/* ERE/ARE bound */
#define	L_BBND	5	/* BRE bound */
#define	L_BRACK	6	/* brackets */
#define	L_CEL	7	/* collating element */
#define	L_ECL	8	/* equivalence class */
#define	L_CCL	9	/* character class */
#define	INTOCON(c)	(v->lexcon = (c))
#define	INCON(con)	(v->lexcon == (con))

/* construct pointer past end of chr array */
#define	ENDOF(array)	((array) + sizeof(array)/sizeof(chr))

/*
 - lexstart - set up lexical stuff, scan leading options
 ^ static void lexstart(struct vars *);
 */
static void
lexstart(
    struct vars *v)
{
    prefixes(v);		/* may turn on new type bits etc. */
    NOERR();

    if (v->cflags&REG_QUOTE) {
	assert(!(v->cflags&(REG_ADVANCED|REG_EXPANDED|REG_NEWLINE)));
	INTOCON(L_Q);
    } else if (v->cflags&REG_EXTENDED) {
	assert(!(v->cflags&REG_QUOTE));
	INTOCON(L_ERE);
    } else {
	assert(!(v->cflags&(REG_QUOTE|REG_ADVF)));
	INTOCON(L_BRE);
    }

    v->nexttype = EMPTY;	/* remember we were at the start */
    next(v);			/* set up the first token */
}

/*
 - prefixes - implement various special prefixes
 ^ static void prefixes(struct vars *);
 */
static void
prefixes(
    struct vars *v)
{
    /*
     * Literal string doesn't get any of this stuff.
     */

    if (v->cflags&REG_QUOTE) {
	return;
    }

    /*
     * Initial "***" gets special things.
     */

    if (HAVE(4) && NEXT3('*', '*', '*')) {
	switch (*(v->now + 3)) {
	case CHR('?'):		/* "***?" error, msg shows version */
	    ERR(REG_BADPAT);
	    return;		/* proceed no further */
	    break;
	case CHR('='):		/* "***=" shifts to literal string */
	    NOTE(REG_UNONPOSIX);
	    v->cflags |= REG_QUOTE;
	    v->cflags &= ~(REG_ADVANCED|REG_EXPANDED|REG_NEWLINE);
	    v->now += 4;
	    return;		/* and there can be no more prefixes */
	    break;
	case CHR(':'):		/* "***:" shifts to AREs */
	    NOTE(REG_UNONPOSIX);
	    v->cflags |= REG_ADVANCED;
	    v->now += 4;
	    break;
	default:		/* otherwise *** is just an error */
	    ERR(REG_BADRPT);
	    return;
	    break;
	}
    }

    /*
     * BREs and EREs don't get embedded options.
     */

    if ((v->cflags&REG_ADVANCED) != REG_ADVANCED) {
	return;
    }

    /*
     * Embedded options (AREs only).
     */

    if (HAVE(3) && NEXT2('(', '?') && iscalpha(*(v->now + 2))) {
	NOTE(REG_UNONPOSIX);
	v->now += 2;
	for (; !ATEOS() && iscalpha(*v->now); v->now++) {
	    switch (*v->now) {
	    case CHR('b'):	/* BREs (but why???) */
		v->cflags &= ~(REG_ADVANCED|REG_QUOTE);
		break;
	    case CHR('c'):	/* case sensitive */
		v->cflags &= ~REG_ICASE;
		break;
	    case CHR('e'):	/* plain EREs */
		v->cflags |= REG_EXTENDED;
		v->cflags &= ~(REG_ADVF|REG_QUOTE);
		break;
	    case CHR('i'):	/* case insensitive */
		v->cflags |= REG_ICASE;
		break;
	    case CHR('m'):	/* Perloid synonym for n */
	    case CHR('n'):	/* \n affects ^ $ . [^ */
		v->cflags |= REG_NEWLINE;
		break;
	    case CHR('p'):	/* ~Perl, \n affects . [^ */
		v->cflags |= REG_NLSTOP;
		v->cflags &= ~REG_NLANCH;
		break;
	    case CHR('q'):	/* literal string */
		v->cflags |= REG_QUOTE;
		v->cflags &= ~REG_ADVANCED;
		break;
	    case CHR('s'):	/* single line, \n ordinary */
		v->cflags &= ~REG_NEWLINE;
		break;
	    case CHR('t'):	/* tight syntax */
		v->cflags &= ~REG_EXPANDED;
		break;
	    case CHR('w'):	/* weird, \n affects ^ $ only */
		v->cflags &= ~REG_NLSTOP;
		v->cflags |= REG_NLANCH;
		break;
	    case CHR('x'):	/* expanded syntax */
		v->cflags |= REG_EXPANDED;
		break;
	    default:
		ERR(REG_BADOPT);
		return;
	    }
	}
	if (!NEXT1(')')) {
	    ERR(REG_BADOPT);
	    return;
	}
	v->now++;
	if (v->cflags&REG_QUOTE) {
	    v->cflags &= ~(REG_EXPANDED|REG_NEWLINE);
	}
    }
}

/*
 - lexnest - "call a subroutine", interpolating string at the lexical level
 * Note, this is not a very general facility.  There are a number of
 * implicit assumptions about what sorts of strings can be subroutines.
 ^ static void lexnest(struct vars *, const chr *, const chr *);
 */
static void
lexnest(
    struct vars *v,
    const chr *beginp,		/* start of interpolation */
    const chr *endp)		/* one past end of interpolation */
{
    assert(v->savenow == NULL);	/* only one level of nesting */
    v->savenow = v->now;
    v->savestop = v->stop;
    v->now = beginp;
    v->stop = endp;
}

/*
 * string constants to interpolate as expansions of things like \d
 */

static const chr backd[] = {	/* \d */
    CHR('['), CHR('['), CHR(':'),
    CHR('d'), CHR('i'), CHR('g'), CHR('i'), CHR('t'),
    CHR(':'), CHR(']'), CHR(']')
};
static const chr backD[] = {	/* \D */
    CHR('['), CHR('^'), CHR('['), CHR(':'),
    CHR('d'), CHR('i'), CHR('g'), CHR('i'), CHR('t'),
    CHR(':'), CHR(']'), CHR(']')
};
static const chr brbackd[] = {	/* \d within brackets */
    CHR('['), CHR(':'),
    CHR('d'), CHR('i'), CHR('g'), CHR('i'), CHR('t'),
    CHR(':'), CHR(']')
};
static const chr backs[] = {	/* \s */
    CHR('['), CHR('['), CHR(':'),
    CHR('s'), CHR('p'), CHR('a'), CHR('c'), CHR('e'),
    CHR(':'), CHR(']'), CHR(']')
};
static const chr backS[] = {	/* \S */
    CHR('['), CHR('^'), CHR('['), CHR(':'),
    CHR('s'), CHR('p'), CHR('a'), CHR('c'), CHR('e'),
    CHR(':'), CHR(']'), CHR(']')
};
static const chr brbacks[] = {	/* \s within brackets */
    CHR('['), CHR(':'),
    CHR('s'), CHR('p'), CHR('a'), CHR('c'), CHR('e'),
    CHR(':'), CHR(']')
};

#define PUNCT_CONN \
	CHR('_'), \
	0x203f /* UNDERTIE */, \
	0x2040 /* CHARACTER TIE */,\
	0x2054 /* INVERTED UNDERTIE */,\
	0xfe33 /* PRESENTATION FORM FOR VERTICAL LOW LINE */, \
	0xfe34 /* PRESENTATION FORM FOR VERTICAL WAVY LOW LINE */, \
	0xfe4d /* DASHED LOW LINE */, \
	0xfe4e /* CENTRELINE LOW LINE */, \
	0xfe4f /* WAVY LOW LINE */, \
	0xff3f /* FULLWIDTH LOW LINE */

static const chr backw[] = {	/* \w */
    CHR('['), CHR('['), CHR(':'),
    CHR('a'), CHR('l'), CHR('n'), CHR('u'), CHR('m'),
    CHR(':'), CHR(']'), PUNCT_CONN, CHR(']')
};
static const chr backW[] = {	/* \W */
    CHR('['), CHR('^'), CHR('['), CHR(':'),
    CHR('a'), CHR('l'), CHR('n'), CHR('u'), CHR('m'),
    CHR(':'), CHR(']'), PUNCT_CONN, CHR(']')
};
static const chr brbackw[] = {	/* \w within brackets */
    CHR('['), CHR(':'),
    CHR('a'), CHR('l'), CHR('n'), CHR('u'), CHR('m'),
    CHR(':'), CHR(']'), PUNCT_CONN
};

/*
 - lexword - interpolate a bracket expression for word characters
 * Possibly ought to inquire whether there is a "word" character class.
 ^ static void lexword(struct vars *);
 */
static void
lexword(
    struct vars *v)
{
    lexnest(v, backw, ENDOF(backw));
}

/*
 - next - get next token
 ^ static int next(struct vars *);
 */
static int			/* 1 normal, 0 failure */
next(
    struct vars *v)
{
    chr c;

    /*
     * Errors yield an infinite sequence of failures.
     */

    if (ISERR()) {
	return 0;		/* the error has set nexttype to EOS */
    }

    /*
     * Remember flavor of last token.
     */

    v->lasttype = v->nexttype;

    /*
     * REG_BOSONLY
     */

    if (v->nexttype == EMPTY && (v->cflags&REG_BOSONLY)) {
	/* at start of a REG_BOSONLY RE */
	RETV(SBEGIN, 0);	/* same as \A */
    }

    /*
     * If we're nested and we've hit end, return to outer level.
     */

    if (v->savenow != NULL && ATEOS()) {
	v->now = v->savenow;
	v->stop = v->savestop;
	v->savenow = v->savestop = NULL;
    }

    /*
     * Skip white space etc. if appropriate (not in literal or [])
     */

    if (v->cflags&REG_EXPANDED) {
	switch (v->lexcon) {
	case L_ERE:
	case L_BRE:
	case L_EBND:
	case L_BBND:
	    skip(v);
	    break;
	}
    }

    /*
     * Handle EOS, depending on context.
     */

    if (ATEOS()) {
	switch (v->lexcon) {
	case L_ERE:
	case L_BRE:
	case L_Q:
	    RET(EOS);
	    break;
	case L_EBND:
	case L_BBND:
	    FAILW(REG_EBRACE);
	    break;
	case L_BRACK:
	case L_CEL:
	case L_ECL:
	case L_CCL:
	    FAILW(REG_EBRACK);
	    break;
	}
	assert(NOTREACHED);
    }

    /*
     * Okay, time to actually get a character.
     */

    c = *v->now++;

    /*
     * Deal with the easy contexts, punt EREs to code below.
     */

    switch (v->lexcon) {
    case L_BRE:			/* punt BREs to separate function */
	return brenext(v, c);
	break;
    case L_ERE:			/* see below */
	break;
    case L_Q:			/* literal strings are easy */
	RETV(PLAIN, c);
	break;
    case L_BBND:		/* bounds are fairly simple */
    case L_EBND:
	switch (c) {
	case CHR('0'): case CHR('1'): case CHR('2'): case CHR('3'):
	case CHR('4'): case CHR('5'): case CHR('6'): case CHR('7'):
	case CHR('8'): case CHR('9'):
	    RETV(DIGIT, (chr)DIGITVAL(c));
	    break;
	case CHR(','):
	    RET(',');
	    break;
	case CHR('}'):		/* ERE bound ends with } */
	    if (INCON(L_EBND)) {
		INTOCON(L_ERE);
		if ((v->cflags&REG_ADVF) && NEXT1('?')) {
		    v->now++;
		    NOTE(REG_UNONPOSIX);
		    RETV('}', 0);
		}
		RETV('}', 1);
	    } else {
		FAILW(REG_BADBR);
	    }
	    break;
	case CHR('\\'):		/* BRE bound ends with \} */
	    if (INCON(L_BBND) && NEXT1('}')) {
		v->now++;
		INTOCON(L_BRE);
		RET('}');
	    } else {
		FAILW(REG_BADBR);
	    }
	    break;
	default:
	    FAILW(REG_BADBR);
	    break;
	}
	assert(NOTREACHED);
	break;
    case L_BRACK:		/* brackets are not too hard */
	switch (c) {
	case CHR(']'):
	    if (LASTTYPE('[')) {
		RETV(PLAIN, c);
	    } else {
		INTOCON((v->cflags&REG_EXTENDED) ? L_ERE : L_BRE);
		RET(']');
	    }
	    break;
	case CHR('\\'):
	    NOTE(REG_UBBS);
	    if (!(v->cflags&REG_ADVF)) {
		RETV(PLAIN, c);
	    }
	    NOTE(REG_UNONPOSIX);
	    if (ATEOS()) {
		FAILW(REG_EESCAPE);
	    }
	    (DISCARD)lexescape(v);
	    switch (v->nexttype) {	/* not all escapes okay here */
	    case PLAIN:
		return 1;
		break;
	    case CCLASS:
		switch (v->nextvalue) {
		case 'd':
		    lexnest(v, brbackd, ENDOF(brbackd));
		    break;
		case 's':
		    lexnest(v, brbacks, ENDOF(brbacks));
		    break;
		case 'w':
		    lexnest(v, brbackw, ENDOF(brbackw));
		    break;
		default:
		    FAILW(REG_EESCAPE);
		    break;
		}

		/*
		 * lexnest() done, back up and try again.
		 */

		v->nexttype = v->lasttype;
		return next(v);
		break;
	    }

	    /*
	     * Not one of the acceptable escapes.
	     */

	    FAILW(REG_EESCAPE);
	    break;
	case CHR('-'):
	    if (LASTTYPE('[') || NEXT1(']')) {
		RETV(PLAIN, c);
	    } else {
		RETV(RANGE, c);
	    }
	    break;
	case CHR('['):
	    if (ATEOS()) {
		FAILW(REG_EBRACK);
	    }
	    switch (*v->now++) {
	    case CHR('.'):
		INTOCON(L_CEL);

		/*
		 * Might or might not be locale-specific.
		 */

		RET(COLLEL);
		break;
	    case CHR('='):
		INTOCON(L_ECL);
		NOTE(REG_ULOCALE);
		RET(ECLASS);
		break;
	    case CHR(':'):
		INTOCON(L_CCL);
		NOTE(REG_ULOCALE);
		RET(CCLASS);
		break;
	    default:		/* oops */
		v->now--;
		RETV(PLAIN, c);
		break;
	    }
	    assert(NOTREACHED);
	    break;
	default:
	    RETV(PLAIN, c);
	    break;
	}
	assert(NOTREACHED);
	break;
    case L_CEL:			/* collating elements are easy */
	if (c == CHR('.') && NEXT1(']')) {
	    v->now++;
	    INTOCON(L_BRACK);
	    RETV(END, '.');
	} else {
	    RETV(PLAIN, c);
	}
	break;
    case L_ECL:			/* ditto equivalence classes */
	if (c == CHR('=') && NEXT1(']')) {
	    v->now++;
	    INTOCON(L_BRACK);
	    RETV(END, '=');
	} else {
	    RETV(PLAIN, c);
	}
	break;
    case L_CCL:			/* ditto character classes */
	if (c == CHR(':') && NEXT1(']')) {
	    v->now++;
	    INTOCON(L_BRACK);
	    RETV(END, ':');
	} else {
	    RETV(PLAIN, c);
	}
	break;
    default:
	assert(NOTREACHED);
	break;
    }

    /*
     * That got rid of everything except EREs and AREs.
     */

    assert(INCON(L_ERE));

    /*
     * Deal with EREs and AREs, except for backslashes.
     */

    switch (c) {
    case CHR('|'):
	RET('|');
	break;
    case CHR('*'):
	if ((v->cflags&REG_ADVF) && NEXT1('?')) {
	    v->now++;
	    NOTE(REG_UNONPOSIX);
	    RETV('*', 0);
	}
	RETV('*', 1);
	break;
    case CHR('+'):
	if ((v->cflags&REG_ADVF) && NEXT1('?')) {
	    v->now++;
	    NOTE(REG_UNONPOSIX);
	    RETV('+', 0);
	}
	RETV('+', 1);
	break;
    case CHR('?'):
	if ((v->cflags&REG_ADVF) && NEXT1('?')) {
	    v->now++;
	    NOTE(REG_UNONPOSIX);
	    RETV('?', 0);
	}
	RETV('?', 1);
	break;
    case CHR('{'):		/* bounds start or plain character */
	if (v->cflags&REG_EXPANDED) {
	    skip(v);
	}
	if (ATEOS() || !iscdigit(*v->now)) {
	    NOTE(REG_UBRACES);
	    NOTE(REG_UUNSPEC);
	    RETV(PLAIN, c);
	} else {
	    NOTE(REG_UBOUNDS);
	    INTOCON(L_EBND);
	    RET('{');
	}
	assert(NOTREACHED);
	break;
    case CHR('('):		/* parenthesis, or advanced extension */
	if ((v->cflags&REG_ADVF) && NEXT1('?')) {
	    NOTE(REG_UNONPOSIX);
	    v->now++;
	    switch (*v->now++) {
	    case CHR(':'):	/* non-capturing paren */
		RETV('(', 0);
		break;
	    case CHR('#'):	/* comment */
		while (!ATEOS() && *v->now != CHR(')')) {
		    v->now++;
		}
		if (!ATEOS()) {
		    v->now++;
		}
		assert(v->nexttype == v->lasttype);
		return next(v);
		break;
	    case CHR('='):	/* positive lookahead */
		NOTE(REG_ULOOKAHEAD);
		RETV(LACON, 1);
		break;
	    case CHR('!'):	/* negative lookahead */
		NOTE(REG_ULOOKAHEAD);
		RETV(LACON, 0);
		break;
	    default:
		FAILW(REG_BADRPT);
		break;
	    }
	    assert(NOTREACHED);
	}
	if (v->cflags&REG_NOSUB) {
	    RETV('(', 0);	/* all parens non-capturing */
	} else {
	    RETV('(', 1);
	}
	break;
    case CHR(')'):
	if (LASTTYPE('(')) {
	    NOTE(REG_UUNSPEC);
	}
	RETV(')', c);
	break;
    case CHR('['):		/* easy except for [[:<:]] and [[:>:]] */
	if (HAVE(6) &&	*(v->now+0) == CHR('[') &&
		*(v->now+1) == CHR(':') &&
		(*(v->now+2) == CHR('<') || *(v->now+2) == CHR('>')) &&
		*(v->now+3) == CHR(':') &&
		*(v->now+4) == CHR(']') &&
		*(v->now+5) == CHR(']')) {
	    c = *(v->now+2);
	    v->now += 6;
	    NOTE(REG_UNONPOSIX);
	    RET((c == CHR('<')) ? '<' : '>');
	}
	INTOCON(L_BRACK);
	if (NEXT1('^')) {
	    v->now++;
	    RETV('[', 0);
	}
	RETV('[', 1);
	break;
    case CHR('.'):
	RET('.');
	break;
    case CHR('^'):
	RET('^');
	break;
    case CHR('$'):
	RET('$');
	break;
    case CHR('\\'):		/* mostly punt backslashes to code below */
	if (ATEOS()) {
	    FAILW(REG_EESCAPE);
	}
	break;
    default:		/* ordinary character */
	RETV(PLAIN, c);
	break;
    }

    /*
     * ERE/ARE backslash handling; backslash already eaten.
     */

    assert(!ATEOS());
    if (!(v->cflags&REG_ADVF)) {/* only AREs have non-trivial escapes */
	if (iscalnum(*v->now)) {
	    NOTE(REG_UBSALNUM);
	    NOTE(REG_UUNSPEC);
	}
	RETV(PLAIN, *v->now++);
    }
    (DISCARD)lexescape(v);
    if (ISERR()) {
	FAILW(REG_EESCAPE);
    }
    if (v->nexttype == CCLASS) {/* fudge at lexical level */
	switch (v->nextvalue) {
	case 'd':	lexnest(v, backd, ENDOF(backd)); break;
	case 'D':	lexnest(v, backD, ENDOF(backD)); break;
	case 's':	lexnest(v, backs, ENDOF(backs)); break;
	case 'S':	lexnest(v, backS, ENDOF(backS)); break;
	case 'w':	lexnest(v, backw, ENDOF(backw)); break;
	case 'W':	lexnest(v, backW, ENDOF(backW)); break;
	default:
	    assert(NOTREACHED);
	    FAILW(REG_ASSERT);
	    break;
	}
	/* lexnest done, back up and try again */
	v->nexttype = v->lasttype;
	return next(v);
    }

    /*
     * Otherwise, lexescape has already done the work.
     */

    return !ISERR();
}

/*
 - lexescape - parse an ARE backslash escape (backslash already eaten)
 * Note slightly nonstandard use of the CCLASS type code.
 ^ static int lexescape(struct vars *);
 */
static int			/* not actually used, but convenient for RETV */
lexescape(
    struct vars *v)
{
    chr c;
    int i;
    static const chr alert[] = {
	CHR('a'), CHR('l'), CHR('e'), CHR('r'), CHR('t')
    };
    static const chr esc[] = {
	CHR('E'), CHR('S'), CHR('C')
    };
    const chr *save;

    assert(v->cflags&REG_ADVF);

    assert(!ATEOS());
    c = *v->now++;
    if (!iscalnum(c)) {
	RETV(PLAIN, c);
    }

    NOTE(REG_UNONPOSIX);
    switch (c) {
    case CHR('a'):
	RETV(PLAIN, chrnamed(v, alert, ENDOF(alert), CHR('\007')));
	break;
    case CHR('A'):
	RETV(SBEGIN, 0);
	break;
    case CHR('b'):
	RETV(PLAIN, CHR('\b'));
	break;
    case CHR('B'):
	RETV(PLAIN, CHR('\\'));
	break;
    case CHR('c'):
	NOTE(REG_UUNPORT);
	if (ATEOS()) {
	    FAILW(REG_EESCAPE);
	}
	RETV(PLAIN, (chr)(*v->now++ & 037));
	break;
    case CHR('d'):
	NOTE(REG_ULOCALE);
	RETV(CCLASS, 'd');
	break;
    case CHR('D'):
	NOTE(REG_ULOCALE);
	RETV(CCLASS, 'D');
	break;
    case CHR('e'):
	NOTE(REG_UUNPORT);
	RETV(PLAIN, chrnamed(v, esc, ENDOF(esc), CHR('\033')));
	break;
    case CHR('f'):
	RETV(PLAIN, CHR('\f'));
	break;
    case CHR('m'):
	RET('<');
	break;
    case CHR('M'):
	RET('>');
	break;
    case CHR('n'):
	RETV(PLAIN, CHR('\n'));
	break;
    case CHR('r'):
	RETV(PLAIN, CHR('\r'));
	break;
    case CHR('s'):
	NOTE(REG_ULOCALE);
	RETV(CCLASS, 's');
	break;
    case CHR('S'):
	NOTE(REG_ULOCALE);
	RETV(CCLASS, 'S');
	break;
    case CHR('t'):
	RETV(PLAIN, CHR('\t'));
	break;
    case CHR('u'):
	c = (uchr) lexdigits(v, 16, 1, 4);
	if (ISERR()) {
	    FAILW(REG_EESCAPE);
	}
	RETV(PLAIN, c);
	break;
    case CHR('U'):
	i = lexdigits(v, 16, 1, 8);
	if (ISERR()) {
	    FAILW(REG_EESCAPE);
	}
	if (i > 0xFFFF) {
	    /* TODO: output a Surrogate pair
	     */
	    i = 0xFFFD;
	}
	RETV(PLAIN, (uchr) i);
	break;
    case CHR('v'):
	RETV(PLAIN, CHR('\v'));
	break;
    case CHR('w'):
	NOTE(REG_ULOCALE);
	RETV(CCLASS, 'w');
	break;
    case CHR('W'):
	NOTE(REG_ULOCALE);
	RETV(CCLASS, 'W');
	break;
    case CHR('x'):
	NOTE(REG_UUNPORT);
	c = (uchr) lexdigits(v, 16, 1, 2);
	if (ISERR()) {
	    FAILW(REG_EESCAPE);
	}
	RETV(PLAIN, c);
	break;
    case CHR('y'):
	NOTE(REG_ULOCALE);
	RETV(WBDRY, 0);
	break;
    case CHR('Y'):
	NOTE(REG_ULOCALE);
	RETV(NWBDRY, 0);
	break;
    case CHR('Z'):
	RETV(SEND, 0);
	break;
    case CHR('1'): case CHR('2'): case CHR('3'): case CHR('4'):
    case CHR('5'): case CHR('6'): case CHR('7'): case CHR('8'):
    case CHR('9'):
	save = v->now;
	v->now--;		/* put first digit back */
	c = (uchr) lexdigits(v, 10, 1, 255);	/* REs >255 long outside spec */
	if (ISERR()) {
	    FAILW(REG_EESCAPE);
	}

	/*
	 * Ugly heuristic (first test is "exactly 1 digit?")
	 */

	if (v->now - save == 0 || ((int) c > 0 && (int)c <= v->nsubexp)) {
	    NOTE(REG_UBACKREF);
	    RETV(BACKREF, (chr)c);
	}

	/*
	 * Oops, doesn't look like it's a backref after all...
	 */

	v->now = save;

	/*
	 * And fall through into octal number.
	 */

    case CHR('0'):
	NOTE(REG_UUNPORT);
	v->now--;		/* put first digit back */
	c = (uchr) lexdigits(v, 8, 1, 3);
	if (ISERR()) {
	    FAILW(REG_EESCAPE);
	}
	if (c > 0xff) {
	    /* out of range, so we handled one digit too much */
	    v->now--;
	    c >>= 3;
	}
	RETV(PLAIN, c);
	break;
    default:
	assert(iscalpha(c));
	FAILW(REG_EESCAPE);	/* unknown alphabetic escape */
	break;
    }
    assert(NOTREACHED);
}

/*
 - lexdigits - slurp up digits and return chr value
 ^ static int lexdigits(struct vars *, int, int, int);
 */
static int			/* chr value; errors signalled via ERR */
lexdigits(
    struct vars *v,
    int base,
    int minlen,
    int maxlen)
{
    int n;
    int len;
    chr c;
    int d;
    const uchr ub = (uchr) base;

    n = 0;
    for (len = 0; len < maxlen && !ATEOS(); len++) {
	if (n > 0x10fff) {
	    /* Stop when continuing would otherwise overflow */
	    break;
	}
	c = *v->now++;
	switch (c) {
	case CHR('0'): case CHR('1'): case CHR('2'): case CHR('3'):
	case CHR('4'): case CHR('5'): case CHR('6'): case CHR('7'):
	case CHR('8'): case CHR('9'):
	    d = DIGITVAL(c);
	    break;
	case CHR('a'): case CHR('A'): d = 10; break;
	case CHR('b'): case CHR('B'): d = 11; break;
	case CHR('c'): case CHR('C'): d = 12; break;
	case CHR('d'): case CHR('D'): d = 13; break;
	case CHR('e'): case CHR('E'): d = 14; break;
	case CHR('f'): case CHR('F'): d = 15; break;
	default:
	    v->now--;		/* oops, not a digit at all */
	    d = -1;
	    break;
	}

	if (d >= base) {	/* not a plausible digit */
	    v->now--;
	    d = -1;
	}
	if (d < 0) {
	    break;		/* NOTE BREAK OUT */
	}
	n = n*ub + (uchr)d;
    }
    if (len < minlen) {
	ERR(REG_EESCAPE);
    }

    return n;
}

/*
 - brenext - get next BRE token
 * This is much like EREs except for all the stupid backslashes and the
 * context-dependency of some things.
 ^ static int brenext(struct vars *, pchr);
 */
static int			/* 1 normal, 0 failure */
brenext(
    struct vars *v,
    pchr pc)
{
    chr c = (chr)pc;

    switch (c) {
    case CHR('*'):
	if (LASTTYPE(EMPTY) || LASTTYPE('(') || LASTTYPE('^')) {
	    RETV(PLAIN, c);
	}
	RET('*');
	break;
    case CHR('['):
	if (HAVE(6) &&	*(v->now+0) == CHR('[') &&
		*(v->now+1) == CHR(':') &&
		(*(v->now+2) == CHR('<') || *(v->now+2) == CHR('>')) &&
		*(v->now+3) == CHR(':') &&
		*(v->now+4) == CHR(']') &&
		*(v->now+5) == CHR(']')) {
	    c = *(v->now+2);
	    v->now += 6;
	    NOTE(REG_UNONPOSIX);
	    RET((c == CHR('<')) ? '<' : '>');
	}
	INTOCON(L_BRACK);
	if (NEXT1('^')) {
	    v->now++;
	    RETV('[', 0);
	}
	RETV('[', 1);
	break;
    case CHR('.'):
	RET('.');
	break;
    case CHR('^'):
	if (LASTTYPE(EMPTY)) {
	    RET('^');
	}
	if (LASTTYPE('(')) {
	    NOTE(REG_UUNSPEC);
	    RET('^');
	}
	RETV(PLAIN, c);
	break;
    case CHR('$'):
	if (v->cflags&REG_EXPANDED) {
	    skip(v);
	}
	if (ATEOS()) {
	    RET('$');
	}
	if (NEXT2('\\', ')')) {
	    NOTE(REG_UUNSPEC);
	    RET('$');
	}
	RETV(PLAIN, c);
	break;
    case CHR('\\'):
	break;			/* see below */
    default:
	RETV(PLAIN, c);
	break;
    }

    assert(c == CHR('\\'));

    if (ATEOS()) {
	FAILW(REG_EESCAPE);
    }

    c = *v->now++;
    switch (c) {
    case CHR('{'):
	INTOCON(L_BBND);
	NOTE(REG_UBOUNDS);
	RET('{');
	break;
    case CHR('('):
	RETV('(', 1);
	break;
    case CHR(')'):
	RETV(')', c);
	break;
    case CHR('<'):
	NOTE(REG_UNONPOSIX);
	RET('<');
	break;
    case CHR('>'):
	NOTE(REG_UNONPOSIX);
	RET('>');
	break;
    case CHR('1'): case CHR('2'): case CHR('3'): case CHR('4'):
    case CHR('5'): case CHR('6'): case CHR('7'): case CHR('8'):
    case CHR('9'):
	NOTE(REG_UBACKREF);
	RETV(BACKREF, (chr)DIGITVAL(c));
	break;
    default:
	if (iscalnum(c)) {
	    NOTE(REG_UBSALNUM);
	    NOTE(REG_UUNSPEC);
	}
	RETV(PLAIN, c);
	break;
    }

    assert(NOTREACHED);
}

/*
 - skip - skip white space and comments in expanded form
 ^ static void skip(struct vars *);
 */
static void
skip(
    struct vars *v)
{
    const chr *start = v->now;

    assert(v->cflags&REG_EXPANDED);

    for (;;) {
	while (!ATEOS() && iscspace(*v->now)) {
	    v->now++;
	}
	if (ATEOS() || *v->now != CHR('#')) {
	    break;		/* NOTE BREAK OUT */
	}
	assert(NEXT1('#'));
	while (!ATEOS() && *v->now != CHR('\n')) {
	    v->now++;
	}

	/*
	 * Leave the newline to be picked up by the iscspace loop.
	 */
    }

    if (v->now != start) {
	NOTE(REG_UNONPOSIX);
    }
}

/*
 - newline - return the chr for a newline
 * This helps confine use of CHR to this source file.
 ^ static chr newline(NOPARMS);
 */
static chr
newline(void)
{
    return CHR('\n');
}

/*
 - chrnamed - return the chr known by a given (chr string) name
 * The code is a bit clumsy, but this routine gets only such specialized
 * use that it hardly matters.
 ^ static chr chrnamed(struct vars *, const chr *, const chr *, pchr);
 */
static chr
chrnamed(
    struct vars *v,
    const chr *startp,		/* start of name */
    const chr *endp,		/* just past end of name */
    pchr lastresort)		/* what to return if name lookup fails */
{
    celt c;
    int errsave;
    int e;
    struct cvec *cv;

    errsave = v->err;
    v->err = 0;
    c = element(v, startp, endp);
    e = v->err;
    v->err = errsave;

    if (e != 0) {
	return (chr)lastresort;
    }

    cv = range(v, c, c, 0);
    if (cv->nchrs == 0) {
	return (chr)lastresort;
    }
    return cv->chrs[0];
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
