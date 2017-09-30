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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../decrunch/decrunch.h"
#include "../osdep/strl.c"
#include "../osdep/strln.c"

static int find_tag(unsigned char *buf, char *tag, int startoffset,
		    int buflen);
static unsigned int readbig_w(char *ptr);
static unsigned int readbig_lw(char *ptr);

static void process_ptk_mod(char *credits, int credits_len, int inst,
			    unsigned char *buf, int modfilelen);
static void process_aon_mod(char *credits, int credits_len,
			    unsigned char *buf, int modfilelen);
static void process_dmu_mod(char *credits, int credits_len,
			    unsigned char *buf);
static void process_dm2_mod(char *credits, int credits_len,
			    unsigned char *buf);
static void process_tfmx_mod(char *credits, int credits_len,
			     unsigned char *buf);
static void process_mon_mod(char *credits, int credits_len,
			    unsigned char *buf);
static void process_synmod_mod(char *credits, int credits_len,
			       unsigned char *buf);
static void process_ahx_mod(char *credits, int credits_len,
			    unsigned char *buf, int modfilelen);

static void process_digi_mod(char *credits, int credits_len,
			     unsigned char *buf, int modfilelen);

/* WTWT */
static void process_WTWT_mod(char *credits, int credits_len,
			     unsigned char *buf, int modfilelen, char *lo,
			     char *hi, int rel);


