/* XMMS UADE plugin
 *
 * Copyright (C) 2000-2001  Heikki Orsila
 *                          heikki.orsila@iki.fi
 *                          http://www.modeemi.fi/~shd
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


#include "deli.h"
#include "../../decrunch/decrunch.h"

/* var decl*/

char *last_filename ="";
char last_pre[256], last_post[256];


/* FUNCTION DECLARATIONS */

void scanfile(char*, char*, char*);


/* Functions */

void scanfile(char* filename, char* pre, char* post) {

  FILE* songfile;
  int filesize = 5122;
  int realfilesize = 0;
  unsigned char buf[5122]="";
  struct stat st;
  int status;

    songfile = fopen (filename, "r");

   if(!songfile) {
       fprintf (stderr, "file open error...\n");
       return;
   } else {

/* decrunch support */
/*
      if ((status = decrunch ( &songfile, filename) < 0)) {

       fprintf (stderr, "decrunching error...\n");
       fclose (songfile);
      return;
    }
*/
   fstat (fileno(songfile), &st);
   fread  (buf, 1, 5122, songfile);
   fclose (songfile);

   filemagic(buf, pre, post, realfilesize); /* Analyze the loaded file */
  }
return;
}
