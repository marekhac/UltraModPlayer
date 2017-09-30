/* XMMS UADE plugin
 *
 * Copyright (C) 2000-2003  Heikki Orsila
 *                          heikki.orsila@iki.fi
 *                          http://uade.ton.tut.fi
 *
 * This plugin is based on xmms 0.9.6 wavplayer input plugin code. Since
 * then all code has been rewritten.
 *
 * Configure and About based on code of the null-plugin by Håvard Kvålen.
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
#include <string.h>
#include <glib.h>
#include <bmp/plugin.h>
#include <bmp/util.h>
#include <libgen.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#include "defaults.h"
#include "uade-os.h"
#include "uadeconfig.h"
#include "uade.h"
#include "uademsg.h"
#include "../src/xmms-slave-msg.h"

#include "effects.h"
#include "gui.h"
#include "btree.h"
#include "checksum.h"

#include "../../osdep/strl.c"

/* defaults.c contains configurable plugin variables (and default values) */
#include "defaults.c"

static pthread_mutex_t uade_db_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t uade_check_mutex = PTHREAD_MUTEX_INITIALIZER;

/* FUNCTION DECLARATIONS */

static void uade_mutex_lock(pthread_mutex_t *m);
static void uade_mutex_unlock(pthread_mutex_t *m);

static void uade_init(void);
static int is_our_file(char *filename);
static int cmf(char *filepath, char *format, char *playername);
static int check_my_file(char *filename, char *format, char *playername);
static void play_file(char *filename);
static void stop(void);
static void set_paused(int paused);
static void uade_pause(short paused);
static int get_time(void);
static void get_song_info(char *filename, char **title, int *length);
static void clean_up(void);

static int signal_slave(int signum);
static void waitforuaetoreact(void);


static void db_add_song_data(tnode **root, char *md5sum, int length, int force);
static int *db_check_song(tnode *root, const char *filename);
static int db_get_name_hash(char *hash, const char *filename);
static int db_read_data(tnode **root, const char *filename);
static void db_write_data(const char *filename, tnode *root);
static void setup_databases(void);
static int uadedbwriteroutine(tnode *node, void *arg);


/* GLOBAL VARIABLE DECLARATIONS */

InputPlugin uade_ip = {
	0,
	0,
	"UADE " VERSION,
	uade_init,        /* input plugin initialization (run uade) */
	uade_about,       /* about gui */
	uade_configure,   /* configuration gui */
	is_our_file,      /* checks if a file is for uade playback */
	0,
	play_file,        /* plays a song */
	stop,             /* stops playing */
	uade_pause,       /* pauses playback */
	uade_seeksubsong, /* changes subsong */
	0,
	get_time,         /* sets xmms playtime */
	0,
	0,
	clean_up,         /* this deletes posix tmp-file */
	0,
	0,
	0,
	0,
	get_song_info,    /* gives xmms general info about played song */
	uade_modinfo,      /* file_info_box */
	0
};

/* the name of the song that is being played. 0 if nothing is being played. */
char *uade_song_basename = 0;
char *uade_song_full_name = 0;

int uadeformats_is_cached = 0;

/* LOCAL VARIABLE DECLARATIONS */

static pid_t slavepid = 0;       /* pid of uade executable */

/* timeout value in seconds for songs that don't stop ever or play otherwise
   too long (timeout<=0 => unlimited play */
static int timeout = -1;

/* some stupid and/or bad state variables */
static int playingbit = 0;
static pthread_t decode_thread;
static int uade_frequency = 44100;
static int uade_nchannels = 2;
static int uade_bytespersample = 2;

static struct uade_msgstruct *uade_struct;   /* shared memory pointer */
static char mapfilename[UADE_PATH_MAX];           /* name of shared mem file */
static char uadename[UADE_PATH_MAX];              /* name of uade executable */

static const int MSG_PATH_MAX = sizeof(uade_struct->playername);

static char current_song_content_hash[33];
static char current_song_name_hash[33];
static tnode *songcontenttree = 0;      /* md5 content hash keys */
static tnode *songnametree = 0;         /* full name keys (including path) */
static char uade_content_db_filename[UADE_PATH_MAX];
static char uade_name_db_filename[UADE_PATH_MAX];
static int db_loaded = 0;
static time_t uade_last_db_save = 0;    /* time_t of database last saved */

/* this variable is used for recording playtimes to the uade database */
static int cumulative_playtime;
static int database_playtime;
static int database_songforce;

static int uade_is_operational = 0;      /* >0 if initialization is success */


struct uade_format_list {
  struct uade_format_list* next;
  char ext[16];
  char player[64];
  char playerfilepath[256];
};

static struct uade_format_list* uadeformats_head = 0;

static int uade_bigendian; /* initalized by uade_init() */
static AFormat uade_format; /* itialized by uade_init() */


/* this function is first called by xmms. returns pointer to plugin table */
InputPlugin* get_iplugin_info(void) {
  return &uade_ip;
}

