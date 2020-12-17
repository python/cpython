/*
 * tkMacOSXDebug.c --
 *
 *	Implementation of Macintosh specific functions for debugging MacOS
 *	events, regions, etc...
 *
 * Copyright 2001-2009, Apple Inc.
 * Copyright (c) 2006-2009 Daniel A. Steffen <das@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkMacOSXPrivate.h"
#include "tkMacOSXDebug.h"

#ifdef TK_MAC_DEBUG

#include <mach-o/dyld.h>
#include <mach-o/nlist.h>

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXGetNamedDebugSymbol --
 *
 *	Dynamically acquire address of a named symbol from a loaded dynamic
 *	library, so that we can use API that may not be available on all OS
 *	versions. For debugging purposes, if we cannot find the symbol with
 *	the usual dynamic library APIs, we manually walk the symbol table of
 *	the loaded library. This allows access to unexported symbols such as
 *	private_extern internal debugging functions. If module is NULL or the
 *	empty string, search all loaded libraries (could be very expensive and
 *	should be avoided).
 *
 *	THIS FUCTION IS ONLY TO BE USED FOR DEBUGGING PURPOSES, IT MAY BREAK
 *	UNEXPECTEDLY IN THE FUTURE!
 *
 * Results:
 *	Address of given symbol or NULL if unavailable.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

MODULE_SCOPE void *
TkMacOSXGetNamedDebugSymbol(
    const char *module,
    const char *symbol)
{
    void *addr = TkMacOSXGetNamedSymbol(module, symbol);

#ifndef __LP64__
    if (!addr) {
	const struct mach_header *mh = NULL;
	uint32_t i, n = _dyld_image_count();
	size_t module_len = 0;

	if (module && *module) {
	    module_len = strlen(module);
	}
	for (i = 0; i < n; i++) {
	    if (module && *module) {
		/* Find image with given module name */
		char *name;
		const char *path = _dyld_get_image_name(i);

		if (!path) {
		    continue;
		}
		name = strrchr(path, '/') + 1;
		if (strncmp(name, module, module_len) != 0) {
		    continue;
		}
	    }
	    mh = _dyld_get_image_header(i);
	    if (mh) {
		struct load_command *lc;
		struct symtab_command *st = NULL;
		struct segment_command *sg = NULL;
		uint32_t j, m, nsect = 0, txtsectx = 0;

		lc = (struct load_command*)((const char*) mh +
			sizeof(struct mach_header));
		m = mh->ncmds;
		for (j = 0; j < m; j++) {
		    /* Find symbol table and index of __text section */
		    if (lc->cmd == LC_SEGMENT) {
			/* Find last segment before symbol table */
			sg = (struct segment_command*) lc;
			if (!txtsectx) {
			    /* Count total sections until (__TEXT, __text) */
			    uint32_t k, ns = sg->nsects;

			    if (strcmp(sg->segname, SEG_TEXT) == 0) {
				struct section *s = (struct section *)(
					(char *)sg +
					sizeof(struct segment_command));

				for(k = 0; k < ns; k++) {
				    if (strcmp(s->sectname, SECT_TEXT) == 0) {
					txtsectx = nsect+k+1;
					break;
				    }
				    s++;
				}
			    }
			    nsect += ns;
			}
		    } else if (!st && lc->cmd == LC_SYMTAB) {
			st = (struct symtab_command *) lc;
			break;
		    }
		    lc = (struct load_command *)((char *) lc + lc->cmdsize);
		}
		if (st && sg && txtsectx) {
		    intptr_t base, slide = _dyld_get_image_vmaddr_slide(i);
		    char *strings;
		    struct nlist *sym;
		    uint32_t strsize = st->strsize;
		    int32_t strx;

		    /*
		     * Offset file positions by difference to actual position
		     * in memory of last segment before symbol table:
		     */

		    base = (intptr_t) sg->vmaddr + slide - sg->fileoff;
		    strings = (char *) (base + st->stroff);
		    sym = (struct nlist *) (base + st->symoff);
		    m = st->nsyms;
		    for (j = 0; j < m; j++) {
			/* Find symbol with given name in __text section */
			strx = sym->n_un.n_strx;
			if ((sym->n_type & N_TYPE) == N_SECT &&
				sym->n_sect == txtsectx &&
				strx > 0 && (uint32_t) strx < strsize &&
				strcmp(strings + strx, symbol) == 0) {
			    addr = (char*) sym->n_value + slide;
			    break;
			}
			sym++;
		    }
		}
	    }
	    if (module && *module) {
		/* If given a module name, only search corresponding image */
		break;
	    }
	}
    }
#endif /* __LP64__ */
    return addr;
}
#endif /* TK_MAC_DEBUG */

/*
 * Local Variables:
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */
