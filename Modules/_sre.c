/* -*- Mode: C; tab-width: 4 -*-
 *
 * Secret Labs' Regular Expression Engine
 * $Id$
 *
 * simple regular expression matching engine
 *
 * partial history:
 * 99-10-24 fl	created (bits and pieces from the template matcher)
 * 99-11-13 fl	added categories, branching, and more (0.2)
 * 99-11-16 fl	some tweaks to compile on non-Windows platforms
 * 99-12-18 fl	non-literals, generic maximizing repeat (0.3)
 * 99-02-28 fl	tons of changes (not all to the better ;-) (0.4)
 * 99-03-06 fl	first alpha, sort of (0.5)
 * 99-03-14 fl	removed most compatibility stuff (0.6)
 *
 * Copyright (c) 1997-2000 by Secret Labs AB.  All rights reserved.
 *
 * This code can only be used for 1.6 alpha testing.  All other use
 * require explicit permission from Secret Labs AB.
 *
 * Portions of this engine have been developed in cooperation with
 * CNRI.  Hewlett-Packard provided funding for 1.6 integration and
 * other compatibility work.
 */

#ifndef SRE_RECURSIVE

char copyright[] = " SRE 0.6 Copyright (c) 1997-2000 by Secret Labs AB ";

#include "Python.h"

#include "sre.h"

#include "unicodeobject.h"

#if defined(HAVE_LIMITS_H)
#include <limits.h>
#else
#define INT_MAX 2147483647
#endif

#include <ctype.h> /* temporary hack */

/* defining this one enables tracing */
#undef DEBUG

#ifdef WIN32 /* FIXME: <fl> don't assume Windows == MSVC */
#pragma optimize("agtw", on) /* doesn't seem to make much difference... */
/* fastest possible local call under MSVC */
#define LOCAL(type) static __inline type __fastcall
#else
#define LOCAL(type) static type
#endif

/* error codes */
#define SRE_ERROR_ILLEGAL -1 /* illegal opcode */
#define SRE_ERROR_MEMORY -9 /* out of memory */

#ifdef	DEBUG
#define TRACE(v) printf v
#define PTR(ptr) ((SRE_CHAR*) (ptr) - (SRE_CHAR*) state->beginning)
#else
#define TRACE(v)
#endif

#define SRE_CODE unsigned short /* unsigned short or larger */

typedef struct {
	/* string pointers */
	void* ptr; /* current position (also end of current slice) */
	void* beginning; /* start of original string */
	void* start; /* start of current slice */
	void* end; /* end of original string */
	/* character size */
	int charsize;
	/* registers */
	int marks;
	void* mark[64]; /* FIXME: <fl> should be dynamically allocated! */
	/* FIXME */
	/* backtracking stack */
	void** stack;
	int stacksize;
	int stackbase;
} SRE_STATE;

#if 1 /* FIXME: <fl> fix this one! */
#define SRE_TO_LOWER Py_UNICODE_TOLOWER
#define SRE_IS_DIGIT Py_UNICODE_ISDIGIT
#define SRE_IS_SPACE Py_UNICODE_ISSPACE
#define SRE_IS_ALNUM(ch) ((ch) < 256 ? isalnum((ch)) : 0)
#else
#define SRE_TO_LOWER(ch) ((ch) < 256 ? tolower((ch)) : ch)
#define SRE_IS_DIGIT(ch) ((ch) < 256 ? isdigit((ch)) : 0)
#define SRE_IS_SPACE(ch) ((ch) < 256 ? isspace((ch)) : 0)
#define SRE_IS_ALNUM(ch) ((ch) < 256 ? isalnum((ch)) : 0)
#endif

#define SRE_IS_WORD(ch) (SRE_IS_ALNUM((ch)) || (ch) == '_')

LOCAL(int)
sre_category(SRE_CODE category, unsigned int ch)
{
	switch (category) {
	case 'd':
		return SRE_IS_DIGIT(ch);
	case 'D':
		return !SRE_IS_DIGIT(ch);
	case 's':
		return SRE_IS_SPACE(ch);
	case 'S':
		return !SRE_IS_SPACE(ch);
	case 'w':
		return SRE_IS_WORD(ch);
	case 'W':
		return !SRE_IS_WORD(ch);
	}
	return 0;
}

/* helpers */

LOCAL(int)
_stack_free(SRE_STATE* state)
{
	if (state->stack) {
		TRACE(("release stack\n"));
		free(state->stack);
		state->stack = NULL;
	}
	state->stacksize = 0;
	return 0;
}

static int /* shouldn't be LOCAL */
_stack_extend(SRE_STATE* state, int lo, int hi)
{
	void** stack;
	int stacksize;

	/* grow the stack to a suitable size; we need at least lo entries,
	   at most hi entries.	if for some reason hi is lower than lo, lo
	   wins */

	stacksize = state->stacksize;

	if (stacksize == 0) {
		/* create new stack */
		stacksize = 512;
		if (stacksize < lo)
			stacksize = lo;
		else if (stacksize > hi)
			stacksize = hi;
		TRACE(("allocate stack %d\n", stacksize));
		stack = malloc(sizeof(void*) * stacksize);
	} else {
		/* grow the stack (typically by a factor of two) */
		while (stacksize < lo)
			stacksize = 2 * stacksize;
		/* FIXME: <fl> could trim size if it's larger than lo, and
		   much larger than hi */
		TRACE(("grow stack to %d\n", stacksize));
		stack = realloc(state->stack, sizeof(void*) * stacksize);
	}

	if (!stack) {
		_stack_free(state);
		return SRE_ERROR_MEMORY;
	}

	state->stack = stack;
	state->stacksize = stacksize;

	return 0;
}