void processmodule(char *credits, char *filename, int credits_len)
{
  FILE *modfile;
  struct stat st;
  int modfilelen, minsong, maxsong, currsong;
  unsigned char *buf;
  char pre[256] = "";
  char post[256] = "";
  int ret;

  if (!(modfile = fopen(filename, "rb")))
    return;


  if (decrunch(&modfile, filename) < 0) {
    fclose(modfile);
    return;
  }

  fstat(fileno(modfile), &st);
  modfilelen = st.st_size;

  if (!(buf = malloc(modfilelen))) {
    fprintf(stderr, "can't allocate mem");
    return;
  }

  ret = fread(buf, 1, modfilelen, modfile);

  fclose(modfile);

  if (ret < modfilelen) {
    fprintf(stderr, "uade: processmodule could not read %s fully\n",
	    filename);
    goto out;
  }

  minsong = get_min_subsong();
  maxsong = get_max_subsong();
  currsong = get_curr_subsong();

  strlcpy(credits, g_strdup_printf("file:  %s", filename), credits_len);
  strlcat(credits, g_strdup_printf("\nfile length:  %d bytes\n", modfilelen),
	  credits_len);


  if (strcasecmp(get_modulename(), uade_song_basename) != 0) {
    strlcat(credits, g_strdup_printf("\nmodule:  %s", get_modulename()),
	    credits_len); 
  } else {
    strlcat(credits, g_strdup_printf("\nmodule:  %s", uade_song_basename),
	    credits_len); 
  }

  if (strlen(get_formatname()) == 0) {
    strlcat(credits, g_strdup_printf("\nmodule format:  %s\n", get_playername()),
	    credits_len);
  } else {
    strlcat(credits,
	    g_strdup_printf("\nmodule format:  %s, %s\n",
			    get_playername(), get_formatname()), credits_len);
  }


  if (maxsong > 0 && minsong != maxsong)
    strlcat(credits,
	    g_strdup_printf("subsong: %d/%d\n\n", currsong,
			    maxsong), credits_len);

  filemagic(buf, pre, post, modfilelen);	/* get filetype */

  if (strcasecmp(pre, "DM2") == 0) {
    process_dm2_mod(credits, credits_len, buf);	/*DM2 */

  } else if ((strcasecmp(pre, "MOD15") == 0) ||
	     (strcasecmp(pre, "MOD_UST") == 0)) {
    /*MOD15 */
    process_ptk_mod(credits, credits_len, 15, buf, modfilelen);

  } else if ((strcasecmp(pre, "MOD") == 0) ||
	     (strcasecmp(pre, "PPK") == 0) ||
	     (strcasecmp(pre, "MOD_PC") == 0) ||
	     (strcasecmp(pre, "ICE") == 0) ||
	     (strcasecmp(pre, "ADSC") == 0)) {

    /*MOD*/
    process_ptk_mod(credits, credits_len, 31, buf, modfilelen);


  } else if ((strcasecmp(pre, "AON4") == 0) ||
	     (strcasecmp(pre, "AON") == 0) ||
	     (strcasecmp(pre, "AON8") == 0)) {
     /*AON*/ process_aon_mod(credits, credits_len, buf, modfilelen);

  } else if ((strcasecmp(pre, "DMU") == 0) ||
	     (strcasecmp(pre, "MUG") == 0) ||
	     (strcasecmp(pre, "MUG2") == 0) ||
	     (strcasecmp(pre, "DMU2") == 0)) {
    /* DMU */
    process_dmu_mod(credits, credits_len, buf);

  } else if ((strcasecmp(pre, "TFMX1.5") == 0) ||
	     (strcasecmp(pre, "TFMX7V") == 0) ||
	     (strcasecmp(pre, "MDAT") == 0)) {
    /* TFMX */
    process_tfmx_mod(credits, credits_len, buf);

  } else if (strcasecmp(pre, "MON") == 0) {
    /* MON */
    process_mon_mod(credits, credits_len, buf);

  } else if (strcasecmp(pre, "SYNMOD") == 0) {
    /* Syntracker */
    process_synmod_mod(credits, credits_len, buf);

  } else if (strcasecmp(pre, "DIGI") == 0) {
    /* Digibooster */
    process_digi_mod(credits, credits_len, buf, modfilelen);

  } else if ((strcasecmp(pre, "AHX") == 0) ||
	     (strcasecmp(pre, "THX") == 0)) {
    /* AHX */
    process_ahx_mod(credits, credits_len, buf, modfilelen);

  } else if (strcasecmp(pre, "DL") == 0) {
    process_WTWT_mod(credits, credits_len, buf, modfilelen, "UNCL", "EART",
		     0x28);

  } else if (strcasecmp(pre, "BSS") == 0) {
    process_WTWT_mod(credits, credits_len, buf, modfilelen, "BEAT", "HOVE",
		     0x1c);

  } else if (strcasecmp(pre, "GRAY") == 0) {
    process_WTWT_mod(credits, credits_len, buf, modfilelen, "FRED", "GRAY",
		     0x10);

  } else if (strcasecmp(pre, "JMF") == 0) {
    process_WTWT_mod(credits, credits_len, buf, modfilelen, "J.FL", "OGEL",
		     0x14);

  } else if (strcasecmp(pre, "SPL") == 0) {
    process_WTWT_mod(credits, credits_len, buf, modfilelen, "!SOP", "ROL!",
		     0x10);

  } else if (strcasecmp(pre, "HD") == 0) {
    process_WTWT_mod(credits, credits_len, buf, modfilelen, "H.DA", "VIES",
		     24);


  } else if (strcasecmp(pre, "RIFF") == 0) {
    process_WTWT_mod(credits, credits_len, buf, modfilelen, "RIFF", "RAFF",
		     0x14);

  } else if (strcasecmp(pre, "FP") == 0) {
    process_WTWT_mod(credits, credits_len, buf, modfilelen, "F.PL", "AYER",
		     0x8);

  } else if (strcasecmp(pre, "CORE") == 0) {
    process_WTWT_mod(credits, credits_len, buf, modfilelen, "S.PH", "IPPS",
		     0x20);

  } else if (strcasecmp(pre, "BDS") == 0) {
    process_WTWT_mod(credits, credits_len, buf, modfilelen, "DAGL", "ISH!",
		     0x14);

  } else {
    /* UNKN */
    strlcat(credits, "\n\n(no further module info available, yet)",
	    credits_len);
  }


out:
  free(buf);
  return;
}

