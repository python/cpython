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


/* Python API */
#include "Python.h"
#include "macglue.h"


/* Macintosh API */
#include <Types.h>
#include <AppleEvents.h>
#include <Processes.h>
#include <Files.h>
#include <Gestalt.h>
#include <Events.h>