/* set things up for the 8-bit version */

#define SRE_CHAR unsigned char
#define SRE_AT sre_at
#define SRE_MEMBER sre_member
#define SRE_MATCH sre_match
#define SRE_SEARCH sre_search
#define SRE_RECURSIVE

#include "_sre.c"

#undef SRE_RECURSIVE
#undef SRE_SEARCH
#undef SRE_MATCH
#undef SRE_MEMBER
#undef SRE_AT
#undef SRE_CHAR

/* set things up for the 16-bit unicode version */

#define SRE_CHAR Py_UNICODE
#define SRE_AT sre_uat
#define SRE_MEMBER sre_umember
#define SRE_MATCH sre_umatch
#define SRE_SEARCH sre_usearch

#endif /* SRE_RECURSIVE */

/* -------------------------------------------------------------------- */
/* String matching engine */

/* the following section is compiled twice, with different character
   settings */

LOCAL(int)
SRE_AT(SRE_STATE* state, SRE_CHAR* ptr, SRE_CODE at)
{
	/* check if pointer is at given position.  return 1 if so, 0
	   otherwise */

	int this, that;

	switch (at) {
	case 'a':
		/* beginning */
		return (ptr == state->beginning);
	case 'z':
		/* end */
		return (ptr == state->end);
	case 'b':
		/* word boundary */
		if (state->beginning == state->end)
			return 0;
		that = ((void*) ptr > state->beginning) ?
			SRE_IS_WORD((int) ptr[-1]) : 0;
		this = ((void*) ptr < state->end) ?
			SRE_IS_WORD((int) ptr[0]) : 0;
		return this != that;
	case 'B':
		/* word non-boundary */
		if (state->beginning == state->end)
			return 0;
		that = ((void*) ptr > state->beginning) ?
			SRE_IS_WORD((int) ptr[-1]) : 0;
		this = ((void*) ptr < state->end) ?
			SRE_IS_WORD((int) ptr[0]) : 0;
		return this == that;
	}

	return 0;
}

LOCAL(int)
SRE_MEMBER(SRE_CODE* set, SRE_CHAR ch)
{
	/* check if character is a member of the given set.	 return 1 if
	   so, 0 otherwise */

	int ok = 1;

	for (;;) {
		switch (*set++) {

		case SRE_OP_NEGATE:
			ok = !ok;
			break;

		case SRE_OP_FAILURE:
			return !ok;

		case SRE_OP_LITERAL:
			if (ch == (SRE_CHAR) set[0])
				return ok;
			set++;
			break;

		case SRE_OP_RANGE:
			if ((SRE_CHAR) set[0] <= ch && ch <= (SRE_CHAR) set[1])
				return ok;
			set += 2;
			break;

		case SRE_OP_CATEGORY:
			if (sre_category(set[0], (int) ch))
				return ok;
			set += 1;
			break;

		default:
			/* FIXME: internal error */
			return 0;
		}
	}
}