/* xmms initializes uade by calling this function */
static void uade_init(void) {
  short betest;
  char *beptr = (char *) &betest;

  /* test if cpu is in big-endian mode */
  betest = 0x1234;
  uade_bigendian = (beptr[0] == 0x34) ? 0 : 1;
  uade_format = uade_bigendian ? FMT_S16_BE : FMT_S16_LE;

//  /* read configuration in configure.c module */
  uade_configread();

  /* create temporary name for mmap'ed shared memory file */
  if (!uade_get_temp_name(mapfilename, sizeof(mapfilename))) {
    return;
  }
  /* open and create blank file for shared memory file with size of "length" */
  if (!uade_init_mmap_file(mapfilename, sizeof(struct uade_msgstruct))) {
    return;
  }
  /* mmap shared mem file */
  uade_struct = uade_mmap_file(mapfilename, sizeof(struct uade_msgstruct));
  if (!uade_struct)
    return;

  /* initialize uade_msgstruct to zeros in shared memory */
  memset(uade_struct, 0, sizeof(struct uade_msgstruct));

  /* put xmms pid to uade_struct for uade executable */
  uade_struct->masterpid = getpid();

  /* expands names to uade data dir */
  if(!uade_get_path(uadename, UADE_PATH_UADE, sizeof(uadename)))
    return;
  if(!uade_get_path(uade_struct->scorename, UADE_PATH_SCORE, sizeof(uade_struct->scorename)))
    return;

  /* Set NTSC/PAL mode for uade */
  set_ntsc_pal(use_ntsc);

  /* mark that initialization seems to be a success */
  uade_is_operational = 1;
}

static int signal_slave(int signum) {
  if (slavepid) {
    return uade_send_signal(slavepid, signum);
  }
  return -1;
}

static void setup_databases(void) {
  char globaldbname[UADE_PATH_MAX];
  int ch, cg, nh;

  uade_mutex_lock(&uade_check_mutex);

  if (db_loaded) {
    goto err_db_loaded;
  }

  fprintf(stderr, "uade: loading databases\n");

  uade_content_db_filename[0] = uade_name_db_filename[0] = 0;
  sprintf(uade_content_db_filename,"%s/.uade/db-content", getenv("HOME"));
  sprintf(uade_name_db_filename,"%s/.uade/db-name", getenv("HOME"));
  sprintf(globaldbname, "%s/db-content", UADECONFIG_DATADIR);
  cg = 0;
  if (strcmp(globaldbname, uade_content_db_filename)) {
    cg = db_read_data(&songcontenttree, globaldbname);
  }
  ch = db_read_data(&songcontenttree, uade_content_db_filename);
  if (!ch) {
    if (cg) {
      fprintf(stderr, "uade: couldn't read user content database, but was able to read global content\n      database (%s)\n", globaldbname);
    } else {
      fprintf(stderr,"uade: couldn't read song content database: Following files were tried:\n");
      fprintf(stderr,"\t1. %s\n", uade_content_db_filename);
      fprintf(stderr,"\t2. %s\n", globaldbname);
    }
  }
  nh = db_read_data(&songnametree, uade_name_db_filename);

  uade_mutex_lock(&uade_db_mutex);
  uade_last_db_save = time(0); /* may return -1 ! */
  uade_mutex_unlock(&uade_db_mutex);

  /* important */
  db_loaded = 1;

  fprintf(stderr, "uade: databases loaded\n");

 err_db_loaded:
  uade_mutex_unlock(&uade_check_mutex);
}

static void save_db(void) {
  char dbfilename[UADE_PATH_MAX];
  char *dbdirname;
  /* try to create directory for database */
  strlcpy(dbfilename, uade_content_db_filename, sizeof(dbfilename));
  dbdirname = dirname(dbfilename);
  if (dbdirname) {
    if (mkdir(dbdirname, -1)) {
      if (errno != EEXIST) {
	perror("uade: could not create user database directory");
      }
    } else {
      fprintf(stderr, "uade: created user database directory (%s)\n", dbdirname);
    }
  }
  /* try to write to database */
  db_write_data(uade_content_db_filename, songcontenttree);
  db_write_data(uade_name_db_filename, songnametree);
}

static void clean_up(void) {
  if (mapfilename) {
    fprintf(stderr, "uade: removing tempfile %s (used for shared mem)\n", mapfilename);
    remove(mapfilename);
  }
  save_db();
}


/* xmms calls this function to check song */
static int is_our_file(char *filename) {
  int ours;
  const char *prefix = "file://";
  if(!uade_is_operational) {
    return FALSE;
  }
  if (strncasecmp(filename, prefix, strlen(prefix)) == 0)
    filename += strlen(prefix);
  ours = check_my_file(filename, 0, 0);
  if (ours) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
      return FALSE;
    }
    fclose(f);
  }
  return ours;
}

/* play_file() and is_our_file() call this function to check song */
static int check_my_file(char *filename, char *format, char *playername) {
  int result;
  uade_mutex_lock(&uade_check_mutex);
  result = cmf(filename, format, playername);
  uade_mutex_unlock(&uade_check_mutex);
  return result;
}