/* Get the info out of the protracker module data*/
static void process_ptk_mod(char *credits, int credits_len, int inst,
			    unsigned char *buf, int len)
{
  int i;

  if (inst == 31) {
    if (len >= 0x43c) {
      //strlcat(credits, "magic ID:  ", credits_len);
      //strlncat(credits, buf + 0x438, credits_len, 4);
      strlcat(credits, g_strdup_printf("max positions:  %d\n", buf[0x3b6]),
	      credits_len);
    }
  } else {
    if (len >= 0x1da) {
      strlcat(credits, g_strdup_printf("max positions:  %d\n", buf[0x1d6]),
	      credits_len);
    }
  }

  if (len >= (0x14 + inst * 0x1e)) {
    for (i = 0; i < inst; i++) {
      if (i < 10) {
	strlcat(credits, g_strdup_printf("\ninstr #0%d:  ", i), credits_len);
      } else {
	strlcat(credits, g_strdup_printf("\ninstr #%d:  ", i), credits_len);
      }
      strlncat(credits, buf + 0x14 + (i * 0x1e), credits_len, 22);
    }
  }
}

/* Get the info out of the Art of Noise module data*/
static void process_aon_mod(char *credits, int credits_len,
			    unsigned char *buf, int modfilelen)
{
  int offset;
  unsigned int len;


  offset = find_tag(buf, "NAME", 0, modfilelen);
  if (offset != -1) {

    len = readbig_lw(buf + offset + 4);
    if (len != 0) {
      strlcat(credits, "module name:  ", credits_len);
      strlncat(credits, buf + offset + 8, credits_len, len);
    }
  }

  offset = find_tag(buf, "AUTH", 0, modfilelen);
  if (offset != -1) {

    len = readbig_lw(buf + offset + 4);
    if (len != 0) {
      strlcat(credits, "\nauthor/composer:  ", credits_len);
      strlncat(credits, buf + offset + 8, credits_len, len);
    }
  }

  offset = find_tag(buf, "DATE", 0, modfilelen);
  if (offset != -1) {

    len = readbig_lw(buf + offset + 4);
    if (len != 0) {
      strlcat(credits, "\ncreation date:  ", credits_len);
      strlncat(credits, buf + offset + 8, credits_len, len);
    }
  }

  offset = find_tag(buf, "RMRK", 0, modfilelen);
  if (offset != -1) {
    len = readbig_lw(buf + offset + 4);
    if (len != 0) {
      strlcat(credits, "\n\nremarks:\n", credits_len);
      strlncat(credits, buf + offset + 8, credits_len, len);
    }
  }
}

/* Get the info out of the Mugician 1&2 module data*/
static void process_dmu_mod(char *credits, int credits_len,
			    unsigned char *buf)
{
  int i, maxsong;

  maxsong = get_max_subsong();

  strlcat(credits, "\nsongtitles:", credits_len);
  for (i = 0; i <= maxsong; i++) {
    strlcat(credits, g_strdup_printf("\nsubsong #%d:  ", i), credits_len);
    strlncat(credits, buf + 0x50 + (i * 0x10), credits_len, 12);
  }

}

/* Get the info out of the Deltamusic 2 module data*/
static void process_dm2_mod(char *credits, int credits_len,
			    unsigned char *buf)
{

  strlcat(credits, "\nremarks:", credits_len);
  strlcat(credits, g_strdup_printf("\n%s", buf + 0x148), credits_len);
}

/* Get the info out of the TFMX module data*/
static void process_tfmx_mod(char *credits, int credits_len,
			     unsigned char *buf)
{
  int i;

  strlcat(credits, "\nremarks:", credits_len);
  for (i = 0; i < 0x7; i++) {
    strlcat(credits, g_strdup_printf("\n\t"), credits_len);
    strlncat(credits, buf + 0x10 + (i * 40), credits_len, 40);
  }
}

/* Get the info out of the MON module data*/
static void process_mon_mod(char *credits, int credits_len,
			    unsigned char *buf)
{
  int len;

  len = (buf[3] + (buf[2] << 8) + 2);	/* get rel offset (+2) */
  len = len - 0x0c;		/* len of our remark text */

  if (len > 0) {
    strlcat(credits, "\nremarks:\n\t", credits_len);

    if (buf[0xc] != 0x0f) {
      strlncat(credits, buf + 0x0c, credits_len, len);	/* rubicon */
    } else {
      strlncat(credits, buf + 0x0e, credits_len, len - 2);	/* dragonwars endtheme */
    }
  }
}