LOCAL(int)
SRE_MATCH(SRE_STATE* state, SRE_CODE* pattern)
{
	/* check if string matches the given pattern.  returns -1 for
	   error, 0 for failure, and 1 for success */

	SRE_CHAR* end = state->end;
	SRE_CHAR* ptr = state->ptr;
	int stacksize;
	int stackbase;
	int i, count;

	for (;;) {

		TRACE(("[%p]\n", pattern));

		switch (*pattern++) {

		case SRE_OP_FAILURE:
			/* immediate failure */
			TRACE(("%8d: failure\n", PTR(ptr)));
			return 0;

		case SRE_OP_SUCCESS:
			/* end of pattern */
			TRACE(("%8d: success\n", PTR(ptr)));
			state->ptr = ptr;
			return 1;

		case SRE_OP_AT:
			/* match at given position */
			TRACE(("%8d: match at \\%c\n", PTR(ptr), *pattern));
			if (!SRE_AT(state, ptr, *pattern))
				return 0;
			pattern++;
			break;

		case SRE_OP_LITERAL:
			/* match literal character */
			/* args: <code> */
			TRACE(("%8d: literal %c\n", PTR(ptr), (SRE_CHAR) *pattern));
			if (ptr >= end || *ptr != (SRE_CHAR) *pattern)
				return 0;
			pattern++;
			ptr++;
			break;

		case SRE_OP_NOT_LITERAL:
			/* match anything that is not literal character */
			/* args: <code> */
			TRACE(("%8d: literal not %c\n", PTR(ptr), (SRE_CHAR) *pattern));
			if (ptr >= end || *ptr == (SRE_CHAR) *pattern)
				return 0;
			pattern++;
			ptr++;
			break;

		case SRE_OP_ANY:
			/* match anything */
			TRACE(("%8d: any\n", PTR(ptr)));
			if (ptr >= end)
				return 0;
			ptr++;
			break;

		case SRE_OP_IN:
			/* match set member (or non_member) */
			/* args: <skip> <set> */
			TRACE(("%8d: set %c\n", PTR(ptr), *ptr));
			if (ptr >= end || !SRE_MEMBER(pattern + 1, *ptr))
				return 0;
			pattern += pattern[0];
			ptr++;
			break;

		case SRE_OP_GROUP:
			/* match backreference */
			i = pattern[0];
			{
				/* FIXME: optimize size! */
				SRE_CHAR* p = (SRE_CHAR*) state->mark[i+i];
				SRE_CHAR* e = (SRE_CHAR*) state->mark[i+i+1];
				if (!p || !e || e < p)
					return 0;
				while (p < e) {
					if (ptr >= end || *ptr != *p)
						return 0;
					p++; ptr++;
				}
			}
			pattern++;
			break;

		case SRE_OP_LITERAL_IGNORE:
			TRACE(("%8d: literal lower(%c)\n", PTR(ptr), (SRE_CHAR) *pattern));
			if (ptr >= end || SRE_TO_LOWER(*ptr) != (SRE_CHAR) *pattern)
				return 0;
			pattern++;
			ptr++;
			break;

		case SRE_OP_NOT_LITERAL_IGNORE:
			TRACE(("%8d: literal not lower(%c)\n", PTR(ptr),
				   (SRE_CHAR) *pattern));
			if (ptr >= end || SRE_TO_LOWER(*ptr) == (SRE_CHAR) *pattern)
				return 0;
			pattern++;
			ptr++;
			break;

		case SRE_OP_IN_IGNORE:
			TRACE(("%8d: set lower(%c)\n", PTR(ptr), *ptr));
			if (ptr >= end
				|| !SRE_MEMBER(pattern+1, (SRE_CHAR) SRE_TO_LOWER(*ptr)))
				return 0;
			pattern += pattern[0];
			ptr++;
			break;

		case SRE_OP_MARK:
			/* set mark */
			/* args: <mark> */
			TRACE(("%8d: set mark(%d)\n", PTR(ptr), pattern[0]));
			state->mark[pattern[0]] = ptr;
			pattern++;
			break;

		case SRE_OP_JUMP:
			/* jump forward */
			/* args: <skip> */
			TRACE(("%8d: jump +%d\n", PTR(ptr), pattern[0]));
			pattern += pattern[0];
			break;

		case SRE_OP_CALL:
			/* match subpattern, without backtracking */
			/* args: <skip> <pattern> */
			TRACE(("%8d: match subpattern\n", PTR(ptr)));
			state->ptr = ptr;
			if (!SRE_MATCH(state, pattern + 1))
				return 0;
			pattern += pattern[0];
			ptr = state->ptr;
			break;

		case SRE_OP_MAX_REPEAT_ONE:

			/* match repeated sequence (maximizing regexp).	 this
			   variant only works if the repeated item is exactly one
			   character wide, and we're not already collecting
			   backtracking points.	 for other cases, use the
			   MAX_REPEAT operator instead */

			/* args: <skip> <min> <max> <step> */

			TRACE(("%8d: max repeat one {%d,%d}\n", PTR(ptr),
				   pattern[1], pattern[2]));

			count = 0;

			if (pattern[3] == SRE_OP_ANY) {
				/* repeated wildcard.  skip to the end of the target
				   string, and backtrack from there */
				/* FIXME: must look for line endings */
				if (ptr + pattern[1] > end)
					return 0; /* cannot match */
				count = pattern[2];
				if (count > end - ptr)
					count = end - ptr;
				ptr += count;

			} else if (pattern[3] == SRE_OP_LITERAL) {
				/* repeated literal */
				SRE_CHAR chr = (SRE_CHAR) pattern[4];
				while (count < (int) pattern[2]) {
					if (ptr >= end || *ptr != chr)
						break;
					ptr++;
					count++;
				}

			} else if (pattern[3] == SRE_OP_LITERAL_IGNORE) {
				/* repeated literal */
				SRE_CHAR chr = (SRE_CHAR) pattern[4];
				while (count < (int) pattern[2]) {
					if (ptr >= end || (SRE_CHAR) SRE_TO_LOWER(*ptr) != chr)
						break;
					ptr++;
					count++;
				}

			} else if (pattern[3] == SRE_OP_NOT_LITERAL) {
				/* repeated non-literal */
				SRE_CHAR chr = (SRE_CHAR) pattern[4];
				while (count < (int) pattern[2]) {
					if (ptr >= end || *ptr == chr)
						break;
					ptr++;
					count++;
				}

			} else if (pattern[3] == SRE_OP_NOT_LITERAL_IGNORE) {
				/* repeated non-literal */
				SRE_CHAR chr = (SRE_CHAR) pattern[4];
				while (count < (int) pattern[2]) {
					if (ptr >= end || (SRE_CHAR) SRE_TO_LOWER(*ptr) == chr)
						break;
					ptr++;
					count++;
				}

			} else if (pattern[3] == SRE_OP_IN) {
				/* repeated set */
				while (count < (int) pattern[2]) {
					if (ptr >= end || !SRE_MEMBER(pattern + 5, *ptr))
						break;
					ptr++;
					count++;
				}

			} else {
				/* repeated single character pattern */
				state->ptr = ptr;
				while (count < (int) pattern[2]) {
					i = SRE_MATCH(state, pattern + 3);
					if (i < 0)
						return i;
					if (i == 0)
						break;
					count++;
				}
				state->ptr = ptr;
				ptr += count;
			}

			/* when we arrive here, count contains the number of
			   matches, and ptr points to the tail of the target
			   string.	check if the rest of the pattern matches, and
			   backtrack if not. */

			/* FIXME: <fl> this is a mess.	fix it! */

			TRACE(("%8d: repeat %d found\n", PTR(ptr), count));

			if (count < (int) pattern[1])
				return 0;

			if (pattern[pattern[0]] == SRE_OP_SUCCESS) {
				/* tail is empty.  we're finished */
				TRACE(("%8d: tail is empty\n", PTR(ptr)));
				state->ptr = ptr;
				return 1;

			} else if (pattern[pattern[0]] == SRE_OP_LITERAL) {
				/* tail starts with a literal.	we can speed things up
				   by skipping positions where the rest of the pattern
				   cannot possibly match */
				SRE_CHAR chr = (SRE_CHAR) pattern[pattern[0]+1];
				TRACE(("%8d: tail is literal %d\n", PTR(ptr), chr));
				for (;;) {
					TRACE(("%8d: scan for tail match\n", PTR(ptr)));
					while (count >= (int) pattern[1] &&
						   (ptr >= end || *ptr != chr)) {
						ptr--;
						count--;
					}
					TRACE(("%8d: check tail\n", PTR(ptr)));
					if (count < (int) pattern[1])
						break;
					state->ptr = ptr;
					i = SRE_MATCH(state, pattern + pattern[0]);
					if (i > 0) {
						TRACE(("%8d: repeat %d picked\n", PTR(ptr), count));
						return 1;
					}
					TRACE(("%8d: BACKTRACK\n", PTR(ptr)));
					ptr--;
					count--;
				}

			} else {
				TRACE(("%8d: tail is pattern\n", PTR(ptr)));
				while (count >= (int) pattern[1]) {
					state->ptr = ptr;
					i = SRE_MATCH(state, pattern + pattern[0]);
					if (i > 0) {
						TRACE(("%8d: repeat %d picked\n", PTR(ptr), count));
						return 1;
					}
					TRACE(("%8d: BACKTRACK\n", PTR(ptr)));
					ptr--;
					count--;
				}
			}
			return 0; /* failure! */

/* ----------------------------------------------------------------------- */
/* FIXME: the following section is just plain broken */

		case SRE_OP_MAX_REPEAT:
			/* match repeated sequence (maximizing regexp).	 repeated
			   group should end with a MAX_UNTIL code */

			TRACE(("%8d: max repeat %d %d\n", PTR(ptr),
				   pattern[1], pattern[2]));

			count = 0;
			state->ptr = ptr;

			/* FIXME: <fl> umm.	 what about matching the minimum
			   number of items before starting to collect backtracking
			   positions? */

			stackbase = state->stackbase;

			while (count < (int) pattern[2]) {
				/* store current position on the stack */
				TRACE(("%8d: push mark at index %d\n", PTR(ptr), count));
				if (stackbase + count >= state->stacksize) {
					i = _stack_extend(state, stackbase + count + 1,
									  stackbase + pattern[2]);
					if (i < 0)
						return i;
				}
				state->stack[stackbase + count] = ptr;
				/* check if we can match another item */
				state->stackbase += count + 1;
				i = SRE_MATCH(state, pattern + 3);
				state->stackbase = stackbase; /* rewind */
				if (i != 2)
					break;
				if (state->ptr == ptr) {
					/* if the match was successful but empty, set the
					   count to max and terminate the scanning loop */
					stacksize = count; /* actual size of stack */
					count = (int) pattern[2];
					goto check_tail; /* FIXME: <fl> eliminate goto */
				}
				count++;
				ptr = state->ptr;

			}

			stacksize = count; /* actual number of entries on the stack */

		  check_tail:

			/* when we get here, count is the number of matches,
			   stacksize is the number of match points on the stack
			   (usually same as count, but it might be smaller) and
			   ptr points to the tail. */

			if (count < (int) pattern[1])
				return 0;

			/* make sure that rest of the expression matches.  if it
			   doesn't, backtrack */

			TRACE(("%8d: repeat %d found (stack size = %d)\n", PTR(ptr),
				   count, stacksize + 1));

			TRACE(("%8d: tail is pattern\n", PTR(ptr)));

			/* hope for the best */
			state->ptr = ptr;
			state->stackbase += stacksize + 1;
			i = SRE_MATCH(state, pattern + pattern[0]);
			state->stackbase = stackbase;
			if (i > 0) {
				TRACE(("%8d: repeat %d picked\n", PTR(ptr), count));
				return 1;
			}

			/* backtrack! */
			while (count >= (int) pattern[1]) {
				ptr = state->stack[stackbase + (count < stacksize ? count : stacksize)];
				state->ptr = ptr;
				count--;
				TRACE(("%8d: BACKTRACK\n", PTR(ptr)));
				state->stackbase += stacksize + 1;
				i = SRE_MATCH(state, pattern + pattern[0]);
				state->stackbase = stackbase;
				if (i > 0) {
					TRACE(("%8d: repeat %d picked\n", PTR(ptr), count));
					return 1;
				}
			}
			return 0; /* failure! */

		case SRE_OP_MAX_UNTIL:
			/* match repeated sequence (maximizing regexp).	 repeated
			   group should end with a MAX_UNTIL code */

			TRACE(("%8d: max until\n", PTR(ptr)));
			state->ptr = ptr;
			return 2; /* always succeeds, for now... */

/* end of totally broken section */
/* ----------------------------------------------------------------------- */

		case SRE_OP_MIN_REPEAT:
			/* match repeated sequence (minimizing regexp) */
			TRACE(("%8d: min repeat %d %d\n", PTR(ptr),
				   pattern[1], pattern[2]));
			count = 0;
			state->ptr = ptr;
			/* match minimum number of items */
			while (count < (int) pattern[1]) {
				i = SRE_MATCH(state, pattern + 3);
				if (i <= 0)
					return i;
				count++;
			}
			/* move forward until the tail matches. */
			while (count <= (int) pattern[2]) {
				ptr = state->ptr;
				i = SRE_MATCH(state, pattern + pattern[0]);
				if (i > 0) {
					TRACE(("%8d: repeat %d picked\n", PTR(ptr), count));
					return 1;
				}
				TRACE(("%8d: BACKTRACK\n", PTR(ptr)));
				state->ptr = ptr; /* backtrack */
				i = SRE_MATCH(state, pattern + 3);
				if (i <= 0)
					return i;
				count++;
			}
			return 0; /* failure! */

		case SRE_OP_MIN_UNTIL:
			/* end of repeat group */
			TRACE(("%8d: min until\n", PTR(ptr)));
			state->ptr = ptr;
			return 2; /* always succeeds, for now... */

		case SRE_OP_BRANCH:
			/* match one of several subpatterns */
			/* format: <branch> <size> <head> ... <null> <tail> */
			TRACE(("%8d: branch\n", PTR(ptr)));
			while (*pattern) {
				if (pattern[1] != SRE_OP_LITERAL ||
					(ptr < end && *ptr == (SRE_CHAR) pattern[2])) {
					TRACE(("%8d: branch check\n", PTR(ptr)));
					state->ptr = ptr;
					i = SRE_MATCH(state, pattern + 1);
					if (i > 0) {
						TRACE(("%8d: branch succeeded\n", PTR(ptr)));
						return 1;
					}
				}
				pattern += *pattern;
			}
			TRACE(("%8d: branch failed\n", PTR(ptr)));
			return 0; /* failure! */

		case SRE_OP_REPEAT:
			/* TEMPLATE: match repeated sequence (no backtracking) */
			/* format: <repeat> <skip> <min> <max> */
			TRACE(("%8d: repeat %d %d\n", PTR(ptr), pattern[1], pattern[2]));
			count = 0;
			state->ptr = ptr;
			while (count < (int) pattern[2]) {
				i = SRE_MATCH(state, pattern + 3);
				if (i <= 0)
					break;
				count++;
			}
			if (count <= (int) pattern[1])
				return 0;
			TRACE(("%8d: repeat %d matches\n", PTR(ptr), count));
			pattern += pattern[0];
			ptr = state->ptr;
			break;

		default:
			return SRE_ERROR_ILLEGAL;
		}
	}
}