/* CRAP. REWRITE. */
static int cmf(char *filepath, char *format, char *playername) {
  FILE *formatsfile;
  char formatsname[UADE_PATH_MAX];
  char a[UADE_PATH_MAX], b[UADE_PATH_MAX];
  int x;
  int rval = FALSE;
  char pre[UADE_PATH_MAX], post[UADE_PATH_MAX];
  char *c;
  char playerdir[UADE_PATH_MAX];
  struct uade_format_list *node;

  pre[0] = 0;
  post[0] = 0;
  if (format)
    *format = 0;

  if (!uade_get_prefix(pre, filepath, sizeof(pre))) {
    return FALSE;
  }

  if (!uade_get_postfix(post, filepath, sizeof(post))) {
    return FALSE;
  }

  if (strcasecmp(pre, "mp3") == 0 || strcasecmp(post, "mp3") == 0 ||
     strcasecmp(pre, "wav") == 0 || strcasecmp(post, "wav") == 0 ) {
    return FALSE;
  }

  /* get a new postfix if the postfix is one of common compressor postfixes */
  if (strcasecmp(post, "gz") == 0 || strcasecmp(post, "bz2") == 0 ||
     strcasecmp(post, "pp") == 0 )                                  {
    /* separate postfix from filepath and copy result to post */
    c = strrchr(filepath, (int) '.');
    if ((c - filepath) >= ((int) sizeof(post))) {
      fprintf(stderr, "uade: too long basename (%s)\n", filepath);
      return FALSE;
    }
    strncpy(post, filepath, c - filepath);
    post[c - filepath] = 0;
    /* post must contain at least one dot, otherwise failure */
    c = strrchr(post, '.');
    if (!c) {
      return FALSE;
    }
    strlcpy(post, c + 1, sizeof(post));
  }

  /* conditionally scans our file and sets pre and post to right values.
     if the playername is non-zero the file scanned even if
     filemagick_check is zero. This happens when check_my_file() is
     called from play_file() rather than from is_our_file(). We don't always
     call scanfile() because it would slow down xmms playlist scanning. */
  if (filemagic_check || playername) {
    scanfile(filepath, pre, post);
  }

  if (!uadeformats_is_cached) {

    if (!uade_get_path(playerdir, UADE_PATH_PLAYERDIR, sizeof(playerdir))) {
      fprintf(stderr, "uade: cmf: couldn't get playerdir\n");
      return FALSE;
    }

    while (uadeformats_head) {
      node = uadeformats_head->next;
      free(uadeformats_head);
      uadeformats_head = node;
    }
    /* uadeformats_head is zero */

    node = 0;

    if (!uade_get_path(formatsname, UADE_PATH_FORMATSFILE, sizeof(formatsname))) {
      fprintf(stderr, "uade: cmf: couldn't get formatsfile\n");
      return FALSE;
    }
    formatsfile = fopen(formatsname, "r");
    if (!formatsfile) {
      fprintf(stderr, "uade: cmf: couldn't open formatsfile\n");
      return FALSE;
    }
    
    for (;;) {
      x = fscanf(formatsfile, "%s", a);
      if (x == 0 || x == EOF) break;
      
      // check if this is a comment line (this skips comment lines)
      if (a[0] == '#') {
	while (a[0] != '\n') {
	  x = fscanf(formatsfile, "%c", &a[0]);
	  if (x == 0 || x == EOF) break;
	}
	continue;
      }

      if (!strcasecmp(a, "formats")) {
	for (;;) {
	  x = fscanf(formatsfile, "%s", a);
	  if (x == 0 || x == EOF) break;
	  if (!strcasecmp("endformats", a)) break;
	  
	  /* check if this is a comment line (this skips comment lines) */
	  if (a[0] == '#') {
	    while (a[0] != '\n') {
	      x = fscanf(formatsfile, "%c", &a[0]);
	      if (x == 0 || x == EOF) break;
	    }
	    continue;
	  }
	  
	  x = fscanf(formatsfile, "%s", b);
	  if (x == 0 || x == EOF) break;
	  
	  if (node) {
	    node->next = malloc(sizeof(struct uade_format_list));
	    node = node->next;
	  } else {
	    uadeformats_head = malloc(sizeof(struct uade_format_list));
	    node = uadeformats_head;
	  }
	  node->next = 0;

	  if (!node) {
	    fprintf(stderr, "uade: out of memory (format cache allocation)\n");
	    goto done;
	  }

	  strlcpy(node->ext, a, sizeof(node->ext));
	  strlcpy(node->player, b, sizeof(node->player));
	  strlcpy(node->playerfilepath, playerdir, sizeof(node->playerfilepath));
	  strlcat(node->playerfilepath, b, sizeof(node->playerfilepath));
	}
	break;
      }
    }
  done:
    fclose(formatsfile);
    uadeformats_is_cached = 1;
  }

  node = uadeformats_head;

  if (!node) {
    fprintf(stderr, "uade: this shouldn't happen: cache was reported to exist but it didn't!\nREPORT THIS BUG!! MEMORY HAS BEEN POSSIBLY LEAKED...\n");
    return FALSE;
  }

  do {
    if (strncasecmp(node->ext, pre , sizeof(node->ext)) == 0 ||
	strncasecmp(node->ext, post, sizeof(node->ext)) == 0    ) {
      if (format) {
	strlcpy(format, node->player, MSG_PATH_MAX);
      }
      if (playername) {
	strlcpy(playername, node->playerfilepath, MSG_PATH_MAX);
      }
      return TRUE;
    }
    node = node->next;
  } while (node);

  return rval;
}


