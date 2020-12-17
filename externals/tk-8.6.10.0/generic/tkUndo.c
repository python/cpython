/*
 * tkUndo.c --
 *
 *	This module provides the implementation of an undo stack.
 *
 * Copyright (c) 2002 by Ludwig Callewaert.
 * Copyright (c) 2003-2004 by Vincent Darley.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkUndo.h"

static int		EvaluateActionList(Tcl_Interp *interp,
			    TkUndoSubAtom *action);

/*
 *----------------------------------------------------------------------
 *
 * TkUndoPushStack --
 *
 *	Push elem on the stack identified by stack.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkUndoPushStack(
    TkUndoAtom **stack,
    TkUndoAtom *elem)
{
    elem->next = *stack;
    *stack = elem;
}

/*
 *----------------------------------------------------------------------
 *
 * TkUndoPopStack --
 *
 *	Remove and return the top element from the stack identified by stack.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkUndoAtom *
TkUndoPopStack(
    TkUndoAtom **stack)
{
    TkUndoAtom *elem = NULL;

    if (*stack != NULL) {
	elem = *stack;
	*stack = elem->next;
    }
    return elem;
}

/*
 *----------------------------------------------------------------------
 *
 * TkUndoInsertSeparator --
 *
 *	Insert a separator on the stack, indicating a border for an undo/redo
 *	chunk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkUndoInsertSeparator(
    TkUndoAtom **stack)
{
    TkUndoAtom *separator;

    if (*stack!=NULL && (*stack)->type!=TK_UNDO_SEPARATOR) {
	separator = ckalloc(sizeof(TkUndoAtom));
	separator->type = TK_UNDO_SEPARATOR;
	TkUndoPushStack(stack,separator);
	return 1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TkUndoClearStack --
 *
 *	Clear an entire undo or redo stack and destroy all elements in it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkUndoClearStack(
    TkUndoAtom **stack)		/* An Undo or Redo stack */
{
    TkUndoAtom *elem;

    while ((elem = TkUndoPopStack(stack)) != NULL) {
	if (elem->type != TK_UNDO_SEPARATOR) {
	    TkUndoSubAtom *sub;

	    sub = elem->apply;
	    while (sub != NULL) {
		TkUndoSubAtom *next = sub->next;

		if (sub->action != NULL) {
		    Tcl_DecrRefCount(sub->action);
		}
		ckfree(sub);
		sub = next;
	    }

	    sub = elem->revert;
	    while (sub != NULL) {
		TkUndoSubAtom *next = sub->next;

		if (sub->action != NULL) {
		    Tcl_DecrRefCount(sub->action);
		}
		ckfree(sub);
		sub = next;
	    }
	}
	ckfree(elem);
    }
    *stack = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkUndoPushAction --
 *
 *	Push a new elem on the stack identified by stack. Action and revert
 *	are given through Tcl_Obj's to which we will retain a reference. (So
 *	they can be passed in with a zero refCount if desired).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkUndoPushAction(
    TkUndoRedoStack *stack,	/* An Undo or Redo stack */
    TkUndoSubAtom *apply,
    TkUndoSubAtom *revert)
{
    TkUndoAtom *atom;

    atom = ckalloc(sizeof(TkUndoAtom));
    atom->type = TK_UNDO_ACTION;
    atom->apply = apply;
    atom->revert = revert;

    TkUndoPushStack(&stack->undoStack, atom);
    TkUndoClearStack(&stack->redoStack);
}

/*
 *----------------------------------------------------------------------
 *
 * TkUndoMakeCmdSubAtom --
 *
 *	Create a new undo/redo step which must later be place into an undo
 *	stack with TkUndoPushAction. This sub-atom, if evaluated, will take
 *	the given command (if non-NULL), find its full Tcl command string, and
 *	then evaluate that command with the list elements of 'actionScript'
 *	appended.
 *
 *	If 'subAtomList' is non-NULL, the newly created sub-atom is added onto
 *	the end of the linked list of which 'subAtomList' is a part. This
 *	makes it easy to build up a sequence of actions which will be pushed
 *	in one step.
 *
 *	Note: if the undo stack can persist for longer than the Tcl_Command
 *	provided, the stack will cause crashes when actions are evaluated. In
 *	this case the 'command' argument should not be used. This is the case
 *	with peer text widgets, for example.
 *
 * Results:
 *	The newly created subAtom is returned. It must be passed to
 *	TkUndoPushAction otherwise a memory leak will result.
 *
 * Side effects:
 *	A refCount is retained on 'actionScript'.
 *
 *----------------------------------------------------------------------
 */