LOCAL(int)
SRE_SEARCH(SRE_STATE* state, SRE_CODE* pattern)
{
	SRE_CHAR* ptr = state->start;
	SRE_CHAR* end = state->end;
	int status = 0;

	/* FIXME: <fl> add IGNORE cases (or implement full ASSERT support?	*/

	if (pattern[0] == SRE_OP_LITERAL) {
		/* pattern starts with a literal */
		SRE_CHAR chr = (SRE_CHAR) pattern[1];
		for (;;) {
			while (ptr < end && *ptr != chr)
				ptr++;
			if (ptr == end)
				return 0;
			TRACE(("%8d: search found literal\n", PTR(ptr)));
			state->start = ptr;
			state->ptr = ++ptr;
			status = SRE_MATCH(state, pattern + 2);
			if (status != 0)
				break;
		}

	} else if (pattern[0] == SRE_OP_IN) {
		/* pattern starts with a set */
		for (;;) {
			/* format: <in> <skip> <data> */
			while (ptr < end && !SRE_MEMBER(pattern + 2, *ptr))
				ptr++;
			if (ptr == end)
				return 0;
			TRACE(("%8d: search found set\n", PTR(ptr)));
			state->start = ptr;
			state->ptr = ++ptr;
			status = SRE_MATCH(state, pattern + pattern[1] + 1);
			if (status != 0)
				break;
		}

	} else
		while (ptr <= end) {
			TRACE(("%8d: search\n", PTR(ptr))); 
			state->start = state->ptr = ptr++;
			status = SRE_MATCH(state, pattern);
			if (status != 0)
				break;
		}

	return status;
}