static void read_timeout_configuration(void) {
  /* read timeout value from configuration if necessary */
  if (use_timeout) {
    char timeoutstr[32];
    char *separator;
    strlcpy(timeoutstr, timeout_val, sizeof(timeoutstr));
    separator = strchr(timeoutstr, ':');
    if (separator) {
      /* given in minutes:seconds */
      *separator = 0;
      timeout = atoi(timeoutstr) * 60 + atoi(separator + 1);
    } else {
      /* given in seconds */
      timeout = atoi(timeoutstr);
    }
  }
}


static void waitforuaetoreact(void) {
  while (uade_struct->touaemsgtype) {
    /* check if uade is alive */
    if (signal_slave(0) < 0)
      return;
    xmms_usleep(1000);
  }
}


/* this function definately needs cleanup. HINT: it copies song data from
   a circular shared memory buffer to XMMS sound buffers. The copied data
   is originated from uade executable (see src/sd-sound.h)
*/
static void *play_loop(void *arg) {
  const int frames = 512;
  const int blocksize = 4 * frames;
  char sndbuf[blocksize];
  short *sm;
  int i, l;
  int bytesfree;
  int zerocounter = 0;
  int exceptioncounter;
  int silence_timeout, zerolimit;
  int datainbuffer;
  char filterspace[128];
  const int rbsize = sizeof(uade_struct->soundbuffer);

  arg = arg; /* no warning */
 
  memset(filterspace, 0, sizeof(filterspace));

  silence_timeout = atoi(silence_timeout_val);
  if (silence_timeout < 1) {
    silence_timeout = 1;
  }
  zerolimit = uade_frequency * uade_bytespersample * uade_nchannels * silence_timeout;

  while (playingbit) {

    bytesfree = uade_ip.output->buffer_free();

    if (bytesfree >= blocksize) {

      /* case 1: r=w: datainbuffer = 0      (special case of case 2)
	 case 2: r<w: datainbuffer = w - r
	 case 3: r>w: datainbuffer = w + rbsize - r
      */
      if (uade_struct->sbuf_readoffset <= uade_struct->sbuf_writeoffset) {
	/* case 1 and 2 */
	datainbuffer = uade_struct->sbuf_writeoffset - uade_struct->sbuf_readoffset;
      } else {
	/* case 3 */
	datainbuffer = uade_struct->sbuf_writeoffset + rbsize - uade_struct->sbuf_readoffset;
      }

      if (datainbuffer < blocksize) {
	xmms_usleep(10000);
	continue;
      }
      
      if ((uade_struct->sbuf_readoffset + blocksize) > rbsize) {
	int firstsize = rbsize - uade_struct->sbuf_readoffset;
	memcpy(sndbuf, uade_struct->soundbuffer+uade_struct->sbuf_readoffset, firstsize);
	memcpy(sndbuf+firstsize, uade_struct->soundbuffer, blocksize-firstsize);
      } else {
	memcpy(sndbuf, uade_struct->soundbuffer + uade_struct->sbuf_readoffset, blocksize);
      }
      uade_struct->sbuf_readoffset = (uade_struct->sbuf_readoffset+blocksize) % rbsize;

      if (do_lp_filter) {
	uade_effect_filter((short *) sndbuf, frames, 1, lp_filter, filterspace, sizeof(filterspace));
      }

      if (do_mixing) {
	uade_effect_pan((short *) sndbuf, frames, 2, mixing_parameter);
      }

      if (do_volume_gain) {
	uade_effect_volume_gain((short *) sndbuf, frames, 2, volume_gain_parameter);
      }

      uade_ip.add_vis_pcm(uade_ip.output->written_time(), uade_format, 2, blocksize, sndbuf);
      uade_ip.output->write_audio((void*) sndbuf, blocksize);

      /* check if there's a long silence in the tune => end the song..
	 the algorithm is:
	 - see if all samples are less than 1% of maximum amplitude
	   - if so, then increase zerocounter
	   - if not so, then see if at most 1% of samples are an exception
	     to the amplitude rule.
	       - if there are more than 1% of exceptions => set zerocounter to
	         0, and break the loop
	       - if there are less than 1% of exceptions => ignore all the
	         non-zeros: just add zerocounter
	 - when zerocounter reaches a high value enough the song will be
	   stopped (equivalent number of 30 seconds of samples)
      */
      sm = (short *) sndbuf;
      exceptioncounter = 0;
      for(i=0; i < (2 * frames); i++) {
	l = sm[i] > 0 ? sm[i] : -sm[i];
	if(l < (32767*1/100)) {
	  zerocounter += 2;
	} else {
	  exceptioncounter++;
	  if (exceptioncounter > (2 * frames/100)) {
	    zerocounter = 0;
	    break;
	  }
	}
      }
      if(zerocounter >= zerolimit) {
	zerocounter = 0;
	uade_struct->song_end = 1;
	fprintf(stderr, "uade: song end (xmms plugin doesn't like silence)\n");
      }
    } else {
      xmms_usleep(10000);
    }
  }

  pthread_exit(0);
  return 0;
}