/* Get the info out of the Syntracker module data*/
static void process_synmod_mod(char *credits, int credits_len,
			       unsigned char *buf)
{
  int i;

  strlcat(credits, "\nremarks:", credits_len);
  for (i = 0; i < 0x20; i++) {
    strlcat(credits, g_strdup_printf("\n\t"), credits_len);
    strlncat(credits, buf + 0x14 + (i * 0x20), credits_len, 0x20);
  }
}

/* Get the info out of the DigiBoostertracker module data*/
static void process_digi_mod(char *credits, int credits_len,
			     unsigned char *buf, int modfilelen)
{
  int i;

  /* get meta only if the file is enough long to get all instruction names */
  if (modfilelen < (642 + 0x20 * 0x1e))
    return;

  strlcat(credits, "Songname:  ", credits_len);
  strlncat(credits, buf + 610, credits_len, 0x1f);
  strlcat(credits, g_strdup_printf("\nmax positions:  %d\n\n", buf[47]),
	  credits_len);

  for (i = 0; i < 0x1f; i++) {
    if (i < 10) {
      strlcat(credits, g_strdup_printf("\ninstr #0%d:  ", i), credits_len);
    } else {
      strlcat(credits, g_strdup_printf("\ninstr #%d:  ", i), credits_len);
    }
    strlncat(credits, buf + 642 + (i * 0x1e), credits_len, 0x1e);
  }
}

/* Get the info out of the AHX  module data*/
static void process_ahx_mod(char *credits, int credits_len,
			    unsigned char *buf, int modfilelen)
{
  int i, offset;


  offset = readbig_w(buf + 4);

  if (offset < modfilelen) {
    strlcat(credits, g_strdup_printf("\nSongtitle: %s\n", buf + offset),
	    credits_len);

    for (i = 0; i < buf[12]; i++) {

      offset = offset + 1 + strlen(buf + offset);

      if (offset < modfilelen) {
	strlcat(credits, g_strdup_printf("\n\t%s", buf + offset),
		credits_len);
      }

    }
  }
}

/* Wanted Team's loadseg modules */
static void process_WTWT_mod(char *credits, int credits_len,
			     unsigned char *buf, int modfilelen, char *lo,
			     char *hi, int rel)
{
  int offset, txt_offset, chunk;

  offset = find_tag(buf, lo, 0, modfilelen);	/* check for Magic ID */
  if (offset == -1)
    return;
  offset = find_tag(buf, hi, offset + 4, offset + 8);
  if (offset == -1)
    return;

  chunk = offset - 8;		/* here's where our first chunk should be */
  offset = offset + rel;	/* offset to our info pointers */

  if (chunk < modfilelen && offset < modfilelen) {

    txt_offset = readbig_lw(buf + offset) + chunk;
    if (txt_offset < modfilelen && txt_offset != chunk)
      strlcat(credits,
	      g_strdup_printf("\nMODULENAME:\n %s\n", buf + txt_offset),
	      credits_len);

    txt_offset = readbig_lw(buf + offset + 4) + chunk;
    if (txt_offset < modfilelen && txt_offset != chunk)
      strlcat(credits,
	      g_strdup_printf("\nAUTHORNAME:\n %s\n", buf + txt_offset),
	      credits_len);

    txt_offset = readbig_lw(buf + offset + 8) + chunk;
    if (txt_offset < modfilelen && txt_offset != chunk)
      strlcat(credits,
	      g_strdup_printf("\nSPECIALINFO:\n %s", buf + txt_offset),
	      credits_len);
  }
}

static int find_tag(unsigned char *buf, char *tag, int startoffset,
		    int buflen)
{
  int i;

  if (startoffset > buflen - 4)
    return -1;

  for (i = startoffset; i < buflen - 3; i++) {
    if (buf[i] == tag[0] && buf[i + 1] == tag[1] &&
	buf[i + 2] == tag[2] && buf[i + 3] == tag[3]) {
      return i;
    }
  }
  return -1;
}

static unsigned int readbig_lw(char *ptr)
{
  unsigned char *p = (unsigned char *) ptr;
  unsigned int x = p[3] + (p[2] << 8) + (p[1] << 16) + (p[0] << 24);
  return x;
}

static unsigned int readbig_w(char *ptr)
{
  unsigned char *p = (unsigned char *) ptr;
  unsigned int x = p[1] + (p[0] << 8);
  return x;
}