#ifndef SRE_RECURSIVE

/* -------------------------------------------------------------------- */
/* factories and destructors */

/* see sre.h for object declarations */

staticforward PyTypeObject Pattern_Type;
staticforward PyTypeObject Match_Type;

static PyObject *
_compile(PyObject* self_, PyObject* args)
{
	/* "compile" pattern descriptor to pattern object */

	PatternObject* self;

	PyObject* pattern;
	PyObject* code;
	int groups = 0;
	PyObject* groupindex = NULL;
	if (!PyArg_ParseTuple(args, "OO!|iO", &pattern,
                          &PyString_Type, &code, &groups, &groupindex))
		return NULL;

	self = PyObject_NEW(PatternObject, &Pattern_Type);
	if (self == NULL)
		return NULL;

	Py_INCREF(pattern);
	self->pattern = pattern;

	Py_INCREF(code);
	self->code = code;

	self->groups = groups;

	Py_XINCREF(groupindex);
	self->groupindex = groupindex;

	return (PyObject*) self;
}

static PyObject *
_getcodesize(PyObject* self_, PyObject* args)
{
	return Py_BuildValue("i", sizeof(SRE_CODE));
}

static PyObject*
_pattern_new_match(PatternObject* pattern, SRE_STATE* state,
                   PyObject* string, int status)
{
	/* create match object (from state object) */

	MatchObject* match;
	int i, j;

	TRACE(("status = %d\n", status));

	if (status > 0) {

		/* create match object (with room for extra group marks) */
		match = PyObject_NEW_VAR(MatchObject, &Match_Type, 2*pattern->groups);
		if (match == NULL)
			return NULL;

		Py_INCREF(pattern);
		match->pattern = pattern;

		Py_INCREF(string);
		match->string = string;

		match->groups = pattern->groups+1;

		/* group zero */
		match->mark[0] = ((char*) state->start -
						  (char*) state->beginning) / state->charsize;
		match->mark[1] = ((char*) state->ptr -
						  (char*) state->beginning) / state->charsize;

		/* fill in the rest of the groups */
		for (i = j = 0; i < pattern->groups; i++, j+=2)
			if (state->mark[j] != NULL && state->mark[j+1] != NULL) {
				match->mark[j+2] = ((char*) state->mark[j] -
									(char*) state->beginning) / state->charsize;
				match->mark[j+3] = ((char*) state->mark[j+1] -
									(char*) state->beginning) / state->charsize;
			} else
				match->mark[j+2] = match->mark[j+3] = -1; /* undefined */

		return (PyObject*) match;

	} else if (status < 0) {

		/* internal error */
		PyErr_SetString(
			PyExc_RuntimeError, "internal error in regular expression engine"
			);
		return NULL;

	}

	Py_INCREF(Py_None);
	return Py_None;
}