static void play_file(char *filename) {
  char format[1024], text[1024];
  int playtime;
  int *songdata;
  char *tempname;
  int have_name_hash;
  const char *prefix = "file://";

  if (strncasecmp(filename, prefix, strlen(prefix)) == 0)
    filename += strlen(prefix);

  if (playingbit) {
    fprintf(stderr, "uade: A serious bug has been detected:\n");
    fprintf(stderr, "playingbit != 0 when play_file was called\n");
    playingbit = 0;
  }

  if (!slavepid) {
    int slept = 0;
    char *newargv[] = {uadename, "--xmms-slave", mapfilename, 0};
    fprintf(stderr, "uade: fork-exec(%s)\n", uadename);
    slavepid = uade_fork_exec(newargv);
    while (uade_struct->uade_inited_boolean == 0) {
      if (slept >= 5000000) {
	fprintf(stderr, "uade: xmms plugin was not able to fork-exec uade in 5 seconds. still trying.\n");
	slept = 0;
      }
      xmms_usleep(100000);
      slept += 100000;
    }
  } else if (slavepid < 0) {
    uade_struct->song_end = 1;
    fprintf(stderr, "uade: xmms plugin was not able to fork-exec uade\n");
    return;
  }

  if (!uade_ip.output->open_audio(uade_format, uade_frequency,
				  uade_nchannels)) {
    fprintf(stderr, "uade: error: can't allocate audio from xmms\n");
    return;
  }

  /* get basename of filename into uade_song_basename and tempname */
  tempname = uade_strdup_basename(filename);
  uade_song_basename = tempname ? strdup(tempname) : 0;
  uade_song_full_name = strdup(filename);
  if (!uade_song_basename || !uade_song_full_name) {
    fprintf(stderr, "uade: error. no memory for names. not playing.\n");
    goto err_no_mem;
  }

  set_ntsc_pal(use_ntsc);
  set_song_end_possible(use_songend);
  uade_struct->use_filter = use_filter;

  /* invalidate format cache for each played song (i hope this doesn't leak
     memory) .. only useful if optimize_filescan is on */
  uadeformats_is_cached = 0;

  (void) check_my_file (filename, format, uade_struct->playername);

  if (!strcasecmp(format, "custom")) {
    /* custom song: filename -> playername, and no modulename */
    strlcpy(uade_struct->playername, filename, sizeof(uade_struct->playername));
    uade_struct->modulename[0] = 0;
  } else {
    strlcpy(uade_struct->modulename, filename, sizeof(uade_struct->modulename));
  }

  uade_get_path(uade_struct->scorename, UADE_PATH_SCORE, sizeof(uade_struct->scorename));

  /* set default timeout (unlimited play time) */
  timeout = -1;
  read_timeout_configuration();

  /* load song length database, if not already loaded */
  if (!db_loaded) {
    setup_databases();
  }

  /* database auto saving test */
  if (db_loaded && auto_db_saves) {
    time_t db_time = time(0);
    if (uade_last_db_save > 0 && db_time > 0) {
      /* auto save every hour */
      if ((db_time - uade_last_db_save) > auto_db_save_interval) {
	fprintf(stderr, "uade: auto saving song content database\n");
	save_db();
	uade_mutex_lock(&uade_db_mutex);
	uade_last_db_save = time(0); /* may return -1 ! */
	uade_mutex_unlock(&uade_db_mutex);
      }
    }
  }

  /* estimate play time based on content and name hashes. cases:
     - if only name hash exists, get estimated playtime from songnametree
     - if only content hash exists, get estimated playtime from songconttree
     - if both name and content hashes exist, get estimated playtime from
       songcontenttree, and replay old playtime in songnametree with entry
       from songcontenttree. So content tree is considered more important.
     - if no hash exists, playtime = 0 (indefinite).
  */
  have_name_hash = db_get_name_hash(current_song_name_hash, filename);

  if (filechecksum(current_song_content_hash, filename)) {
    songdata = db_check_song(songcontenttree, current_song_content_hash);
  } else {
    songdata = 0;
  }

  if (songdata) {
    database_playtime = songdata[0];
    database_songforce = songdata[1];
    if (have_name_hash) {
      db_add_song_data(&songnametree, current_song_name_hash, database_playtime, 0);
    }
  } else {
    if (have_name_hash) {
      songdata = db_check_song(songnametree, current_song_name_hash);
    } else {
      songdata = 0;
    }
    if (songdata) {
      database_playtime = songdata[0];
      database_songforce = songdata[1];
    } else {
      database_playtime = 0;
      database_songforce = 0;
    }
  }
  if (database_playtime) {
    playtime = database_playtime;
  } else {
    /* calculate song playtime according to timeout value */
    playtime = (timeout == -1) ? 0 : (1000 * timeout);
  }
  cumulative_playtime = 0;

  /* Tell song len to XMMS, set XMMS window scroller text */
  snprintf(text, sizeof(text), "%s [%s]", tempname, format);
  uade_ip.set_info_text(text);
  uade_ip.set_info(text, playtime, 8 * 4 * uade_frequency, uade_frequency, uade_nchannels);

  free(tempname);

  uade_struct->force_by_default = force_by_default;
  uade_struct->set_subsong = 0;
  uade_struct->subsong = 0;
  uade_struct->dontwritebit = 0;
  uade_struct->song_end = 0;

  uade_struct->plugin_pause_boolean = 0;
  uade_struct->sbuf_writeoffset = 0;
  uade_struct->sbuf_readoffset = 0;

  /* this syncs playername, modulename and scorename with uade's internal
     structures */
  uade_struct->loadnewsongboolean = 1;
  uade_struct->touaemsgtype = UADE_PLAYERNAME;
  if (signal_slave(0) >= 0) {
    signal_slave(SIGHUP);
  }

  playingbit = 1; /* this must be set non-zero before play_loop() starts */

  if (pthread_create(&decode_thread, 0, play_loop, 0)) {
    fprintf(stderr, "uade: can't create play_loop() thread\n");

    playingbit = 0;

    goto err_no_mem;
  }

  return;

 err_no_mem:
  free(uade_song_basename);
  free(uade_song_full_name);
  uade_song_basename = 0;
  uade_song_full_name = 0;
  /* close audio that was opened */
  uade_ip.output->close_audio();
  return;
}

