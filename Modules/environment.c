/*
# Copyright 1995, InfoSeek Corporation 
# All rights reserved.
# Written by Andy Bensky
#
# Permission to use, copy, modify, and distribute this Python software
# and its associated documentation for any purpose (subject to the
# restriction in the following sentence) without fee is hereby granted,
# provided that the above copyright notice appears in all copies, and
# that both that copyright notice and this permission notice appear in
# supporting documentation, and that the name of InfoSeek not be used in
# advertising or publicity pertaining to distribution of the software
# without specific, prior written permission.  This permission is
# explicitly restricted to the copying and modification of the software
# to remain in Python, compiled Python, or other languages (such as C)
# wherein the modified or derived code is exclusively imported into a
# Python module.
# 
# INFOSEEK CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
# SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
# FITNESS. IN NO EVENT SHALL INFOSEEK CORPORATION BE LIABLE FOR ANY
# DIRECT, SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN 
# AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING 
# OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE, 
# EVEN IF INFOSEEK SHALL HAVE BEEN MADE AWARE OF THE POSSIBILITY OF SUCH
# DAMAGES.
*/

/* Hooks to call the Unix putenv() to modify the environment
*/

#include "allobjects.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
 
/* Error conditions that can be raised */
 
/* Headers for functions accessible from Python as module methods */
static object *put_environ( object *self, object *args );
 
static struct methodlist environ_methods[] = {
        {"putenv", put_environ},
        {NULL, NULL}
};
 
 
/* 
 * Name: initenvironment
 * Description: 
 *     Initialzation function that Python will use to establish callbacks to
 * the methods of this module.
 * 
 * Returns: 
 *     void  - 
 * 
 * Notes: 
 */ 
void initenvironment()
{
	object *m, *d;

	m = initmodule("environment", environ_methods);
	d = getmoduledict(m);
}
   
/* 
 * Name: put_environ
 * Description: 
 * accepts 2 string objects as arguments and forms a string of the
 * form string1=string2 that can be passed to the putenv() system call.
 *
 * Returns: 
 *      None object if successfull, otherwise raises a SystemError exception
 *
 * 
 * Notes: 
 */ 
static object *put_environ( object *self, object *args )
{
        char *string1, *string2;
        char *set_str;
        object *return_object = None;
 
        if (args && getargs(args, "(ss)", &string1, &string2))
        {
            set_str = malloc(strlen(string1) + strlen(string2) + 2);
            assert( set_str );
            (void) sprintf(set_str, "%s=%s", string1, string2);
            if ( putenv( set_str ) )
            {
                err_setstr(SystemError, "Error in system putenv call.");
                return_object = 0;
            }
        }
        else
        {
                err_setstr(TypeError, "Usage: putenv(string1, string2)");
                return_object = 0;
        }
 
        return( return_object );
}