/* -------------------------------------------------------------------- */
/* pattern methods */

LOCAL(PyObject*)
_setup(SRE_STATE* state, PyObject* args)
{
	/* prepare state object */

	PyBufferProcs *buffer;
	int i, count;
	void* ptr;

	PyObject* string;
	int start = 0;
	int end = INT_MAX;
	if (!PyArg_ParseTuple(args, "O|ii", &string, &start, &end))
		return NULL;

	/* get pointer to string buffer */
	buffer = string->ob_type->tp_as_buffer;
	if (!buffer || !buffer->bf_getreadbuffer || !buffer->bf_getsegcount ||
		buffer->bf_getsegcount(string, NULL) != 1) {
		PyErr_SetString(PyExc_TypeError, "expected read-only buffer");
		return NULL;
	}

	/* determine buffer size */
	count = buffer->bf_getreadbuffer(string, 0, &ptr);
	if (count < 0) {
		/* sanity check */
		PyErr_SetString(PyExc_TypeError, "buffer has negative size");
		return NULL;
	}

	/* determine character size */
	state->charsize = (PyUnicode_Check(string) ? sizeof(Py_UNICODE) : 1);

	count /= state->charsize;

	/* adjust boundaries */
	if (start < 0)
		start = 0;
	else if (start > count)
		start = count;

	if (end < 0)
		end = 0;
	else if (end > count)
		end = count;

	state->beginning = ptr;

	state->start = (void*) ((char*) ptr + start * state->charsize);
	state->end = (void*) ((char*) ptr + end * state->charsize);

	/* FIXME: dynamic! */
	for (i = 0; i < 64; i++)
		state->mark[i] = NULL;

	state->stack = NULL;
	state->stackbase = 0;
	state->stacksize = 0;

	return string;
}

static void
_pattern_dealloc(PatternObject* self)
{
	Py_XDECREF(self->code);
	Py_XDECREF(self->pattern);
	Py_XDECREF(self->groupindex);
	PyMem_DEL(self);
}