static void stop(void) {

  if (!uade_song_basename) {
    /* song was not loaded */
    return;
  }

  /* this will make play_loop thread terminate itself in time */
  playingbit = 0;

  /* forbid uade to write into shared memory */
  uade_struct->dontwritebit = 1;
  uade_struct->plugin_pause_boolean = 0;
  uade_struct->loadnewsongboolean = 0;

  /* check if uade is alive */
  if (signal_slave(0) >= 0) {
    uade_struct->touaemsgtype = UADE_REBOOT;
    signal_slave(SIGHUP);
    waitforuaetoreact();
  }

  pthread_join(decode_thread, 0);

  uade_ip.output->close_audio();

  free(uade_song_basename);
  free(uade_song_full_name);
  uade_song_basename = 0;
  uade_song_full_name = 0;

  current_song_content_hash[0] = 0;
  current_song_name_hash[0] = 0;

  /* at last removing windows belonging to us */
//  uade_close_win();
}


int is_paused(void) {
  return uade_struct->plugin_pause_boolean;
}

static void set_paused(int paused) {
  uade_struct->plugin_pause_boolean = paused ? 1 : 0;
}

/* function that xmms calls when pausing or unpausing */
static void uade_pause(short paused) {

  if (!uade_song_basename) {
    /* song was not laoded */
    return;
  }

  set_paused((int) paused);
  uade_ip.output->pause(paused);
}

void seek(int newsubsong, char *reason) {
  char pr[1024];

  if (!uade_song_basename) {
    /* song was not laoded */
    return;
  }

  if (reason) {
    sprintf(pr, "(%s)", reason);
  } else {
    pr[0] = 0;
  }

  fprintf(stderr, "uade: seeking to subsong %d %s\n", newsubsong, pr);
  uade_struct->touaemsgtype = UADE_SETSUBSONG;
  uade_struct->set_subsong = 1;
  uade_struct->subsong = newsubsong;
  uade_struct->song_end = 0;
  /* if subsong is changed manually cumulative_playtime is rendered invalid */
  cumulative_playtime = -1;
  signal_slave(SIGHUP);
  waitforuaetoreact();
  uade_ip.output->flush(0);
}


/* get_time() is a rather complicated function. it handles timeout and song
   end detection. if the code doesn't make any sense, don't worry. it doesn't
   make any sense to me either */