TkUndoSubAtom *
TkUndoMakeCmdSubAtom(
    Tcl_Command command,	/* Tcl command token for actions, may be NULL
				 * if not needed. */
    Tcl_Obj *actionScript,	/* The script to append to the command to
				 * perform the action (may be NULL if the
				 * command is not-null). */
    TkUndoSubAtom *subAtomList)	/* Add to the end of this list of actions if
				 * non-NULL */
{
    TkUndoSubAtom *atom;

    if (command == NULL && actionScript == NULL) {
	Tcl_Panic("NULL command and actionScript in TkUndoMakeCmdSubAtom");
    }

    atom = ckalloc(sizeof(TkUndoSubAtom));
    atom->command = command;
    atom->funcPtr = NULL;
    atom->clientData = NULL;
    atom->next = NULL;
    atom->action = actionScript;
    if (atom->action != NULL) {
        Tcl_IncrRefCount(atom->action);
    }

    if (subAtomList != NULL) {
	while (subAtomList->next != NULL) {
	    subAtomList = subAtomList->next;
	}
	subAtomList->next = atom;
    }
    return atom;
}

/*
 *----------------------------------------------------------------------
 *
 * TkUndoMakeSubAtom --
 *
 *	Create a new undo/redo step which must later be place into an undo
 *	stack with TkUndoPushAction. This sub-atom, if evaluated, will take
 *	the given C-funcPtr (which must be non-NULL), and call it with three
 *	arguments: the undo stack's 'interp', the 'clientData' given and the
 *	'actionScript'. The callback should return a standard Tcl return code
 *	(TCL_OK on success).
 *
 *	If 'subAtomList' is non-NULL, the newly created sub-atom is added onto
 *	the end of the linked list of which 'subAtomList' is a part. This
 *	makes it easy to build up a sequence of actions which will be pushed
 *	in one step.
 *
 * Results:
 *	The newly created subAtom is returned. It must be passed to
 *	TkUndoPushAction otherwise a memory leak will result.
 *
 * Side effects:
 *	A refCount is retained on 'actionScript'.
 *
 *----------------------------------------------------------------------
 */

TkUndoSubAtom *
TkUndoMakeSubAtom(
    TkUndoProc *funcPtr,	/* Callback function to perform the
				 * undo/redo. */
    ClientData clientData,	/* Data to pass to the callback function. */
    Tcl_Obj *actionScript,	/* Additional Tcl data to pass to the callback
				 * function (may be NULL). */
    TkUndoSubAtom *subAtomList)	/* Add to the end of this list of actions if
				 * non-NULL */
{
    TkUndoSubAtom *atom;

    if (funcPtr == NULL) {
	Tcl_Panic("NULL funcPtr in TkUndoMakeSubAtom");
    }

    atom = ckalloc(sizeof(TkUndoSubAtom));
    atom->command = NULL;
    atom->funcPtr = funcPtr;
    atom->clientData = clientData;
    atom->next = NULL;
    atom->action = actionScript;
    if (atom->action != NULL) {
        Tcl_IncrRefCount(atom->action);
    }

    if (subAtomList != NULL) {
	while (subAtomList->next != NULL) {
	    subAtomList = subAtomList->next;
	}
	subAtomList->next = atom;
    }
    return atom;
}

