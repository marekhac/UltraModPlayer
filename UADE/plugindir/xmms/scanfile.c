/* XMMS UADE plugin
 *
 * Copyright (C) 2000-2003  Heikki Orsila
 *                          heikki.orsila@iki.fi
 *                          http://uade.ton.tut.fi
 *
 * This plugin is based on xmms 0.9.6 wavplayer input plugin code. Since
 * then all code has been rewritten.
 *
 * the intital gui code was based onthe null-plugin by
 * Håvard Kvålen. Since then the gui code has evolved and was also mostly
 * rewritten.
 * Formatseditor inspired by nscache 0.3's mimetype editor by Stefan Ondrejicka
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>

#include "defaults.h"
#include "uade.h"
#include "gui.h"
#include "../decrunch/decrunch.h"

/* var decl*/

char *last_filename = "";
char last_pre[256], last_post[256];

/* FUNCTION DECLARATIONS */

void scanfile(char*, char*, char*);


/* Functions */

void scanfile(char* filename, char* pre, char* post) {
  /* char scanfile():
     detects formats like e.g.: tfmx1.5, hip, hipc, fc13, fc1.4	 
     - tfmx 1.5 checking based on both tfmx DT and tfmxplay by jhp, 
       and the EP by Don Adan/WT.
     - other checks based on various deliplayer/eagleplayer sources
       from Exotica
     by far not complete...
  */

  FILE *songfile;
  unsigned char buf[5122] = "";
  int status;
  int realfilesize;
  struct stat st;
  
  if(!strcasecmp(filename, last_filename)) {

    strcpy(pre, last_pre);
    strcpy(post, last_post);
    return;

  } else  {

    songfile = fopen (filename, "rb");
    /* decrunch support */
    if(!songfile) {
      /* can't open the file for scanning.. just return */
      return;
    } else {
      if(filemagic_decr) {
	if((status = decrunch ( &songfile, filename) < 0)) {
	  fprintf (stderr, "decrunching error (file %s)\n", filename);
	  fclose (songfile);
	  return;
	}
      }

      fstat (fileno(songfile), &st);
      realfilesize= st.st_size;
      
      fread  (buf, 1, 5122, songfile);
      fclose (songfile);

      filemagic(buf, pre, post, realfilesize);
      /* and that's the end of all the magic ;) */

      /* save the scanned results for later checks */
      last_filename = g_strdup_printf("%s", filename);
      strcpy(last_post, post);
      strcpy(last_pre, pre);
    }
  }
}