static int get_time(void) {
  int currenttime = uade_ip.output->output_time();
  static int endinprogress = 0;
  static int endtime;

  /* First time songend is noticed we do following:
     1. mark that end is in progress (static int endinprogress = 1)
     2. save the output plugin written time for the purpose of determining
        later when to stop playing.. call this saved value endtime.
     3. check if output time is at least endtime
        true  => end song by setting endinprogess=0 and returning -1
        false => return playtime normally

     Second time songend is noticed we only execute step 3.

     If songend is not detected, we set endinprogess to 0 to be certain
     that when song end is noticed the first time previous procedure
     goes smoothly..
  */

  /* if play_time() was not succesful, make xmms end this song */
  if (!uade_song_basename) {
    return -1; /* return xmms end code */
  }

  if (uade_struct->song_end) {
    if (endinprogress == 0) {
      endtime = uade_ip.output->written_time();
      endinprogress = 1;
      /* if a subsong was changed manually, cumulative_playtime became -1
	 and thus it is invalid. add cumulative_playtime only if it's valid */
      if (cumulative_playtime >= 0) {
	/* add cumulative_playtime to find out total duration of all subsongs*/
	cumulative_playtime += endtime;
      }
    }
    if (endinprogress) {
      if (endtime <= uade_ip.output->output_time()) {
	endinprogress = 0;
	if (next_subsong_on_song_end == TRUE && get_max_subsong() != 0) {
	  int cur = get_curr_subsong();
	  if (cur < get_max_subsong()) {
	    int backup = cumulative_playtime;
	    seek(cur + 1, 0);
	    cumulative_playtime = backup;
	    return 0;
	  }
	}

	/* add cumulative time to database only if it's non-negative */
	if (cumulative_playtime >= 0) {
	  db_add_song_data(&songcontenttree, current_song_content_hash, cumulative_playtime, 0);
	  db_add_song_data(&songnametree, current_song_name_hash, cumulative_playtime, 0);
	}
	return -1; /* return end code for smms */
      }
    }
  } else {
    endinprogress = 0;
    if (timeout > 0) {
      int outputtime = uade_ip.output->output_time();

      if ((outputtime/1000) >= timeout) {

	if (next_subsong_on_timeout && get_max_subsong() != 0) {
	  int cur = get_curr_subsong();
	  if (cur < get_max_subsong()) {
	    seek(cur + 1, "timeout");
	    return 0;
	  }
	}

	if (!database_songforce) {
	  if (outputtime >= database_playtime) {
	    return -1; /* return end code for xmms */
	  }
	}
      }
      if (database_songforce) {
	if (outputtime >= database_playtime) {
	  return -1; /* return end code for xmms */
	}
      }
    }
  }
  return currenttime;
}


int get_curr_subsong(void) {
  return uade_struct->subsong;
}


int get_min_subsong(void) {
  return uade_struct->min_subsong;
}


int get_max_subsong(void) {
  return uade_struct->max_subsong;
}


static char *default_modulename = "<no name>";

char *get_modulename(void) {
  if(uade_struct->score_modulename[0]!='\0') {
    return uade_struct->score_modulename;
  } else if (uade_song_basename) {
    return uade_song_basename;
  }
  return default_modulename;
}

static char *default_playername = "Custom";

char *get_playername(void) {
  if (uade_struct->score_playername[0] == 0) {
    /* Assumptions are always bad, I know ;) */
    return default_playername;
  }
  return uade_struct->score_playername;
}

char *get_formatname(void) {
    return uade_struct->score_formatname;
}

char *get_modulefilename(void) {
    return uade_struct->modulename;
}

char *get_playerfilename(void) {
  return uade_struct->playername;
}


static void get_song_info(char *filename, char **title, int *length) {
  char *temp;
  char md5namehash[33];
  const char *prefix = "file://";

  if (strncasecmp(filename, prefix, strlen(prefix)) == 0)
    filename += strlen(prefix);

  temp = strrchr(filename, (int) '/');
  if (temp) {
    temp++;
  } else {
    temp = filename;
  }
  *title = g_strdup(temp);

  /* no length for song defined by default */
  *length = -1;

  if (!db_loaded) {
    setup_databases();
  }

  /* check name database for song length */
  if (db_get_name_hash(md5namehash, filename)) {
    int songlength;
    int *songdata = db_check_song(songnametree, md5namehash);
    songlength = songdata ? songdata[0] : 0;
    if (songlength) {
      *length = songlength;
    }
  }
}



void set_ntsc_pal(int is_ntsc) {
  use_ntsc = is_ntsc;
  if (uade_struct) {
    uade_struct->ntscbit = use_ntsc == FALSE ? 0:1;
    if (uade_song_basename) {
      /* check if uade is alive */
      if(signal_slave(0) >= 0) {
	uade_struct->touaemsgtype=UADE_NTSC;
	signal_slave(SIGHUP);
	waitforuaetoreact();
      }
    }
  }
}


void set_song_end_possible(gboolean use_songend) {
  if (uade_struct) {
    uade_struct->songendpossible = use_songend ? -1:0;
    if (uade_song_basename) {
      if (signal_slave(0) >= 0) {
	uade_struct->touaemsgtype = UADE_SONG_END;
	signal_slave(SIGHUP);
	waitforuaetoreact();
      }
    }
  }
}

struct uadedbentry {
  char hash[33];
  int len;
  int force;
};