/*
 *----------------------------------------------------------------------
 *
 * TkUndoInitStack --
 *
 *	Initialize a new undo/redo stack.
 *
 * Results:
 *	An Undo/Redo stack pointer.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkUndoRedoStack *
TkUndoInitStack(
    Tcl_Interp *interp,		/* The interpreter */
    int maxdepth)		/* The maximum stack depth */
{
    TkUndoRedoStack *stack;	/* An Undo/Redo stack */

    stack = ckalloc(sizeof(TkUndoRedoStack));
    stack->undoStack = NULL;
    stack->redoStack = NULL;
    stack->interp = interp;
    stack->maxdepth = maxdepth;
    stack->depth = 0;
    return stack;
}

/*
 *----------------------------------------------------------------------
 *
 * TkUndoSetMaxDepth --
 *
 *	Set the maximum depth of stack.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May delete elements from the stack if the new maximum depth is smaller
 *	than the number of elements previously in the stack.
 *
 *----------------------------------------------------------------------
 */

void
TkUndoSetMaxDepth(
    TkUndoRedoStack *stack,	/* An Undo/Redo stack */
    int maxdepth)		/* The maximum stack depth */
{
    stack->maxdepth = maxdepth;

    if (stack->maxdepth>0 && stack->depth>stack->maxdepth) {
	TkUndoAtom *elem, *prevelem;
	int sepNumber = 0;

	/*
	 * Maximum stack depth exceeded. We have to remove the last compound
	 * elements on the stack.
	 */

	elem = stack->undoStack;
	prevelem = NULL;
	while ((elem != NULL) && (sepNumber <= stack->maxdepth)) {
	    if (elem->type == TK_UNDO_SEPARATOR) {
		sepNumber++;
	    }
	    prevelem = elem;
	    elem = elem->next;
	}
	CLANG_ASSERT(prevelem);
	prevelem->next = NULL;
	while (elem != NULL) {
	    prevelem = elem;
	    if (elem->type != TK_UNDO_SEPARATOR) {
		TkUndoSubAtom *sub = elem->apply;
		while (sub != NULL) {
		    TkUndoSubAtom *next = sub->next;

		    if (sub->action != NULL) {
			Tcl_DecrRefCount(sub->action);
		    }
		    ckfree(sub);
		    sub = next;
		}
		sub = elem->revert;
		while (sub != NULL) {
		    TkUndoSubAtom *next = sub->next;

		    if (sub->action != NULL) {
			Tcl_DecrRefCount(sub->action);
		    }
		    ckfree(sub);
		    sub = next;
		}
	    }
	    elem = elem->next;
	    ckfree(prevelem);
	}
	stack->depth = stack->maxdepth;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkUndoClearStacks --
 *
 *	Clear both the undo and redo stack.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkUndoClearStacks(
    TkUndoRedoStack *stack)	/* An Undo/Redo stack */
{
    TkUndoClearStack(&stack->undoStack);
    TkUndoClearStack(&stack->redoStack);
    stack->depth = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TkUndoFreeStack
 *
 *	Clear both the undo and redo stack and free the memory allocated to
 *	the u/r stack pointer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkUndoFreeStack(
    TkUndoRedoStack *stack)	/* An Undo/Redo stack */
{
    TkUndoClearStacks(stack);
    ckfree(stack);
}

/*
 *----------------------------------------------------------------------
 *
 * TkUndoCanRedo --
 *
 *	Returns true if redo is possible, i.e. if the redo stack is not empty.
 *
 * Results:
 *	 A boolean.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkUndoCanRedo(
    TkUndoRedoStack *stack)	/* An Undo/Redo stack */
{
    return stack->redoStack != NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkUndoCanUndo --
 *
 *	Returns true if undo is possible, i.e. if the undo stack is not empty.
 *
 * Results:
 *	 A boolean.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkUndoCanUndo(
    TkUndoRedoStack *stack)	/* An Undo/Redo stack */
{
    return stack->undoStack != NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkUndoInsertUndoSeparator --
 *
 *	Insert a separator on the undo stack, indicating a border for an
 *	undo/redo chunk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkUndoInsertUndoSeparator(
    TkUndoRedoStack *stack)
{
    if (TkUndoInsertSeparator(&stack->undoStack)) {
	stack->depth++;
	TkUndoSetMaxDepth(stack, stack->maxdepth);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkUndoRevert --
 *
 *	Undo a compound action on the stack.
 *
 * Results:
 *	A Tcl status code
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkUndoRevert(
    TkUndoRedoStack *stack)
{
    TkUndoAtom *elem;

    /*
     * Insert a separator on the undo and the redo stack.
     */

    TkUndoInsertUndoSeparator(stack);
    TkUndoInsertSeparator(&stack->redoStack);

    /*
     * Pop and skip the first separator if there is one.
     */

    elem = TkUndoPopStack(&stack->undoStack);
    if (elem == NULL) {
	return TCL_ERROR;
    }

    if (elem->type == TK_UNDO_SEPARATOR) {
	ckfree(elem);
	elem = TkUndoPopStack(&stack->undoStack);
    }

    while (elem != NULL && elem->type != TK_UNDO_SEPARATOR) {
	/*
	 * Note that we currently ignore errors thrown here.
	 */

	EvaluateActionList(stack->interp, elem->revert);

	TkUndoPushStack(&stack->redoStack, elem);
	elem = TkUndoPopStack(&stack->undoStack);
    }

    /*
     * Insert a separator on the redo stack.
     */

    TkUndoInsertSeparator(&stack->redoStack);
    stack->depth--;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkUndoApply --
 *
 *	Redo a compound action on the stack.
 *
 * Results:
 *	A Tcl status code
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkUndoApply(
    TkUndoRedoStack *stack)
{
    TkUndoAtom *elem;

    /*
     * Insert a separator on the undo stack.
     */

    TkUndoInsertSeparator(&stack->undoStack);

    /*
     * Pop and skip the first separator if there is one.
     */

    elem = TkUndoPopStack(&stack->redoStack);
    if (elem == NULL) {
	return TCL_ERROR;
    }

    if (elem->type == TK_UNDO_SEPARATOR) {
	ckfree(elem);
	elem = TkUndoPopStack(&stack->redoStack);
    }

    while (elem != NULL && elem->type != TK_UNDO_SEPARATOR) {
	/*
	 * Note that we currently ignore errors thrown here.
	 */

	EvaluateActionList(stack->interp, elem->apply);

	TkUndoPushStack(&stack->undoStack, elem);
	elem = TkUndoPopStack(&stack->redoStack);
    }

    /*
     * Insert a separator on the undo stack.
     */

    TkUndoInsertSeparator(&stack->undoStack);
    stack->depth++;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * EvaluateActionList --
 *
 *	Execute a linked list of undo/redo sub-atoms. If any sub-atom returns
 *	a non TCL_OK value, execution of subsequent sub-atoms is cancelled and
 *	the error returned immediately.
 *
 * Results:
 *	A Tcl status code
 *
 * Side effects:
 *	The undo/redo subAtoms can perform arbitrary actions.
 *
 *----------------------------------------------------------------------
 */

static int
EvaluateActionList(
    Tcl_Interp *interp,		/* Interpreter to evaluate the action in. */
    TkUndoSubAtom *action)	/* Head of linked list of action steps to
				 * perform. */
{
    int result = TCL_OK;

    while (action != NULL) {
	if (action->funcPtr != NULL) {
	    result = action->funcPtr(interp, action->clientData,
		    action->action);
	} else if (action->command != NULL) {
	    Tcl_Obj *cmdNameObj, *evalObj;

	    cmdNameObj = Tcl_NewObj();
	    evalObj = Tcl_NewObj();
	    Tcl_IncrRefCount(evalObj);
	    Tcl_GetCommandFullName(interp, action->command, cmdNameObj);
	    Tcl_ListObjAppendElement(NULL, evalObj, cmdNameObj);
	    if (action->action != NULL) {
	        Tcl_ListObjAppendList(NULL, evalObj, action->action);
	    }
	    result = Tcl_EvalObjEx(interp, evalObj, TCL_EVAL_GLOBAL);
	    Tcl_DecrRefCount(evalObj);
	} else {
	    result = Tcl_EvalObjEx(interp, action->action, TCL_EVAL_GLOBAL);
	}
	if (result != TCL_OK) {
	    return result;
	}
	action = action->next;
    }
    return result;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
