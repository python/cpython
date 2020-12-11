/*
 * Tag tables.  3/4-baked, work in progress.
 *
 * Copyright (C) 2005, Joe English.  Freely redistributable.
 */

#include <string.h>	/* for memset() */
#include <tcl.h>
#include <tk.h>

#include "ttkTheme.h"
#include "ttkWidget.h"

/*------------------------------------------------------------------------
 * +++ Internal data structures.
 */
struct TtkTag {
    int 	priority;		/* 1=>highest */
    const char	*tagName;		/* Back-pointer to hash table entry */
    void	*tagRecord;		/* User data */
};

struct TtkTagTable {
    Tk_Window		tkwin;		/* owner window */
    Tk_OptionSpec	*optionSpecs;	/* ... */
    Tk_OptionTable	optionTable;	/* ... */
    int         	recordSize;	/* size of tag record */
    int 		nTags;		/* #tags defined so far */
    Tcl_HashTable	tags;		/* defined tags */
};

/*------------------------------------------------------------------------
 * +++ Tags.
 */
static Ttk_Tag NewTag(Ttk_TagTable tagTable, const char *tagName)
{
    Ttk_Tag tag = ckalloc(sizeof(*tag));
    tag->tagRecord = ckalloc(tagTable->recordSize);
    memset(tag->tagRecord, 0, tagTable->recordSize);
    /* Don't need Tk_InitOptions() here, all defaults should be NULL. */
    tag->priority = ++tagTable->nTags;
    tag->tagName = tagName;
    return tag;
}

static void DeleteTag(Ttk_TagTable tagTable, Ttk_Tag tag)
{
    Tk_FreeConfigOptions(tag->tagRecord,tagTable->optionTable,tagTable->tkwin);
    ckfree(tag->tagRecord);
    ckfree(tag);
}

/*------------------------------------------------------------------------
 * +++ Tag tables.
 */

Ttk_TagTable Ttk_CreateTagTable(
    Tcl_Interp *interp, Tk_Window tkwin,
    Tk_OptionSpec optionSpecs[], int recordSize)
{
    Ttk_TagTable tagTable = ckalloc(sizeof(*tagTable));
    tagTable->tkwin = tkwin;
    tagTable->optionSpecs = optionSpecs;
    tagTable->optionTable = Tk_CreateOptionTable(interp, optionSpecs);
    tagTable->recordSize = recordSize;
    tagTable->nTags = 0;
    Tcl_InitHashTable(&tagTable->tags, TCL_STRING_KEYS);
    return tagTable;
}

void Ttk_DeleteTagTable(Ttk_TagTable tagTable)
{
    Tcl_HashSearch search;
    Tcl_HashEntry *entryPtr;

    entryPtr = Tcl_FirstHashEntry(&tagTable->tags, &search);
    while (entryPtr != NULL) {
	DeleteTag(tagTable, Tcl_GetHashValue(entryPtr));
	entryPtr = Tcl_NextHashEntry(&search);
    }

    Tcl_DeleteHashTable(&tagTable->tags);
    ckfree(tagTable);
}

Ttk_Tag Ttk_GetTag(Ttk_TagTable tagTable, const char *tagName)
{
    int isNew = 0;
    Tcl_HashEntry *entryPtr = Tcl_CreateHashEntry(
	&tagTable->tags, tagName, &isNew);

    if (isNew) {
	tagName = Tcl_GetHashKey(&tagTable->tags, entryPtr);
	Tcl_SetHashValue(entryPtr, NewTag(tagTable,tagName));
    }
    return Tcl_GetHashValue(entryPtr);
}

Ttk_Tag Ttk_GetTagFromObj(Ttk_TagTable tagTable, Tcl_Obj *objPtr)
{
    return Ttk_GetTag(tagTable, Tcl_GetString(objPtr));
}

/*------------------------------------------------------------------------
 * +++ Tag sets.
 */

/* Ttk_GetTagSetFromObj --
 * 	Extract an array of pointers to Ttk_Tags from a Tcl_Obj.
 * 	objPtr may be NULL, in which case a new empty tag set is returned.
 *
 * Returns NULL and leaves an error message in interp->result on error.
 *
 * Non-NULL results must be passed to Ttk_FreeTagSet().
 */
Ttk_TagSet Ttk_GetTagSetFromObj(
    Tcl_Interp *interp, Ttk_TagTable tagTable, Tcl_Obj *objPtr)
{
    Ttk_TagSet tagset = ckalloc(sizeof(*tagset));
    Tcl_Obj **objv;
    int i, objc;

    if (objPtr == NULL) {
	tagset->tags = NULL;
	tagset->nTags = 0;
	return tagset;
    }

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	ckfree(tagset);
    	return NULL;
    }

    tagset->tags = ckalloc((objc+1) * sizeof(Ttk_Tag));
    for (i=0; i<objc; ++i) {
	tagset->tags[i] = Ttk_GetTagFromObj(tagTable, objv[i]);
    }
    tagset->tags[i] = NULL;
    tagset->nTags = objc;

    return tagset;
}

/* Ttk_NewTagSetObj --
 * 	Construct a fresh Tcl_Obj * from a tag set.
 */
