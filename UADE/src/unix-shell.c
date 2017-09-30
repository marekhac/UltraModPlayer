/* Copyright 2003 Heikki Orsila <heikki.orsila@iki.fi>
*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <glob.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include "debug.h"

#include "../config.h"

#include "uade-os.h"
#include "unix-shell.h"
#include "playlist.h"

#include "../decrunch/decrunch.h"
#include "../plugindir/amifilemagic.h"

#include "../osdep/strl.c"

static struct uade_slave *me = 0;

static struct playlist playlist;

static int one_subsong = 0;

static int playerforced = 0;

/* catch CTRL-C and cause REBOOT */
static void us_sigint_handler(int signum)
{
  signum = signum;
  if (!uade_debug) {
    int ltime;
    ltime = uade_timer_poll();
    if (uade_reboot || (ltime > 0 && ltime < 100)) {
      fprintf(stderr,"uade: exit forced with ctrl-c\n");
      uade_exit(0);
    }
    uade_reboot = 1;
  } else {
    activate_debugger();
  }
}

static int us_setup(struct uade_song *song, int argc, char **argv)
{
  int no_more_opts, i, j, is_recursive = 0;

  /* create CTRL-C break for cmdline tool */
  if (!uade_create_signalhandler(us_sigint_handler, UADE_SIGINT)) {
    fprintf(stderr, "uade: failed to setup ctrl-c handler. it won't work.\n");
  }

  if (!playlist_init(&playlist)) {
    fprintf(stderr, "uade: unix-shell: failed to initialize playlist.\n");
  }

  me->timeout = 10 * 60;    /* 10 minutes (I know, a stupid value) */
  me->silence_timeout = 20; /* default silence timeout is 20 seconds */

  no_more_opts = 0;

  for (i = 1; i < argc;) {

    j = i;

    /* if argv[i] begins with '-', see if it is a switch. If it is not, i == j
       holds after tests, we queue argv[i] as a song into playlist */

    if (argv[i][0] == '-') {

      /* check for playername parameter */
      if (!strncmp(argv[i], "-player", 3) || !strcmp(argv[i], "-P")) {
	if ((i+1) < argc) {
	  strlcpy(song->playername, argv[i+1], sizeof(song->playername));
	  playerforced = 1;
	  i += 2;
	} else {
	  fprintf(stderr, "Player name missing for -P parameter\n\n");
	  uade_print_help(1);
	  uade_exit(-1);
	}

	/* check for modulename parameter */
      } else if (!strncmp(argv[i], "-module", 4) || !strcmp(argv[i], "-M")) {
	if ((i+1) < argc) {
	  if (!playlist_add(&playlist, argv[i + 1], 0)) {
	    fprintf(stderr, "uade: couldn't add %s into playlist\n", argv[i + 1]);
	  }
	  i += 2;
	} else {
	  fprintf(stderr, "Song name missing for -M parameter\n\n");
	  uade_print_help(1);
	  uade_exit(-1);
	}

	/* check for scorename parameter */
      } else if (!strcmp(argv[i], "-score") || !strcmp(argv[i], "-S")) {
	if ((i+1) < argc) {
	  strlcpy(song->scorename, argv[i+1], sizeof(song->scorename));
	} else {
	  fprintf(stderr, "missing parameter for -S\n\n");
	  uade_print_help(1);
	  uade_exit(-1);
	}
	i += 2;

      } else if (!strcmp(argv[i], "-sub") || !strcmp(argv[i], "-s")) {
	if ((i+1) >= argc) {
	  fprintf(stderr, "parameter missing for %s\n", argv[i]);
	  uade_print_help(1);
	  uade_exit(-1);
	}
	song->subsong = atoi(argv[i+1]);
	if ((*argv[i+1] != '+') && (*argv[i+1] != '-')) { 
	  song->set_subsong = 1;
	} else {
	  song->set_subsong = 2;
	  fprintf(stderr, "%s parameter parsed, setting subsong %d+min_subsong\n", argv[i], song->subsong);
	}
	i += 2;

      } else if (!strcmp(argv[i], "-repeat") || !strcmp(argv[i], "-rp")) {
	playlist_repeat(&playlist);
	i++;

      } else if (!strcmp(argv[i], "-one")) {
	one_subsong = 1;
	i++;

      } else if (!strcmp(argv[i], "-timeout") || !strcmp(argv[i], "-t")) {
	if ((i+1) >= argc) {
	  fprintf(stderr, "timeout parameter missing\n");
	  uade_print_help(1);
	  uade_exit(-1);
	}
	me->timeout = atoi(argv[i+1]);
	if (me->timeout <= 0) {
	  fprintf(stderr, "uade: illegal timeout parameter %s\n", argv[i+1]);
	  uade_exit(-1);
	}
	i += 2;

      } else if (!strcmp(argv[i], "-varpid")) {
	uade_create_var_pid();
	i++;

      } else if (!strcmp(argv[i], "-st")) {
	if ((i+1) < argc) {
	  me->subsong_timeout = atoi(argv[i+1]);
	} else {
	  fprintf(stderr, "per subsong timeout parameter missing\n");
	  uade_print_help(1);
	  uade_exit(-1);
	}
	i += 2;

      } else if (!strncmp(argv[i], "-device", 4)) {
	if ((i+1) < argc) {
	  if (!(uade_unix_sound_device = strdup(argv[i + 1]))) {
	    fprintf (stderr, "no memory for unix sound device name\n");
	    uade_exit(-1);
	  }
	} else {
	  fprintf(stderr, "sound device filename missing with %s\n", argv[i]);
	  uade_exit(-1);
	}
	i += 2;
	
      } else if (!strcmp(argv[i], "-sit")) {
	if ((i+1) < argc) {
	  me->silence_timeout = atoi(argv[i+1]);
	} else {
	  fprintf(stderr, "silence timeout parameter missing\n");
	  uade_print_help(1);
	  uade_exit(-1);
	}
	i += 2;

      } else if (!strcmp(argv[i], "-interactive") || !strcmp(argv[i], "-i")) {
	fprintf(stderr, "unix shell interactive mode!!!!!!!!!!!!!\n");
	uade_exit(-1);
	i++;

      } else if (!strcmp(argv[i], "-rand") || !strcmp(argv[i], "-r")) {
	playlist_random(&playlist, 1);
	i++;

      } else if (!strcmp(argv[i], "-recursive") || !strcmp(argv[i], "-R")) {
	is_recursive = 1;
	i++;

      } else if (!strcmp(argv[i], "--")) {
	no_more_opts = 1;
	i++;
      }
    }

    /* i == j implies argv[i] didn't match any switch, so we assume it's
       a song to be queued into playlist */
    if (i == j || no_more_opts) {
      if (!playlist_add(&playlist, argv[i], is_recursive)) {
	fprintf(stderr, "uade: couldn't add %s into playlist\n", argv[i]);
      }
      i++;
    }
  }

  /* if songlist is empty, use given playername (-P) if exists */
  if (playlist_empty(&playlist)) {
    /* don't exit if there is a player set, it could be a custom song given
       with -P */
    if (song->playername[0] == 0) {
      uade_print_help(2);
      uade_exit(-1);
    }
    /* queue zero string as module, only player is used */
    if (!playlist_add(&playlist, "", 0)) {
      fprintf(stderr, "uade: couldn't add to playlist\n");
    }
  }

  return 1;
}


