/* sc68.c */

/* part of libdecr.a for uade
 *
 * strips header data of sc68 files to get the original
 * Amiga music data back
 *
 *
 */

/*
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
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


int strip_sc68 (FILE *f, FILE *fo)
{
    uint8 *packed, *unpacked;
    int plen, unplen, i, j;
    int begdata=0;
    int enddata=0;
    int count=0;
    struct stat st;

    if (fo == NULL)
	return -1;

    fstat (fileno (f), &st);
    plen = st.st_size;

    packed = malloc (plen);
    if (!packed)
	{
	fprintf(stderr, "can't allocate memory for sc68 file...");
	return -1;
    	}

    fread (packed, plen,1 ,f);

    for (i = 0; i <= plen-4; i++)
     {
     if (packed[i]=='S' && packed [i+1]=='C' && packed [i+2]=='D' && packed [i+3]=='A')
	{
	j=0;
	while (j < plen-i-9) {
	 if (packed[i+8+j] == 0x4E && packed[i+9+j] == 0x71)
	    {
	        j=j+2;
            } else {
        	if (begdata != 0) count++;
    		begdata = i + 8 +j; 
		break;
	    }
        }

      }
     if (packed[i]=='S' && packed [i+1]=='C' && packed [i+2]=='E' && packed [i+3]=='F')
	{
	enddata = i - 1 ; 
        }
    }     

    unplen = enddata - begdata;
    //fprintf (stderr, "%d, %d, %d\n", plen, unplen, begdata);
    if (!unplen)
     {
     fprintf (stderr, "not a sc68 file...");
     return -1;	/* no valid data found */
      }

    unpacked = (uint8 *) malloc (unplen);
    if (!unpacked)
     {
     fprintf (stderr, "can't allocate mem");
     return -1;	/* no valid data found */
     }

    memcpy (unpacked,packed+begdata,unplen);

    if (count >1) fprintf (stderr, "\nWARNING: sc68 file actually contains %d modules!\nOnly the last one will be used... ", count);

    fwrite (unpacked, unplen, 1, fo);
    free (unpacked);
    free (packed);

    return 0;
}
