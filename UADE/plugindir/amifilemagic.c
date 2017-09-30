/* XMMS UADE plugin
 *
 * Copyright (C) 2000-2003  Heikki Orsila
 *                          heikki.orsila@iki.fi
 *                          http://uade.ton.tut.fi
 *
 *  This module mostly written by Michael Doering.
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

 Notes from shd about security:

 This module tries to avoid any buffer overruns by not copying anything but
 hard coded strings (such as "FC13") into pre or post strings. This doesn't
 copy any data from modules to program memory. Any memory writing with
 non-hard-coded data is an error by assumption. This module will only
 determine the format of a given module.

 Occasional memory reads over buffer ranges can occur, but they will of course
 be fixed when spotted :P The worst that can happen with reading over the
 buffer range is core dump :)

 Disclaimer from shd:

 We know the code in this module is ugly, but please show us the way to
 write this cleanly.

 Disclaimer from mld:
 
 all I care is that it works :)


*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>

#include "amifilemagic.h"

static int chk_id_offset(unsigned char *buf, int bufsize, const char *patterns[], int offset, char *pre);

/* mod patterns at file offset 0x438 */
const char *mod_patterns[] = {"M.K.", "N.T.", "M!K!", "M&K!", ".M.K", 0};
/* startrekker patterns at file offset 0x438 */
const char *startrekker_patterns[] = {"FLT4", "FLT8", "EXO4", "EXO8", 0};

/* IMPORTANT: '\0' characters do not work in patterns */
const char *offset_0000_patterns[] = {
 /*			ID:		Prefix:	   	Desc:		*/
			"DIGI Booster", "DIGI",	   /* Digibooster */
			"OKTASONG",	"OKT",	   /* Oktalyzer */
			"SYNTRACKER",	"SYNMOD",  /* Syntracker */
			"OBISYNTHPACK", "OSP",	   /* Synthpack */
			"SOARV1.0",	"SA",	   /* Sonic Arranger*/
			"AmBk",		"ABK",	   /* Amos ABK */
			"FUCO",		"BSI",	   /* FutureComposer BSI*/
			"MMU2",		"DSS",	   /* DSS */
			"GLUE",		"GLUE",	   /* GlueMon */
			"ISM!",		"IS",	   /* In Stereo */
			"IS20",		"IS20",	   /* In Stereo 2 */
			"SMOD",		"FC13",	   /* FC 1.3 */
			"FC14",		"FC14",	   /* FC 1.4 */
			"MMDC",		"MMDC",	   /* Med packer */
			"MCMD",		"MCMD",	   /* Med packer */
			"MSOB",		"MSO",	   /* Medley */
			"MODU",		"NTP",	   /* Novotrade */
			"COSO",		"HIPC",	   /* Hippel Coso */
			"BeEp",		"JAM",	   /* Jamcracker */
			"ALL ",		"DM1",	   /* Deltamusic 1 */
			"YMST",		"YM",	   /* MYST ST-YM*/
			"AMC ", 	"AMC",	   /* AM-Composer */
			"P40A",		"P40A",	   /* The Player 4.0a */
			"P40B",		"P40B",	   /* The Player 4.0b */
			"P41A",		"P41A",	   /* The Player 4.1a */
			"P60A",         "P60A",    /* The Player 6.0a */
			"SNT!", 	"PRU2",	   /* Prorunner 2 */
			"MEXX_TP2",	"TP2",	   /* Tracker Packer 2 */
			"CPLX_TP3",	"TP3",	   /* Tracker Packer 3 */
			"MEXX",		"TP1",	   /* Tracker Packer 2 */
			"PM40",		"PM40",	   /* Promizer 4.0 */
			"FC-M",		"FC-M",	   /* FC-M */
			"E.M.S. V6.",   "EMSV6",   /* EMS version 6 */
			"MCMD",		"MCMD_org", /* 0x00 MCMD format */
			"STP3",		"STP3",     /* Soundtracker Pro 2 */
			"MTM",		"MTM",      /* Multitracker */
			"Extended Module:",		"XM",      /* Fasttracker2*/
    			NULL, 		NULL};
	
const char *offset_0024_patterns[] = {
 /*			ID:		Prefix:	   	Desc:		*/
			"UNCLEART",	"DL",		/* Dave Lowe WT */					
			"DAVELOWE",	"DL_deli",	/* Dave Lowe Deli */
			"J.FLOGEL",	"JMF",		/* Janko Mrsic-Flogel */
			"BEATHOVEN",	"BSS",		/* BSS */
			"FREDGRAY",	"GRAY",		/* Fred Gray */
			"H.DAVIES",	"HD",		/* Howie Davies */
			"RIFFRAFF",	"RIFF",		/* Riff Raff */
			"!SOPROL!",	"SPL",		/* Soprol */
			"F.PLAYER",	"FP",		/* F.Player */
			"S.PHIPPS",	"CORE",		/* Core Design */
			"DAGLISH!",	"BDS",		/* Benn Daglish */
    			NULL, 		NULL};


/* check for 'pattern' in 'buf'.
   the 'pattern' must lie inside range [0, maxlen) in the buffer.
   returns true if pattern is at buf[offset], otherwrise false
 */