static int db_read_data(tnode **root, const char *filename) {
  FILE *dbFile;
  char md5[64];
  int x;
  char lbuf[16];
  int length;
  int force;
  char line[256];
  int r;
  int maxentries = 0;
  int nentries = 0;
  struct uadedbentry *entries = 0;

  maxentries = 64;
  if(!(entries = malloc(maxentries * sizeof(struct uadedbentry)))) {
    fprintf(stderr, "uade: db_read_data: no memory for malloc\n");
    return 0;
  }

  dbFile = fopen(filename, "r");
  if(!dbFile) {
    return 0;
  }

  while (1) {
    if (!fgets(line, sizeof(line), dbFile)) {
      break;
    }
    force = 0;
    if (strchr(line, (int) '+')) {
      force = 1;
    }
    x = sscanf(line, "%32s %d\n", md5, (int*) lbuf);
    if (x == 2) {
      length = *((int *) lbuf);
      /* ignore songs with duration < 1000 ms */
      if (length >= 1000) {
	if (strlen(md5) == 32) {
	  /* add hash to the tree only if it doesn't exist yet */
	  if (!db_check_song(*root, md5)) {
	    if (nentries >= maxentries) {
	      maxentries *= 2;
	      entries = realloc(entries,maxentries*sizeof(struct uadedbentry));
	      if (!entries) {
		fprintf(stderr, "uade: db_read_data: no memory for realloc\n");
		break;
	      }
	    }
	    strcpy(entries[nentries].hash, md5);
	    entries[nentries].len = length;
	    entries[nentries].force = force;
	    nentries++;
	  }
	} else {
	  fprintf(stderr, "uade: db_read_data: illegal data %s %d\n", md5, length);
	}
      }
    } else {
      break;
    }
  }
  fclose(dbFile);

  /* add entries into hash tree in random order (random order for performace
     reasons) */
  for (x = nentries; x > 0; x--) {
    r = random() % x;
    db_add_song_data(root, entries[r].hash, entries[r].len, entries[r].force);
    if (r != (x - 1))
      entries[r] = entries[x - 1];
  }

  free(entries);
  return 1;
}


static void db_write_data(const char *filename, tnode *root) {
  if (root) {
    FILE *dbFile;
    uade_mutex_lock(&uade_db_mutex);
    dbFile = fopen(filename, "w");
    if (!dbFile) {
      fprintf(stderr, "uade: couldn't open database %s for writing\n", filename);
      uade_mutex_unlock(&uade_db_mutex);
      return;
    }
    uade_last_db_save = time(0);
    btree_traverse(root, uadedbwriteroutine, (void *) dbFile);
    fclose(dbFile);
    uade_mutex_unlock(&uade_db_mutex);
  }
}


static int uadedbwriteroutine(tnode *node, void *arg) {
  int length;
  int *songdata = (int *) node->data;
  char fstring[2];
  length = songdata[0];
  strcpy(fstring, songdata[1] ? "+" : ""); /* + if force, empty if no force */
  if (strlen(node->key.key) == 32) {
    fprintf((FILE *) arg, "%s %s%d\n", node->key.key, fstring, length);
  } else {
    fprintf(stderr,"uade: db_write_data: illegal data (%s %d)\n", node->key.key, length);
  }
  return 1;
}

static int db_get_name_hash(char *hash, const char *filename) {
  char md5[33];
  int filesize;
  struct stat st;
  if (!hash)
    return 0;
  hash[0] = 0;
  if (stat(filename, &st))
    return 0;
  filesize = st.st_size;
  strchecksum(md5, filename);
  if (strlen(md5) != 32) {
    fprintf(stderr, "uade: db_get_name_hash: name md5sum has wrong size! (%s)\n", filename);
    return 0;
  }
  sprintf(hash, "%.8x", filesize);
  memcpy(&hash[8], md5, 24);
  hash[32] = 0;
  return 1;
}


static int *db_check_song(tnode *root, const char *hash) {
  tnode *node;
  tnodekey key;
  int *dblengthptr;
  strlcpy(key.key, hash, sizeof(key.key));
  if (strlen(key.key) != 32) {
    fprintf (stderr, "uade: db_check_song(): illegal hash length\n");
    return 0;
  }
  dblengthptr = 0;
  uade_mutex_lock(&uade_db_mutex);
  /* no problem if root == 0 */
  node = btree_addnode(root, 0, &key, 0);
  if (node) {
    dblengthptr = (int *) node->data;
    if (strlen(node->key.key) != 32) {
      fprintf(stderr,"uade: db_check_song: BUG %s %d\n", node->key.key, *dblengthptr);
    }
  }
  uade_mutex_unlock(&uade_db_mutex);
  return dblengthptr;
}


/* if song data already exists in the database the old length is replaced by
   the new length */
static void db_add_song_data(tnode **root, char *hash, int length, int force) {
  tnodekey key;
  int *newdata;
  if (hash) {
    if (strlen(hash) != 32) {
      fprintf(stderr, "uade: db_add_song_data: BUG (report this):\n");
      fprintf(stderr, "uade: %s %s %d\n", uade_song_basename ? uade_song_basename : "null", hash, length);
      return;
    }
    strlcpy(key.key, hash, sizeof(key.key));

    newdata = malloc(2 * sizeof(int));
    if (!newdata) {
      fprintf(stderr, "uade: db_add_song_data: out of memory\n");
      return;
    }
    newdata[0] = length;
    newdata[1] = force;

    uade_mutex_lock(&uade_db_mutex);
    if (*root) {
      /* if the key already exists the old data is replaced with new data */
      btree_addnode(*root, newdata, &key, 1);
    } else {
      /* create tree */
      *root = btree_createtree(newdata, &key);
    }
    uade_mutex_unlock(&uade_db_mutex);
  }
}


static void uade_mutex_lock(pthread_mutex_t *m) {
  pthread_mutex_lock(m);
}


static void uade_mutex_unlock(pthread_mutex_t *m) {
  pthread_mutex_unlock(m);
}
