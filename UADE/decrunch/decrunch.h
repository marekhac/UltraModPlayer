/* Based on the decr. code of:
 *
 * Extended Module Player
 * Copyright (C) 1996-1999 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * modified for uade by mld
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See doc/COPYING
 * for more information.
 */

/* uncomment for support for the external XPK Linux for Unix */
//#define HAVE_XPKLIB 1

#include <stdio.h>
#include <signal.h>

typedef signed char int8;
typedef signed short int int16;
#ifndef __amigaos4__
//conflicting types (signed long)
typedef signed int int32;
#endif
typedef unsigned char uint8;
typedef unsigned short int uint16;
#ifndef __amigaos4__
typedef unsigned int uint32;
#endif

int decrunch (FILE **f, char *s);