static int patterntest(const char *buf, const char *pattern,
		       int offset, int bytes, int maxlen)
{
  if ((offset + bytes) <= maxlen) {
    return memcmp(buf + offset, pattern, bytes) ? 0 : 1;
  }
  fprintf(stderr, "uade: warning: would have searched filemagic outside of range\n");
  return 0;
}

static int tronictest(unsigned char *buf, int bufsize) {
  int a = 0;
  
  a = ((buf[0x02] <<8) + buf[0x03]) + ((buf[0x06] <<8) + buf[0x07]) +
    ((buf[0x0a] <<8) + buf[0x0b]) + ((buf[0x0e] <<8) + buf[0x0f]) + 0x10;

  if ((a+1 > bufsize) || (a & 1<<0)) return 0;	/* size  & btst #0, d1;*/

  a = ((buf[a] <<8) + buf[a+1]) + a;
  if ((a+7 > bufsize) || (a & 1<<0)) return 0; /*size & btst #0,d1*/
  
  if ( ( ((buf[a+4] <<24) + (buf[a+5] <<16) +
	  (buf[a+6] <<8)  +  buf[a+7]) != 0x5800b0)) return 0;

  return 1;
}

static int tfmxtest(unsigned char *buf, int bufsize, char *pre, char *post) {

  int ret = 0;

  if (buf[0] == 'T' && buf[1] == 'F' && buf[2] =='H' && buf[3] =='D') {
    if (buf[0x8] == 0x01) {
      strcpy (pre, "TFHD1.5");		/* One File TFMX format */
      strcpy (post, "");		/* by Alexis NASR */
      ret = 1;
    } else if (buf[0x8] == 0x02) {
      strcpy (pre, "TFHDPro");
      strcpy (post, "");
      ret = 1;
    } else if (buf[0x8] == 0x03) {
      strcpy (pre, "TFHD7V");
      strcpy (post, "");
      ret = 1;
    }

  } else if ((buf[0] == 'T' && buf[1] == 'F' && buf[2] =='M' && buf[3] == 'X')||
	     (buf[0] == 't' && buf[1] == 'f' && buf[2] =='m' && buf[3] == 'x'))  {

    
    if ((buf [4] == '-' &&  buf[5] == 'S' && buf[6] =='O' && buf[7] == 'N' && buf[8] == 'G')||
	(buf [4] == '_' &&  buf[5] == 'S' && buf[6] =='O' && buf[7] == 'N' && buf[8] == 'G' && buf[9] == ' ')||
	(buf [4] == 'S' &&  buf[5] == 'O' && buf[6] =='N' && buf[7] == 'G')||
	(buf [4] == 's' &&  buf[5] == 'o' && buf[6] =='n' && buf[7] == 'g')||
	(buf [4] == 0x20)) {

	strcpy (pre, "MDAT");	/*default TFMX: TFMX Pro*/
	strcpy (post, "");
	ret = 1;

      if ((buf [10] =='b'  && buf[11] =='y')  ||
	  (buf [16] == ' ' && buf[17] ==' ')  ||
	  (buf [16] == '(' && buf[17] =='E' && buf[18] == 'm' && buf[19] =='p' && buf[20] =='t' && buf[21] == 'y' && buf[22] ==')' ) ||
	  (buf [16] == 0x30 && buf[17] == 0x3d) || /*lethal Zone*/
	  (buf [4]  == 0x20)) {
	if (buf[464]==0x00 && buf[465]==0x00 && buf[466]==0x00 && buf[467]==0x00) {
	  if ((buf [14]!=0x0e && buf[15] !=0x60) || /*z-out title */
	      (buf [14]==0x08 && buf[15] ==0x60 && buf[4644] != 0x09 && buf[4645] != 0x0c) || /* metal law */
	      (buf [14]==0x0b && buf[15] ==0x20 && buf[5120] != 0x8c && buf[5121] != 0x26) || /* bug bomber */
	      (buf [14]==0x09 && buf[15] ==0x20 && buf[3876] != 0x93 && buf[3977] != 0x05)) { /* metal preview */
	    strcpy (pre, "TFMX1.5");	/*TFMX 1.0 - 1.6*/
	    strcpy (post, "");
	  }
	}
      } else if (((buf[0x0e]== 0x08 && buf[0x0f] ==0xb0) &&   /* BMWi */
		  (buf[0x140] ==0x00 && buf[0x141]==0x0b) && /*End tackstep 1st subsong*/
		  (buf[0x1d2]== 0x02 && buf[0x1d3] ==0x00) && /*Trackstep datas*/

		  (buf[0x200] == 0xff && buf[0x201] ==0x00 && /*First effect*/
		   buf[0x202] == 0x00 && buf[0x203] ==0x00 &&
		   buf[0x204] == 0x01 && buf[0x205] ==0xf4 && 
		   buf[0x206] ==0xff && buf[0x207] ==0x00)) ||

		 ((buf[0x0e]== 0x0A && buf[0x0f] ==0xb0) && /* B.C Kid */
		  (buf[0x140] ==0x00 && buf[0x141]==0x15) && /*End tackstep 1st subsong*/
		  (buf[0x1d2]== 0x02 && buf[0x1d3] ==0x00) && /*Trackstep datas*/

		  (buf[0x200] == 0xef && buf[0x201] ==0xfe && /*First effect*/
		   buf[0x202] == 0x00 && buf[0x203] ==0x03 &&
		   buf[0x204] == 0x00 && buf[0x205] ==0x0d && 
		   buf[0x206] ==0x00 && buf[0x207] ==0x00)))  {
	strcpy (pre, "TFMX7V");	/* "special cases TFMX 7V*/
	strcpy (post, "");


      } else {

	int e, i, s, t;

	/* Trackstep datas offset */
	if (buf[0x1d0] ==0x00 && buf[0x1d1] ==0x00 && buf[0x1d2] ==0x00 && buf[0x1d3] ==0x00) {
	  /* unpacked*/
	  s = 0x00000800; 
	} else {
	  /*packed*/
	  s = (buf[0x1d0] <<24) + (buf[0x1d1] <<16) + (buf[0x1d2] <<8) + buf[0x1d3]; /*packed*/
	}

	for (i = 0; i < 0x3d; i += 2) {
	  if (( (buf[0x140+i] <<8 ) +buf[0x141+i]) > 0x00 ) { /*subsong*/
	    t = (((buf[0x100+i]<<8) +(buf[0x101+i]))*16 +s ); /*Start of subsongs Trackstep data :)*/
	    e = (((buf[0x140+i]<<8) +(buf[0x141+i]))*16 +s ); /*End of subsongs Trackstep data :)*/
	    if (t < bufsize || e < bufsize) {
	      for (t = t ; t < e ; t += 2) {
		if (buf[t] == 0xef && buf[t+1] == 0xfe) {
		  if (buf[t+2] == 0x00 && buf[t+3] == 0x03 &&
		      buf[t+4] == 0xff && buf[t+5] == 0x00 && buf[t+6] == 0x00) {
		    i=0x3d;
		    strcpy (pre, "TFMX7V");	/*TFMX 7V*/
		    strcpy (post, "");
		    break;
		  }
		}
	      }
	    }
	  }
	}
      }
    }
  }
  return ret;
}

