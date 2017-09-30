/* decrunch.c
 *
 * based on load.c from:
 * Extended Module Player
 *
 * Copyright (C) 1996-1999 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * CHANGES: (modified for uade by mld)
 * removed all xmp related code)
 * added "custom" labels of pp20 files
 * added support for external unrar decruncher
 * added support for the external XPK Lib for Unix (the xType usage *g*)
 *
 * TODO:
 * real builtin support for XPK lib for Unix
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See doc/COPYING
 * for more information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef __EMX__
#include <sys/types.h>
#endif


#include <sys/stat.h>
#include <unistd.h>
#include "decrunch.h"
#include "../config.h"


int decrunch_sqsh (FILE *, FILE *);
int decrunch_pp (FILE *, FILE *, char *filename);
int decrunch_mmcmp (FILE *, FILE *);
int strip_sc68 (FILE *, FILE *);
int easo_hack (FILE *, FILE *);


#define BUILTIN_PP	0x01
#define BUILTIN_SQSH	0x02
#define BUILTIN_MMCMP	0x03
#define BUILTIN_SC68	0x04
#define BUILTIN_EASO	0x05


int decrunch (FILE **f, char *s)
{
    unsigned char b[64];
    char *cmd;
    FILE *t;
    int fd, builtin, res;
    char *packer, *temp, *temp2;
    char fmode[16];
    int nbytes;

    packer = cmd = NULL;
    builtin = res = 0;

    nbytes = fread (b, 1, 64, *f);

    if (nbytes >= 2 && b[0] == 'P' && b[1] == 'K') {
        packer = "Zip";

#ifdef __EMX__
	cmd = "unzip -pqqC \"%s\" -x readme '*.diz' '*.nfo' '*.txt' "
		"'*.exe' '*.com' 2>NUL";
#else

	cmd = "unzip -pqqC \"%s\" -x '*readme*' '*.diz' '*.nfo' '*.txt' '*.inf'"
		"'*.exe' '*.com' 2>/dev/null";

#endif

    } else if (nbytes >= 5 && b[2] == '-' && b[3] == 'l' && b[4] == 'h') {
	packer = "LHa";
#ifdef __EMX__
	fprintf( stderr, "LHA for OS/2 does NOT support output to stdout.\n" );
#endif
	cmd = "lha -pq \"%s\"";
    } else if (nbytes >= 2 && b[0] == 31 && b[1] == 139) {
	packer = "gzip";
	cmd = "gzip -dc \"%s\"";
    } else if (nbytes >= 3 && b[0] == 'R' && b[1] == 'a' && b[2] == 'r') {
      /* needs the freeware unrar unpacker for archives created by 
	 the win shareware packer rar.
	 Just for the case one might need it...*/
	packer = "rar";
	cmd = "unrar p -inul -xreadme -x*.diz -x*.nfo -x*.txt "
	        "-x*.exe -x*.com \"%s\"";
    } else if (nbytes >= 3 && b[0] == 'B' && b[1] == 'Z' && b[2] == 'h') {
	packer = "bzip2";
	cmd = "bzip2 -dc \"%s\"";
    } else if (nbytes >= 2 && b[0] == 31 && b[1] == 157) {
	packer = "compress";


#ifdef __EMX__
	fprintf( stderr, "I was unable to find a OS/2 version of UnCompress...\n" );
	fprintf( stderr, "I hope that the default command will work if a OS/2 version is found/created!\n" );
#endif

	cmd = "uncompress -c \"%s\"";
    } else if ( nbytes >= 4 && (
				(b[0] == 'P'  && b[1] == 'X'  && b[2] == '2'  && b[3] == '0' ) ||
				(b[0] == 'P'  && b[1] == 'P'  && b[2] == '2'  && b[3] == '0' )
				)) {
        packer = "PowerPacker data";
	builtin = BUILTIN_PP;
#ifdef HAVE_XPKLIB
	/* a lame way to interact with the XPKLib I know... *g* 
	   But I am lazy, feel free to make it right :) */
        } else if (nbytes >= 4 && b[0] == 'X' && b[1] == 'P' && b[2] == 'K' && b[3] == 'F'){
	packer = "XPK";
	cmd = "xType \"%s\"";
#else
    } else if (nbytes >= 12 && b[0] == 'X' && b[1] == 'P' && b[2] == 'K' && b[3] == 'F' &&
	b[8] == 'S' && b[9] == 'Q' && b[10] == 'S' && b[11] == 'H') {
	packer = "XPK SQSH";
	builtin = BUILTIN_SQSH;
#endif
    } else if (nbytes >= 8 && b[0] == 'z' && b[1] == 'i' && b[2] == 'R' && b[3] == 'C' &&
	b[4] == 'O' && b[5] == 'N' && b[6] == 'i' && b[7] == 'a') {
	packer = "MMCMP";
	builtin = BUILTIN_MMCMP;

    } else if (nbytes >= 9 && b[0] == 'S' && b[1] == 'C' && b[2] == '6' && b[3] == '8' &&
	b[5] == 'M' && b[6] == 'u' && b[7] == 's' && b[8] == 'i') {
	packer = "SC68 fileformat";
	builtin = BUILTIN_SC68;

    } else if (nbytes >= 4 && b[0] == 'E' && b[1] == 'A' && b[2] == 'S' && b[3] == 'O'){
	packer = "M.Grouleff/Earache(EASO) music";
	builtin = BUILTIN_EASO;
    }

    fseek (*f, 0, SEEK_SET);

    if (!packer)
	return 0;

    fprintf (stderr, "uade: processing %s file... ", packer);

    temp = strdup ("/tmp/decr_XXXXXX");
    if (!temp) {
      fprintf (stderr, "failed (not enough memory)\n");
      return -1;
    }

    if ((fd = mkstemp (temp)) < 0) {
	fprintf (stderr, "failed (couldn't have temp file)\n");
	free(temp);
	return -1;
    }
    unlink (temp);