static PyObject*
_pattern_match(PatternObject* self, PyObject* args)
{
	SRE_STATE state;
	PyObject* string;
	int status;

	string = _setup(&state, args);
	if (!string)
		return NULL;

	state.ptr = state.start;

	if (state.charsize == 1) {
		status = sre_match(&state, PatternObject_GetCode(self));
	} else {
		status = sre_umatch(&state, PatternObject_GetCode(self));
	}

	_stack_free(&state);

	return _pattern_new_match(self, &state, string, status);
}

static PyObject*
_pattern_search(PatternObject* self, PyObject* args)
{
	SRE_STATE state;
	PyObject* string;
	int status;

	string = _setup(&state, args);
	if (!string)
		return NULL;

	if (state.charsize == 1) {
		status = sre_search(&state, PatternObject_GetCode(self));
	} else {
		status = sre_usearch(&state, PatternObject_GetCode(self));
	}

	_stack_free(&state);

	return _pattern_new_match(self, &state, string, status);
}

static PyObject*
_pattern_findall(PatternObject* self, PyObject* args)
{
	/* FIXME: not sure about the semantics here.  this is good enough
	   for SXP, though... */

	SRE_STATE state;
	PyObject* string;
	PyObject* list;
	int status;

	string = _setup(&state, args);
	if (!string)
		return NULL;

	list = PyList_New(0);

	while (state.start < state.end) {

		PyObject* item;
		
		state.ptr = state.start;

		if (state.charsize == 1) {
			status = sre_match(&state, PatternObject_GetCode(self));
		} else {
			status = sre_umatch(&state,	PatternObject_GetCode(self));
		}

		if (status >= 0) {

			if (status == 0)
				state.ptr = (void*) ((char*) state.start + 1);

			item = PySequence_GetSlice(
				string,
				((char*) state.start - (char*) state.beginning),
				((char*) state.ptr - (char*) state.beginning));
			if (!item)
				goto error;
			if (PyList_Append(list, item) < 0)
				goto error;

			state.start = state.ptr;

		} else {

			/* internal error */
			PyErr_SetString(
				PyExc_RuntimeError,
				"internal error in regular expression engine"
				);
			goto error;

		}
	}

	_stack_free(&state);

	return list;

error:
	_stack_free(&state);
	return NULL;
	
}

static PyMethodDef _pattern_methods[] = {
	{"match", (PyCFunction) _pattern_match, 1},
	{"search", (PyCFunction) _pattern_search, 1},
	{"findall", (PyCFunction) _pattern_findall, 1},
	{NULL, NULL}
};

static PyObject*  
_pattern_getattr(PatternObject* self, char* name)
{
    PyObject* res;

	res = Py_FindMethod(_pattern_methods, (PyObject*) self, name);

	if (res)
		return res;

	PyErr_Clear();

    /* attributes */
	if (!strcmp(name, "pattern")) {
        Py_INCREF(self->pattern);
		return self->pattern;
    }
    
	PyErr_SetString(PyExc_AttributeError, name);
	return NULL;
}

statichere PyTypeObject Pattern_Type = {
	PyObject_HEAD_INIT(NULL)
	0, "Pattern", sizeof(PatternObject), 0,
	(destructor)_pattern_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)_pattern_getattr, /*tp_getattr*/
};

/* -------------------------------------------------------------------- */
/* match methods */

static void
_match_dealloc(MatchObject* self)
{
	Py_XDECREF(self->string);
	Py_DECREF(self->pattern);
	PyMem_DEL(self);
}

static PyObject*
getslice_by_index(MatchObject* self, int index)
{
	if (index < 0 || index >= self->groups) {
		/* raise IndexError if we were given a bad group number */
		PyErr_SetString(
			PyExc_IndexError,
			"no such group"
			);
		return NULL;
	}

	if (self->string == Py_None || self->mark[index+index] < 0) {
		/* return None if the string or group is undefined */
		Py_INCREF(Py_None);
		return Py_None;
	}

	return PySequence_GetSlice(
		self->string, self->mark[index+index], self->mark[index+index+1]
		);
}

static PyObject*
getslice(MatchObject* self, PyObject* index)
{
	if (!PyInt_Check(index) && self->pattern->groupindex != NULL) {
		/* FIXME: resource leak? */
		index = PyObject_GetItem(self->pattern->groupindex, index);
		if (!index)
			return NULL;
	}

	if (PyInt_Check(index))
		return getslice_by_index(self, (int) PyInt_AS_LONG(index));

	return getslice_by_index(self, -1); /* signal error */
}

static PyObject*
_match_group(MatchObject* self, PyObject* args)
{
	PyObject* result;
	int i, size;

	size = PyTuple_GET_SIZE(args);

	switch (size) {
	case 0:
		result = getslice(self, Py_False); /* force error */
		break;
	case 1:
		result = getslice(self, PyTuple_GET_ITEM(args, 0));
		break;
	default:
		/* fetch multiple items */
		result = PyTuple_New(size);
		if (!result)
			return NULL;
		for (i = 0; i < size; i++) {
			PyObject* item = getslice(self, PyTuple_GET_ITEM(args, i));
			if (!item) {
				Py_DECREF(result);
				return NULL;
			}
			PyTuple_SET_ITEM(result, i, item);
		}
		break;
	}
	return result;
}