/* CRAP. REWRITE */
static void check_name_extension(char *playername, char *modname, int maxl)
{
  FILE *formatsfile;
  FILE *songfile;
  struct stat st;
   
  int filesize = 5122;
  int realfilesize = 0;
  unsigned char buf[5122] = "";
  int status;
  char formatsfilename[UADE_PATH_MAX];
  char playerdir[UADE_PATH_MAX];
  int x;
  char a[UADE_PATH_MAX], b[UADE_PATH_MAX], pre[UADE_PATH_MAX], post[UADE_PATH_MAX];

  *playername = 0;

  pre[0] = 0;
  post[0] = 0;
  if (!uade_get_prefix(pre, modname, sizeof(pre))) {
    return;
  }
  if (!uade_get_postfix(post, modname, sizeof(post))) {
    return;
  }

  if (!uade_get_path(formatsfilename, UADE_PATH_FORMATSFILE, sizeof(formatsfilename))) {
    return;
  }
  formatsfile = fopen(formatsfilename, "rb");
  if (!formatsfile) {
    return;
  }

  songfile = fopen (modname, "rb");
  if (!songfile) {
    fprintf (stderr, "uade: can not open file %s\n", modname);
    goto closeffile;
  }
  if ((status = decrunch ( &songfile, modname ) < 0 )) {
    fprintf (stderr, "decrunching error...\n");
    fclose (songfile);
    goto closeffile;
  }

  fstat (fileno(songfile),&st);
  realfilesize = st.st_size;

  fread  (buf, 1, 5122, songfile);
  fclose (songfile);
  filemagic (buf, pre, post, realfilesize);

  /* get player directory */
  uade_get_path(playerdir, UADE_PATH_PLAYERDIR, sizeof(playerdir));

  for (;;) {
    x = fscanf(formatsfile, "%s", a);
    if (x==0 || x==EOF) break;

    /* check if this is a comment line (this skips comment lines) */
    if (a[0]=='#') {
      while (a[0]!='\n') {
	x = fscanf(formatsfile,"%c",&a[0]);
	if (x == 0 || x==EOF) break;
      }
      continue;
    }

    if (strcasecmp(a,"formats") == 0) {
      for (;;) {
	x = fscanf(formatsfile, "%s", a);
	if (x == 0 || x == EOF) break;
	if (strcasecmp("endformats", a) == 0) break;

	/* check if this is a comment line (this skips comment lines) */
	if (a[0] == '#') {
	  while (a[0] != '\n') {
	    x = fscanf(formatsfile, "%c", &a[0]);
	    if (x == 0 || x == EOF) break;
	  }
	  continue;
	}
	
	x = fscanf(formatsfile,"%s",b);
	if (x == 0 || x == EOF) break;

	if (strcasecmp(pre, a) == 0 || strcasecmp(post, a) == 0) {
	  if (strcmp(b, "custom") == 0) {
	    strlcpy(playername, modname, maxl);
	    modname[0] = 0;
	  } else {
	    /* Find out player directory path */
	    strlcpy(playername, playerdir, maxl);
	    strlcat(playername, b, maxl);
	  }
	  goto closeffile;
	}
      }
      break;
    }
  }
 closeffile:
  fclose(formatsfile);
}