/* returns:	 -1 for a mod with bad length 		*/
/* 		 0 for no mod or not checked		*/
/*		 1 for a mod with good length		*/
static int modlentest(unsigned char *buf, int filesize, char *pre) {
  int ret = 0;
  int i = 0;
  int no_of_instr = 15;
  int smpl = 0;
  int header = 600;
  int plist = 600-130;
  int maxpattern = 0;

  if (strcasecmp(pre, "MOD") == 0) {
    no_of_instr = 31;
    header = 1084;
    plist = 1084-4-130;
  } else if ((strcasecmp(pre, "MOD15") != 0) || 
	     (strcasecmp(pre, "MOD_UST") != 0)) return 0;

  if (header > filesize) return 0; /* no mod */
  for (i = 0; i < 128; i++) {
    if (buf[plist+2+i] > maxpattern) maxpattern = buf[plist+2+i];
  }
  if (maxpattern > 100) return 0;
  
  if (buf[43]+no_of_instr*30 > filesize) return 0; /* no mod*/  
  if (buf[43]+no_of_instr*30 > 5122) return 0; /* not enough data in buffer*/  
  
  for (i = 0; i < no_of_instr; i++) {
    smpl = smpl + (buf[42+i*30] <<8)  +  buf[43+i*30]; /* smpl len  */
  }

  if ( (filesize < (header+(maxpattern+1)*1024+smpl*2)) ||
       (filesize > (header+(maxpattern+1)*1024+smpl*2)+1024) ){
    fprintf(stderr, "uade: *** WARNING *** calculated length %d doesn't match the file length %d\n", header+(maxpattern+1)*1024+smpl*2, filesize);
    ret=-1;
  } else {
    ret=1; /*size ok, sort of*/
  }
  return ret;
}