static PyObject*
_match_groups(MatchObject* self, PyObject* args)
{
	PyObject* result;
	int index;

	result = PyTuple_New(self->groups-1);
	if (!result)
		return NULL;

	for (index = 1; index < self->groups; index++) {
		PyObject* item;
		/* FIXME: <fl> handle default! */
		item = getslice_by_index(self, index);
		if (!item) {
			Py_DECREF(result);
			return NULL;
		}
		PyTuple_SET_ITEM(result, index-1, item);
	}

	return result;
}

static PyObject*
_match_groupdict(MatchObject* self, PyObject* args)
{
	PyObject* result;
	PyObject* keys;
	int index;

	result = PyDict_New();
	if (!result)
		return NULL;
	if (!self->pattern->groupindex)
		return result;

	keys = PyMapping_Keys(self->pattern->groupindex);
	if (!keys)
		return NULL;

	for (index = 0; index < PySequence_Length(keys); index++) {
		PyObject* key;
		PyObject* item;
		key = PySequence_GetItem(keys, index);
		if (!key) {
			Py_DECREF(keys);
			Py_DECREF(result);
			return NULL;
		}
		item = getslice(self, key);
		if (!item) {
			Py_DECREF(key);
			Py_DECREF(keys);
			Py_DECREF(result);
			return NULL;
		}
		/* FIXME: <fl> this can fail, right? */
		PyDict_SetItem(result, key, item);
	}

	Py_DECREF(keys);

	return result;
}

static PyObject*
_match_start(MatchObject* self, PyObject* args)
{
	int index = 0;
	if (!PyArg_ParseTuple(args, "|i", &index))
		return NULL;

	if (index < 0 || index >= self->groups) {
		PyErr_SetString(
			PyExc_IndexError,
			"no such group"
			);
		return NULL;
	}

	if (self->mark[index*2] < 0) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	return Py_BuildValue("i", self->mark[index*2]);
}

static PyObject*
_match_end(MatchObject* self, PyObject* args)
{
	int index = 0;
	if (!PyArg_ParseTuple(args, "|i", &index))
		return NULL;

	if (index < 0 || index >= self->groups) {
		PyErr_SetString(
			PyExc_IndexError,
			"no such group"
			);
		return NULL;
	}

	if (self->mark[index*2] < 0) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	return Py_BuildValue("i", self->mark[index*2+1]);
}

static PyObject*
_match_span(MatchObject* self, PyObject* args)
{
	int index = 0;
	if (!PyArg_ParseTuple(args, "|i", &index))
		return NULL;

	if (index < 0 || index >= self->groups) {
		PyErr_SetString(
			PyExc_IndexError,
			"no such group"
			);
		return NULL;
	}

	if (self->mark[index*2] < 0) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	return Py_BuildValue("ii", self->mark[index*2], self->mark[index*2+1]);
}

static PyMethodDef _match_methods[] = {
	{"group", (PyCFunction) _match_group, 1},
	{"start", (PyCFunction) _match_start, 1},
	{"end", (PyCFunction) _match_end, 1},
	{"span", (PyCFunction) _match_span, 1},
	{"groups", (PyCFunction) _match_groups, 1},
	{"groupdict", (PyCFunction) _match_groupdict, 1},
	{NULL, NULL}
};

static PyObject*  
_match_getattr(MatchObject* self, char* name)
{
	PyObject* res;

	res = Py_FindMethod(_match_methods, (PyObject*) self, name);
	if (res)
		return res;

	PyErr_Clear();

	/* attributes! */
	if (!strcmp(name, "string")) {
        Py_INCREF(self->string);
		return self->string;
    }
	if (!strcmp(name, "regs"))
		/* FIXME: should return the whole list! */
		return Py_BuildValue("((i,i))", self->mark[0], self->mark[1]);
	if (!strcmp(name, "re")) {
        Py_INCREF(self->pattern);
		return (PyObject*) self->pattern;
    }
	if (!strcmp(name, "groupindex") && self->pattern->groupindex) {
        Py_INCREF(self->pattern->groupindex);
		return self->pattern->groupindex;
    }
	if (!strcmp(name, "pos"))
		return Py_BuildValue("i", 0); /* FIXME */
	if (!strcmp(name, "endpos"))
		return Py_BuildValue("i", 0); /* FIXME */

	PyErr_SetString(PyExc_AttributeError, name);
	return NULL;
}

/* FIXME: implement setattr("string", None) as a special case (to
   detach the associated string, if any */

statichere PyTypeObject Match_Type = {
	PyObject_HEAD_INIT(NULL)
	0, "Match",
	sizeof(MatchObject), /* size of basic object */
	sizeof(int), /* space for group item */
	(destructor)_match_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)_match_getattr, /*tp_getattr*/
};

static PyMethodDef _functions[] = {
	{"compile", _compile, 1},
	{"getcodesize", _getcodesize, 1},
	{NULL, NULL}
};

void
#ifdef WIN32
__declspec(dllexport)
#endif
init_sre()
{
	/* Patch object types */
	Pattern_Type.ob_type = Match_Type.ob_type = &PyType_Type;

	Py_InitModule("_sre", _functions);
}

#endif