static int us_get_next(struct uade_song *song)
{
  song->cur_subsong = song->min_subsong = song->max_subsong = 0;

  if (playerforced) {

    song->modulename[0] = 0;

    if (!playlist_get_next(song->modulename, sizeof(song->modulename), &playlist)) {
      return 0;
    }

  } else {

    song->playername[0] = 0;

    while (song->playername[0] == 0) {

      song->modulename[0] = 0;

      if (!playlist_get_next(song->modulename, sizeof(song->modulename), &playlist)) {
	return 0;
      }

      /* ugly hack. clear song->modulename[0] if song is a custom! */
      check_name_extension(song->playername, song->modulename, sizeof(song->playername));
      if (song->playername[0] == 0) {
	fprintf(stderr,"uade: file %s: unknown format\n", song->modulename);
      }
    }
  }
  return 1;
}


static int us_list_empty(void)
{
  return playlist_empty(&playlist);
}

static void us_skip_to_next_song(void)
{
}

static void us_song_end(struct uade_song *song, char *reason, int kill_it)
{
  /* if kill_it is zero, try to switch to next subsong. fatal errors
     will set kill_it non-zero and hence other subsongs won't be played */
  if (!kill_it) {
    if (song->max_subsong && !one_subsong) {
      if (song->cur_subsong < song->max_subsong) {
	song->cur_subsong++;
	uade_change_subsong(song->cur_subsong);
	return;
      }
    }
  }
  uade_reboot = 1;
}

static void us_subsinfo(struct uade_song *song, int mins, int maxs, int curs)
{
  fprintf(stderr, "uade: subsong info: minimum: %d maximum: %d current: %d\n", mins, maxs, curs);
  song->min_subsong = mins;
  song->max_subsong = maxs;
  song->cur_subsong = curs;
}

static void us_got_playername(char *playername)
{
  fprintf(stderr,"uade: playername: %s\n", playername);
}

static void us_got_modulename(char *modulename)
{
  fprintf(stderr,"uade: modulename: %s\n", modulename);
}

static void us_got_formatname(char *formatname)
{
  fprintf(stderr,"uade: formatname: %s\n", formatname);
}

void us_functions(struct uade_slave *slave)
{
  me = slave;
  me->setup = us_setup;
  me->get_next = us_get_next;
  me->list_empty = us_list_empty;
  me->skip_to_next_song = us_skip_to_next_song;
  me->song_end = us_song_end;
  me->subsinfo = us_subsinfo;
  me->got_playername = us_got_playername;
  me->got_modulename = us_got_modulename;
  me->got_formatname = us_got_formatname;
}
