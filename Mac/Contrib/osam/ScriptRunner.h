/*
 *
 * This is a simple module to allow the 
 * user to compile and execute an applescript
 * which is passed in as a text item.
 *
 *  Sean Hummel <seanh@prognet.com>
 *  1/20/98
 *  RealNetworks
 *
 *  Jay Painter <jpaint@serv.net> <jpaint@gimp.org> <jpaint@real.com>
 *  
 *
 */
#pragma once

#include <OSA.h>

OSAError CompileAndExecute     (const char     *text, 
				AEDesc         *result, 
				OSAActiveUPP    proc);

OSAError CompileAndSave        (const char     *text, 
				const char     *outfile, 
				OSAActiveUPP    proc, 
				AEDesc         *result);

OSAError ExecuteScriptFile     (const char     *theFile,
				OSAActiveUPP    proc, 
				AEDesc         *result);