/* returns:	 0 for an undefined mod 		*/
/* 		 1 for a DOC Soundtracker mod		*/
/*		 2 for a Ultimate ST mod		*/
/*		 3 for a SoundtrackerV2.0 -V4.0 (todo)	*/
static int mod15check(unsigned char *buf, int bufsize) {
  int i = 0;
  int slen = 0;
  int srep = 0;
  int sreplen = 0;
  int vol = 0;
  int ret = 0;

  if (bufsize < 0x1f3) return 0; /* file too small */
  if (bufsize < 49+15*30) return 0; /* file too small */

  if (buf[0x1d6] != 0x00 && buf[0x1d6] < 0x78 && buf[0x1f3] != 1){
    for (i = 0; i < 128; i++) { /* pattern step table: 128 posbl. entries*/
      if (buf[600-130+2+i] > 63) return 0;  /*can be 0 -63 */
    }

    for (i = 0; i < 15; i++) {
      vol =buf[45+i*30];
      slen =((buf[42+i*30] <<8)  +  buf[43+i*30]) *2;
      srep =((buf[46+i*30] <<8)  +  buf[47+i*30]);
      sreplen = ((buf[48+i*30] <<8)  +  buf[49+i*30]) *2;
      //fprintf (stderr, "%d, slen: %d, %d (srep %d, sreplen %d), vol: %d\n",i, slen, srep+sreplen,srep, sreplen, vol);
      if ((vol > 0x40) ||
	  (sreplen+srep > slen && slen != 0) ||
	  //(slen == 0 && vol != 0) ||
	  (slen == 0 && sreplen >2 )) return 0;
    }
    ret=1;/* mod15*/
    fprintf(stderr, "*** INFO *** if it sounds broken or isn't played it could also be an Ultimate-ST file: use -P and -M parameter to overide autodetectiong\n");
  }
  

  if (ret==1 &&  buf [0x1d7] != 00 ){  /* could be ust */
    for (i = 0; i < 15; i++) {
      if ( ( ((buf[20+i*30] <<24) + (buf[21+i*30] <<16) + /* no empty  */
	      (buf[22+i*30] <<8)  +  buf[23+i*30]) != 0)) { /* smpl name*/
	
	/* slen < 9999 */
	slen = (buf[42+i*30] <<8)  +  buf[43+i*30];
	if (slen <= 9999){
	  /* repeat offset + repeat size*2 <0xffff */
	  srep  =((buf[48+i*30] <<8)  +  buf[49+i*30])*2 +
	    ((buf[46+i*30] <<8)  +  buf[47+i*30]);
	  if (srep  > 0xffff) return ret;
	  
	} else {
	  return ret;	/* seems to be a plain mod15 */
	}
      }
    }

    if  (buf[0x1d7] != 0x78) ret= 2;
    /* check smpl names */
    if (ret == 1) {
      for (i = 0; i < 15; i++) {
	if ( ( ((buf[20+i*30] <<24) + (buf[21+i*30] <<16) + /* no empty  */
		(buf[22+i*30] <<8)  +  buf[23+i*30]) != 0)) { /* smpl name  */

	  if (buf[25+i*30] == ':' && buf [22+i*30] == '-' &&
	    ((buf[20+i*30] == 'S' && buf [21+i*30] == 'T') ||
	     (buf[20+i*30] == 's' && buf [21+i*30] == 't'))) return ret;
	}
      }
      /* TODO: checks for the two UST effects  0+1 */
      //ret = 2;
    }
  }
  return ret;
}

#if 0
/* USELESS CRAP */
static int music_maker_48_test(unsigned char *buf, char *pre, char *post, 
			       int size)
{
  if (size < 24)
    return 0;

  if (strcasecmp(post, "sdata") != 0)
    return 0;

  if ((buf[0] == 'S' && buf[1] == 'E' && buf[22] != 0xff) ||
      (buf[0] != 0xff)) {
    if (buf[23] >= 0x10 && buf[23] <= 0x40) {
      strcpy(pre, "MM4");
      post[0] = 0;
      return 1;
    }
  }

  if ((buf[0] == 'S' && buf[1] == 'E' && buf[22] != 0xff) ||
      (buf[0] != 0xff)) {
    if (buf[25] >= 123 && buf[23] <= 224) {
      strcpy(pre, "MM8");
      post[0] = 0;
      return 1;
    }
  }
  return 0;
}
#endif