Tcl_Obj *Ttk_NewTagSetObj(Ttk_TagSet tagset)
{
    Tcl_Obj *result = Tcl_NewListObj(0,0);
    int i;

    for (i = 0; i < tagset->nTags; ++i) {
	Tcl_ListObjAppendElement(
	    NULL, result, Tcl_NewStringObj(tagset->tags[i]->tagName, -1));
    }
    return result;
}

void Ttk_FreeTagSet(Ttk_TagSet tagset)
{
    ckfree(tagset->tags);
    ckfree(tagset);
}

/* Ttk_TagSetContains -- test if tag set contains a tag.
 */
int Ttk_TagSetContains(Ttk_TagSet tagset, Ttk_Tag tag)
{
    int i;
    for (i = 0; i < tagset->nTags; ++i) {
	if (tagset->tags[i] == tag) {
	    return 1;
	}
    }
    return 0;
}

/* Ttk_TagSetAdd -- add a tag to a tag set.
 *
 * Returns: 0 if tagset already contained tag,
 * 1 if tagset was modified.
 */
int Ttk_TagSetAdd(Ttk_TagSet tagset, Ttk_Tag tag)
{
    int i;
    for (i = 0; i < tagset->nTags; ++i) {
	if (tagset->tags[i] == tag) {
	    return 0;
	}
    }
    tagset->tags = ckrealloc(tagset->tags, 
	    (tagset->nTags+1)*sizeof(tagset->tags[0]));
    tagset->tags[tagset->nTags++] = tag;
    return 1;
}

/* Ttk_TagSetRemove -- remove a tag from a tag set.
 *
 * Returns: 0 if tagset did not contain tag,
 * 1 if tagset was modified.
 */
int Ttk_TagSetRemove(Ttk_TagSet tagset, Ttk_Tag tag)
{
    int i = 0, j = 0;
    while (i < tagset->nTags) {
	if ((tagset->tags[j] = tagset->tags[i]) != tag) {
	    ++j;
	}
	++i;
    }
    tagset->nTags = j;
    return j != i;
}

/*------------------------------------------------------------------------
 * +++ Utilities for widget commands.
 */

/* Ttk_EnumerateTags -- implements [$w tag names]
 */
int Ttk_EnumerateTags(
    Tcl_Interp *interp, Ttk_TagTable tagTable)
{
    return TtkEnumerateHashTable(interp, &tagTable->tags);
}

/* Ttk_EnumerateTagOptions -- implements [$w tag configure $tag]
 */
int Ttk_EnumerateTagOptions(
    Tcl_Interp *interp, Ttk_TagTable tagTable, Ttk_Tag tag)
{
    return TtkEnumerateOptions(interp, tag->tagRecord,
	tagTable->optionSpecs, tagTable->optionTable, tagTable->tkwin);
}

/* Ttk_TagOptionValue -- implements [$w tag configure $tag -option]
 */
Tcl_Obj *Ttk_TagOptionValue(
    Tcl_Interp *interp,
    Ttk_TagTable tagTable,
    Ttk_Tag tag,
    Tcl_Obj *optionName)
{
    return Tk_GetOptionValue(interp,
	tag->tagRecord, tagTable->optionTable, optionName, tagTable->tkwin);
}

/* Ttk_ConfigureTag -- implements [$w tag configure $tag -option value...]
 */
int Ttk_ConfigureTag(
    Tcl_Interp *interp,
    Ttk_TagTable tagTable,
    Ttk_Tag tag,
    int objc, Tcl_Obj *const objv[])
{
    return Tk_SetOptions(
	interp, tag->tagRecord, tagTable->optionTable,
	objc, objv, tagTable->tkwin, NULL/*savedOptions*/, NULL/*mask*/);
}

/*------------------------------------------------------------------------
 * +++ Tag values.
 */

#define OBJ_AT(record, offset) (*(Tcl_Obj**)(((char*)record)+offset))

void Ttk_TagSetValues(Ttk_TagTable tagTable, Ttk_TagSet tagSet, void *record)
{
    const int LOWEST_PRIORITY = 0x7FFFFFFF;
    int i, j;

    memset(record, 0, tagTable->recordSize);

    for (i = 0; tagTable->optionSpecs[i].type != TK_OPTION_END; ++i) {
	Tk_OptionSpec *optionSpec = tagTable->optionSpecs + i;
	int offset = optionSpec->objOffset;
	int prio = LOWEST_PRIORITY;

	for (j = 0; j < tagSet->nTags; ++j) {
	    Ttk_Tag tag = tagSet->tags[j];
	    if (OBJ_AT(tag->tagRecord, offset) != 0 && tag->priority < prio) {
		OBJ_AT(record, offset) = OBJ_AT(tag->tagRecord, offset);
		prio = tag->priority;
	    }
	}
    }
}

void Ttk_TagSetApplyStyle(
    Ttk_TagTable tagTable, Ttk_Style style, Ttk_State state, void *record)
{
    Tk_OptionSpec *optionSpec = tagTable->optionSpecs;

    while (optionSpec->type != TK_OPTION_END) {
	int offset = optionSpec->objOffset;
	const char *optionName = optionSpec->optionName;
	Tcl_Obj *val = Ttk_StyleMap(style, optionName, state);
	if (val) {
	    OBJ_AT(record, offset) = val;
	} else if (OBJ_AT(record, offset) == 0) {
	    OBJ_AT(record, offset) = Ttk_StyleDefault(style, optionName);
	}
	++optionSpec;
    }
}

