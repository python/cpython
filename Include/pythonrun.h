/***********************************************************
Copyright 1991 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Interfaces to parse and execute pieces of python code */

void initall PROTO((void));

int run PROTO((FILE *, char *));

int run_script PROTO((FILE *, char *));
int run_tty_1 PROTO((FILE *, char *));
int run_tty_loop PROTO((FILE *, char *));

int parse_string PROTO((char *, int, struct _node **));
int parse_file PROTO((FILE *, char *, int, struct _node **));

object *eval_node PROTO((struct _node *, char *, object *, object *));

object *run_string PROTO((char *, int, object *, object *));
object *run_file PROTO((FILE *, char *, int, object *, object *));
object *run_err_node PROTO((int, struct _node *, char *, object *, object *));
object *run_node PROTO((struct _node *, char *, object *, object *));

void print_error PROTO((void));

void goaway PROTO((int));