void filemagic(unsigned char *buf, char *pre, char *post, int realfilesize) {
      /* char filemagic():
	detects formats like e.g.: tfmx1.5, hip, hipc, fc13, fc1.4	 
	    - tfmx 1.5 checking based on both tfmx DT and tfmxplay by jhp, 
	      and the EP by Don Adan/WT.
	    - tfmx 7v checking based on info by don adan, the amore file
	      ripping description and jhp's desc of the tfmx format.
	    - other checks based on e.g. various player sources from Exotica 
	      or by checking bytes with a hexeditor
	by far not complete...

	NOTE: Those Magic ID checks are quite lame compared to the checks the
	amiga replayer do... well, after all we are not ripping. so they
	have to do at the moment :)
       */

  int i, j, t;
  const int bufsize = 5122;

  for (i = 0; mod_patterns[i]; i++) {
    if (patterntest(buf, mod_patterns[i], 0x438, 4, bufsize)) {
      /* generic protracker or clone */
      strcpy(pre, "MOD");
      strcpy(post, "");
      if (modlentest (buf, realfilesize, pre) <0) { /*modlen ok?*/
	//strcpy(pre, "");
	//strcpy (post, "");
      }
      return;
    }
  }
  
  for (i=0; startrekker_patterns[i]; i++) {
    if (patterntest(buf, startrekker_patterns[i], 0x438, 4, bufsize)) {
      t = 0;
      for (j=0; j<30*0x1e; j=j+0x1e) {
	if (buf[0x2a + j] == 0 && buf[0x2b + j] == 0 && buf[0x2d + j] != 0) {
	  t = t + 1;		/* no of AM instr. */
	}
      }
      if (t > 0){
	strcpy (pre, "ADSC");	/* Startrekker 4 AM / ADSC*/
	strcpy (post, "");
      } else {
	strcpy (pre, "MOD");	/* generic Startrekker MOD*/
	strcpy (post, "");
	if (modlentest (buf, realfilesize, pre) <0){
	  //strcpy(pre, "");
	  //strcpy (post, ""); 
	}
      }
      return;
    }
  }

 t = mod15check(buf, bufsize);
 if (t > 0) {
  strcpy (pre, "MOD15");		/*normal Soundtracker 15*/
  strcpy (post, "");
    if (t == 2) {
    strcpy (pre, "MOD_UST"); /*Ultimate ST*/
    strcpy (post, "");
    }
    if (modlentest(buf, realfilesize, pre) < 0){
    strcpy (pre, "");
    strcpy (post, "");
    }
 }

 if (((buf[0x438] >= '1' && buf[0x438] <= '3') && (buf[0x439] >= '0' && buf[0x439] <= '9') && buf[0x43a] =='C' && buf[0x43b] =='H') ||
              ((buf[0x438] >= '2' && buf[0x438] <= '8') && buf[0x439] == 'C' && buf[0x43a] =='H' && buf[0x43b] =='N')||
	       (buf[0x438] == 'T' && buf[0x439] == 'D' && buf[0x43a] =='Z')||
	       (buf[0x438] == 'O' && buf[0x439] == 'C' && buf[0x43a] =='T' && buf[0x43b] =='A')||
	       (buf[0x438] == 'C' && buf[0x439] == 'D' && buf[0x43a] =='8' && buf[0x43b] =='1')){
	       strcpy (pre, "MOD_PC");		/*Multichannel Tracker*/
	       strcpy (post, "");

   } else if (buf[0x2c] == 'S' && buf[0x2d] == 'C' && buf[0x2e] == 'R' && buf[0x2f] == 'M'){
	       strcpy (pre, "S3M");		/*Scream Tracker*/
	       strcpy (post, "");

   } else if ((buf[0] == 0x60 && buf[2] == 0x60 && buf[4] ==0x48 && buf[5] == 0xe7) ||
	      (buf[0] == 0x60 && buf[2] == 0x60 && buf[4] ==0x41 && buf[5] == 0xfa) ||
	      (buf[0] == 0x60 && buf[1] == 0x00 && buf[4] == 0x60 && buf[5] == 0x00 && buf[8] ==0x48 && buf[9] == 0xe7)||
	      (buf[0] == 0x60 && buf[1] == 0x00 && buf[4] == 0x60 && buf[5] == 0x00 && buf[8] ==0x60 && buf[9] == 0x00 && buf[12] ==0x60 && buf[13] == 0x00 && buf[16] ==0x48 && buf[17] == 0xe7)){
	       strcpy (pre, "HIP");		/* Hippel*/
	       strcpy (post, "");

   } else if (buf[0x348] == '.' && buf[0x349] == 'Z' && buf[0x34A] =='A' && buf[0x34B] =='D' &&
              buf[0x34c] == 'S' && buf[0x34d] == '8' && buf[0x34e] =='9' && buf[0x34f] =='.'){
	       strcpy (pre, "MKII");		/* Mark II*/
	       strcpy (post, "");

   } else if (((buf[0] == 0x08 && buf[1] == 0xf9 && buf[2] ==0x00 && buf[3] == 0x01) &&
	       (buf[4] == 0x00 && buf[5] == 0xbb && buf[6] ==0x41 && buf[7] == 0xfa) &&
	        ((buf[0x25c] == 0x4e && buf[0x25d] == 0x75) || (buf[0x25c] == 0x4e && buf[0x25d] == 0xf9))) ||
	      ((buf[0] == 0x41 && buf[1] == 0xfa) &&
	       (buf[4] == 0xd1 && buf[5] == 0xe8) &&
	        (((buf[0x230] == 0x4e && buf[0x231] == 0x75) || (buf[0x230] == 0x4e && buf[0x231] == 0xf9)) ||		 
	         ((buf[0x29c] == 0x4e && buf[0x29d] == 0x75) || (buf[0x29c] == 0x4e && buf[0x29d] == 0xf9))
		   ))){		 
	       strcpy (pre, "SID1");		/* SidMon1 */
	       strcpy (post, "");

   } else if (buf[0] == 0x4e && buf[1] == 0xfa && 
               buf[4] == 0x4e && buf[5] == 0xfa &&
               buf[8] == 0x4e && buf[9] == 0xfa &&
               buf[0xc] == 0x4e && buf[0xd] == 0xfa){
	        for (i=0x10; i<256; i=i+2){
		  if (buf[i+0] == 0x4e && buf[i+1] ==0x75 && buf[i+2] == 0x47 && buf[i+3] == 0xfa &&
		     buf[i+12] == 0x4e && buf[i+13] ==0x75
		     ){ 
        	       strcpy (pre, "FRED");		/* FRED */
    		       strcpy (post, "");
    		       break;
	           }
		  }

   } else if (buf[0] == 0x60 && buf[1] == 0x00 && 
              buf[4] == 0x60 && buf[5] == 0x00 && 
              buf[8] == 0x60 && buf[9] == 0x00 &&
              buf[12] == 0x48 && buf[13] == 0xe7) {
	       strcpy (pre, "MA");		/*Music Assembler*/
	       strcpy (post, "");

   } else if (buf[0] == 0x00 && buf[1] == 0x00 &&
              buf[2] == 0x00 && buf[3] == 0x28 && 
	      (buf[7] >= 0x34 && buf[7] <= 0x64)&&
              buf[0x20] == 0x21 && (buf[0x21] == 0x54 || buf[0x21] ==0x44 ) &&
	      buf[0x22] == 0xff &&  buf[0x23] == 0xff) {
	       strcpy (pre, "SA-P");		/*SonicArranger Packed*/
	       strcpy (post, "");

   } else if (buf[0] == 0x4e && buf[1] == 0xfa &&
              buf[4] == 0x4e && buf[5] == 0xfa && 
	      buf[8] == 0x4e && buf[9] == 0xfa)   {
     t = ((buf[2]*256) + buf[3]);
     if (t < bufsize - 9){
       if (buf[2+t] == 0x4b && buf [3+t] == 0xfa &&
	   buf[6+t] == 0x08 && buf [7+t] == 0xad && buf[8+t] == 0x00 && buf[9+t] == 0x00) {
	 strcpy (pre, "MON");		/*M.O.N*/
	 strcpy (post, "");
       }
     }			

   } else if (buf[0] == 0x02 && buf[1] == 0x39 &&
              buf[2] == 0x00 && buf[3] == 0x01 && 
              buf[8] == 0x66 &&  buf[9] == 0x02 &&
	      buf[10] == 0x4e && buf[11] == 0x75 &&
	      buf[12] == 0x78 && buf[13] == 0x00 &&
	      buf[14] == 0x18 && buf[15] == 0x39 ){
		          strcpy (pre, "MON_old");		/*M.O.N_old*/
		          strcpy (post, "");			

   } else if (buf[0] == 0x48 && buf[1] == 0xe7 &&  buf[2] == 0xf1 && buf[3] == 0xfe && 
              buf[4] == 0x61 && buf[5] == 0x00) {
     t = ((buf[6]*256) + buf[7]);
     if (t < (bufsize - 17)) {
       for (i=0; i<10; i=i+2) {
	 if (buf[6+t+i] == 0x47 && buf[7+t+i] == 0xfa) {
	   strcpy (pre, "DW");	/*Whittaker Type1*/
	   strcpy (post, "");	/* FIXME: incomplete */
	 }
       }
     } 
 
   } else if  (buf[0]  == 0x13 && buf[1]  == 0xfc &&
               buf[2]  == 0x00 && buf[3]  == 0x40 &&
	       buf[8]  == 0x4e && buf[9]  == 0x71 &&
               buf[10] == 0x04 && buf[11] == 0x39 && 
	       buf[12] == 0x00 && buf[13]  == 0x01 &&               
	       buf[18] == 0x66 && buf[19] == 0xf4 &&
               buf[20] == 0x4e && buf[21] == 0x75 && 
	       buf[22] == 0x48 && buf[23] == 0xe7 &&
	       buf[24] == 0xff && buf[25] == 0xfe){
	        strcpy (pre, "EX");	/*Fashion Tracker*/
	        strcpy (post, "");


/* Magic ID */

   } else if (buf[0x3a] == 'S' && buf[0x3b] == 'I' && buf[0x3c] == 'D' && 
              buf[0x3d] == 'M' && buf[0x3e] == 'O' && buf[0x3f] == 'N' &&
	      buf[0x40] == ' ' && buf[0x41] == 'I' && buf[0x42] == 'I' ) {
	       strcpy (pre, "SID2");		/* SidMon II*/
	       strcpy (post, "");

   } else if (buf[0x28] == 'R' && buf[0x29] == 'O' && buf[0x2a] == 'N' && 
              buf[0x2b] == '_' && buf[0x2c] == 'K' && buf[0x2d] == 'L' &&
	      buf[0x2e] == 'A' && buf[0x2f] == 'R' && buf[0x30] == 'E' && 
	      buf[0x31] == 'N') {
	       strcpy (pre, "RK");		/* Ron Klaren (CustomMade) */
	       strcpy (post, "");

   } else if (buf[0x3e] == 'A' && buf[0x3f] == 'C' && buf[0x40] =='T' && buf[0x41] =='I'&&
	     buf[0x42] == 'O' && buf[0x43] == 'N' && buf[0x44] =='A' && buf[0x45] =='M') {
	       strcpy (pre, "AST");		/*Actionanamics*/
	       strcpy (post, "");

   } else if (buf[26] == 'V' && buf[27] == '.' && buf[28] =='2') {
	       strcpy (pre, "BP");    	/* Soundmon V2*/
	       strcpy (post, "");

   } else if (buf[26] == 'V' && buf[27] == '.' && buf[28] =='3') {
	       strcpy (pre, "BP3");		/* Soundmon V2.2*/
	       strcpy (post, "");

   } else if (buf[60] == 'S' && buf[61] == 'O' && buf[62] =='N' && buf[63] =='G') {
	       strcpy (pre, "SFX13");		/* Sfx 1.3-1.8*/
	       strcpy (post, "");

   } else if (buf[124] == 'S' && buf[125] == 'O' && buf[126] =='N' && buf[127] =='G') {
	       strcpy (pre, "SFX20");		/* Sfx 2.0*/
	       strcpy (post, "");

   } else if (buf[0x1a] == 'E' && buf[0x1b] == 'X' && buf[0x1c] =='I' && buf[0x1d] =='T') {
	       strcpy (pre, "AAM");		/*Audio Arts & Magic*/
	       strcpy (post, "");
   } else if (buf[8] == 'E' && buf[9] == 'M' && buf[10] =='O' && buf[11] =='D' &&
	      buf[12] == 'E' && buf[13] =='M' && buf[14] =='I' && buf[15] =='C') {
	       strcpy (pre, "EMOD");		/* EMOD*/
	       strcpy (post, "");

/* generic ID Check at offset 0x24 */

   } else if (chk_id_offset(buf, bufsize, offset_0024_patterns, 0x24, pre)){
   	       strcpy (post, ""); 

/* HIP7 ID Check at offset 0x04 */
   } else if (patterntest(buf, " **** Player by Jochen Hippel 1990 **** ", 
			  0x04, 40, bufsize)){
	       strcpy (pre, "HIP7");		/* HIP7*/
	       strcpy (post, "");

/* Magic ID at Offset 0x00 */
   } else if (buf[0] == 'M' && buf[1] == 'M' && buf[2] =='D') {
     if (buf[0x3] >= '0' && buf[0x3] < '3') {
       /*move.l mmd_songinfo(a0),a1*/
       int s = (buf[8] <<24) + (buf[9] <<16) + (buf[0xa] <<8) + buf[0xb];
       if (((int) buf[s+767]) & (1<<6)) {	/* btst #6, msng_flags(a1);*/
	 strcpy (pre, "OCTAMED");		/*OCTAMED*/
	 strcpy (post, "");
       } else {
	 strcpy (pre, "MED");		/*MED*/
	 strcpy (post, "");
       }		
     } else if (buf[0x3] != 'C') {
       strcpy (pre, "MMD3");		/* mmd3 and above*/
       strcpy (post, "");
     }

     /* all TFMX format tests here */
   } else if (tfmxtest(buf, bufsize, pre, post)) {
     /* is TFMX, nothing to do here (pre and post set in tfmxtest() */

   } else if (buf[0] == 'A' && buf[1] == 'O' && buf[2] =='N') {
    	     if (buf[3] == '4')
	     {
	       strcpy (pre, "AON4");		/* Art Of Noise*/
	       strcpy (post, "");
	     } else {
	       strcpy (pre, "AON8");
	       strcpy (post, "");
	     }

   } else if (buf[0] == 'T' && buf[1] == 'H' && buf[2] =='X') {
    	     if ((buf[3] == 0x00 )||(buf[3] == 0x01) )
	     {
	       strcpy (pre, "AHX");		/* AHX */
	       strcpy (post, "");
	     } 

   } else if (buf[1] == 'M' && buf[2] == 'U' && buf[3] =='G' && buf[4] =='I' &&
	      buf[5] == 'C' && buf[6] =='I' && buf[7] =='A'&& buf[8] =='N'){ 
    	      if (buf[9] =='2') {
	       strcpy (pre, "MUG2");		/* Digimugi2*/
	       strcpy (post, "");
	      } else {
	       strcpy (pre, "MUG");		/* Digimugi*/
	       strcpy (post, "");
	      }

   } else if (buf[0] == 'A' && buf[1] == 'R' && buf[2] =='P' && buf[3] == 0x2e) {
	       strcpy (pre, "MTP2");		/* HolyNoise / Major Tom*/
	       strcpy (post, "");


   } else if (buf[0] == 'L' && buf[1] == 'M' && buf[2] =='E' && buf[3] ==0x00) {
	       strcpy (pre, "LME");		/* LegLess*/
	       strcpy (post, "");


   } else if (buf[0] == 'P' && buf[1] == 'S' && buf[2] =='A' && buf[3] == 0x00) {
	       strcpy (pre, "PSA");		/* PSA*/
	       strcpy (post, "");

   } else if ((buf[0] == 'S' && buf[1] == 'y' && buf[2] =='n' && buf[3] =='t' && buf[4] == 'h' && buf[6] == '.' && buf[8] == 0x00) && 
	     (buf[5] > '1' && buf[5] < '4')){
	       strcpy (pre, "SYN");		/* Synthesis*/
	       strcpy (post, "");


   } else if (buf[0xbc6] == '.' && buf[0xbc7] == 'F' && buf[0xbc8] =='N' && buf[0xbc9] =='L') {
	       strcpy (pre, "DM2");		/* Delta 2.0*/
	       strcpy (post, "");

   } else if (buf[0] == 'Y' && buf[1] == 'M' && buf[3] =='!') {
  	       strcpy (pre, "");		/* don't play stplay files*/
  	       strcpy (post, "");


   } else if (buf[0] == 'R' && buf[1] == 'J' && buf[2] =='P'){
   
             if ( buf[4] == 'S' && buf[5] == 'M' && buf[6] =='O' && buf[7] =='D') {
	        strcpy (pre, "RJP");		/* Vectordean (Richard Joseph Player)*/
	        strcpy (post, "");
		} else {
		strcpy (pre, "");		/* but don't play .ins files*/
	        strcpy (post, "");
		}
   } else if (buf[0] == 'F' && buf[1] == 'O' && buf[2] =='R' && buf[3] =='M'){
             if ( buf[8] == 'S' && buf[9] == 'M' && buf[10] =='U' && buf[11] =='S') {
	        strcpy (pre, "SMUS");		/* Sonix*/
	        strcpy (post, "");
	     }
	     
  // } else if (buf[0x00] == 0x00 && buf[0x01] == 0xfe &&
  //            buf[0x30] == 0x00 && buf[0x31] ==0x00 && buf[0x32] ==0x01 && buf[0x33] ==0x40 &&
  //	      realfilesize > 332 ){
  //	     }
  //	     strcpy (pre, "SMUS");		/* Tiny Sonix*/
  //         strcpy (post, "");


   } else if (tronictest(buf, bufsize)) {
	        strcpy (pre, "TRONIC");		/* Tronic*/
	        strcpy (post, "");

/* generic ID Check at offset 0x00 */

   } else if (chk_id_offset(buf, bufsize, offset_0000_patterns, 0x00, pre)) {
     strcpy (post, ""); 
     
     /*magic ids of some modpackers*/
  } else  if (buf[0x438] == 'P' && buf[0x439] == 'W' && buf[0x43a] =='R' && buf[0x43b] == 0x2e){
	       strcpy (pre, "PPK");		/*Polkapacker*/
	       strcpy (post, "");

  } else  if (buf[0x100] == 'S' && buf[0x101] == 'K' && buf[0x102] =='Y' && buf[0x103] == 'T'){
	       strcpy (pre, "SKT");		/*Skytpacker*/
	       strcpy (post, "");

  } else  if ((buf[0x5b8] == 'I' && buf[0x5b9] == 'T' && buf[0x5ba] =='1' && buf[0x5bb] == '0')||
	      (buf[0x5b8] == 'M' && buf[0x5b9] == 'T' && buf[0x5ba] =='N' && buf[0x5bb] == 0x00)){
	       strcpy (pre, "ICE");		/*Ice/Soundtracker 2.6*/
	       strcpy (post, "");

  } else  if (buf[0x3b8] == 'K' && buf[0x3b9] == 'R' && buf[0x3ba] =='I' && buf[0x3bb] == 'S'){
	       strcpy (pre, "KRIS");		/*Kristracker*/
	       strcpy (post, "");

/* Custom file check */
   } else if (buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x03 && buf[3] == 0xf3){ /*CUSTOM*/

     i = (buf[0x0b]*4)+0x1c; /* beginning of first chunk */

     if (i < bufsize - 0x42) {

       t = 0;
       /* unfort. we can't always assume: moveq #-1,d0 rts before "delirium" */
       /* search 0x40 bytes from here, (enough?)*/
       while ((buf[i+t+0] != 'D' && buf[i+t+1] !='E' && buf[i+t+2] != 'L' && buf[i+t+3] !='I') &&
	      (t < 0x40)) {
	 t++;
       }

       if (t < 0x40) {
	 /* longword after Delirium is rel. offset from first chunk 
	    where "hopefully" the delitags are */
	 int s = (buf[i+t+10]*256)+buf[i+t+11] + i;	/* 64K */
	 if (s < bufsize - 0x33) {
	   for (i=0; i<0x30; i=i+4){
	     if (buf[i+s+0] == 0x80 && buf[i+s+1] ==0x00 && 
		 buf[i+s+2] == 0x44 && buf[i+s+3] ==0x55){ 
	       strcpy (pre, "CUST");                    /* CUSTOM */
	       strcpy (post, "");
	       break;
	     }
	   }
	 }
       }
     }

   } else if (buf[12] == 0x00) {
     int s = (buf[12]*256 + buf[13] + 1) * 14;
     if (s < (bufsize-91)) {
       if (buf[80+s] == 'p' && buf[81+s] == 'a' && buf[82+s] == 't' && buf[83+s] == 't' && buf[87+s] == 32 && 
	   buf[88+s] == 'p' && buf[89+s] == 'a' && buf[90+s] == 't' && buf[91+s] == 't') {
	 strcpy (pre, "PUMA");		/* Pumatracker */
	 strcpy (post, "");
       }
     } 
   }
}

/* We are currently stupid and check only for a few magic IDs at the offsets
 * chk_id_offset returns 1 on success and sets the right prefix/extension
 * in pre
 * TODO: more and less easy check for the rest of the 52 trackerclones
 */

static int chk_id_offset(unsigned char *buf, int bufsize, 
			 const char *patterns[], int offset, char *pre)
{
  int i;

  for (i = 0; patterns[i]; i=i+2) {
    if (patterntest(buf, patterns[i], offset,  
		    strlen(patterns[i]), bufsize)) {
      /* match found */
      strcpy (pre, patterns[i + 1]);
      return 1;
    }
  }
  return 0;
}