#if defined(AMIGA) && defined(__libnix__)
    strcpy(fmode, "wb");
#else
    /* it really should be like this */
    strcpy(fmode, "w+b");
#endif
    if ((t = fdopen (fd, fmode)) == NULL) {
	fprintf (stderr, "failed (fdopen())\n");
	free(temp);
	return -1;
    }

    if (cmd) {
	int n;
	char *line, *buf;
	FILE *p;

	line = malloc (strlen (cmd) + strlen (s) + 16);
	sprintf (line, cmd, s);

	if ((p = popen (line, "r")) == NULL) {

	    fprintf (stderr,"failed\n");

	    fclose (t);
	    free (line);
	    free (temp);
	    return -1;
	}
	free (line);
#define BSIZE 0x4000
	if ((buf = malloc (BSIZE)) == NULL) {

	    fprintf (stderr,"failed\n");

	    free (temp);
	    pclose (p);
	    fclose (t);
	    return -1;
	}
	while ((n = fread (buf, 1, BSIZE, p)) > 0) {
	    fwrite (buf, 1, n, t);
	} 
	free (buf);
	pclose (p);
    } else switch (builtin) {
    case BUILTIN_PP:    
	res = decrunch_pp (*f, t, s);
	break;
    case BUILTIN_SQSH:    
	res = decrunch_sqsh (*f, t);
	break;
    case BUILTIN_MMCMP:    
	res = decrunch_mmcmp (*f, t);
	break;
    case BUILTIN_SC68:    
	res = strip_sc68 (*f, t);
	break;
    case BUILTIN_EASO:    
	res = easo_hack (*f, t);
	break;
    }

    if (res < 0) {

	fprintf (stderr, "failed\n");

	unlink (temp);
	free (temp);
	return -1;
    }

    fprintf (stderr, "done\n");

    fclose (*f);
    *f = t;
 
    temp2 = strdup (temp);
    if (decrunch (f, temp))
	unlink (temp2);
    free (temp2);
    free (temp);

    return 1;
}
