/* Copyright 2003-2004 Heikki Orsila <heikki.orsila@iki.fi>

TODO:
 - ls, command launch support, pwd, ...
 - mono/stereo (pan)
 - prompt delay (so that it doesn't get mixed with information) (perhaps change
   the whole event system)
 - play -> add
 - real play and stop
 - a real playlist system
 - add recursive and non-recursive
 - move to using ncurses
 - volume control
*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <glob.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include "debug.h"

#include "../config.h"

#include "uade-os.h"
#include "playlist.h"
#include "../decrunch/decrunch.h"
#include "../plugindir/amifilemagic.h"
#include "../osdep/strl.c"

#include "unix-shell-int.h"


static int usi_get_next(struct uade_song *song);


#define INTERACTION_EOL 1
#define INTERACTION_TAB 2

static int tab_state = 0;
static int tab_pos = 0;
static int prompt_shown = 0;
static int cmd_number = 1;
static int cmd_buf_size = 0;
static char cmd_buf[4096];
static char auto_pattern[UADE_PATH_MAX];

static struct termios old_terminal;

static struct uade_slave *me = 0;
static struct playlist playlist;

static int one_subsong = 0;
static int playerforced = 0;


static void uade_restore_terminal(void) {
  tcsetattr(0, TCSANOW, &old_terminal);
}


/* catch CTRL-C and cause REBOOT */
static void usi_sigint_handler(int signum)
{
  fprintf(stderr, "signal handler catched\n");
  uade_exit(-1);
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


static int usi_setup(struct uade_song *song, int argc, char **argv)
{
  int no_more_opts, i, j, is_recursive = 0;
  struct termios tp;

  /* create CTRL-C break for cmdline tool */
  if (!uade_create_signalhandler(usi_sigint_handler, UADE_SIGINT)) {
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

  if (tcgetattr(0, &old_terminal)) {
    perror("uade: can't setup interactive mode");
    uade_exit(-1);
  }
  atexit(uade_restore_terminal);
  tp = old_terminal;
  tp.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT);
  if (tcsetattr(0, TCSAFLUSH, &tp)) {
    perror("uade: can't setup interactive mode (tcsetattr())");
    uade_exit(-1);
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


static int usi_list_empty(void)
{
  return playlist_empty(&playlist);
}

static void usi_skip_to_next_song(void)
{
}

static void usi_song_end(struct uade_song *song, char *reason, int kill_it)
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

static void usi_subsinfo(struct uade_song *song, int mins, int maxs, int curs)
{
  fprintf(stderr, "uade: subsong info: minimum: %d maximum: %d current: %d\n", mins, maxs, curs);
  song->min_subsong = mins;
  song->max_subsong = maxs;
  song->cur_subsong = curs;
}

static void usi_got_playername(char *playername)
{
  fprintf(stderr,"uade: playername: %s\n", playername);
}

static void usi_got_modulename(char *modulename)
{
  fprintf(stderr,"uade: modulename: %s\n", modulename);
}

static void usi_got_formatname(char *formatname)
{
  fprintf(stderr,"uade: formatname: %s\n", formatname);
}


/* strip leading whitespace from cmd_buf */
static void strip_whitespace(void) {
  int i;
  i = 0;
  while (i < cmd_buf_size) {
    if (cmd_buf[i] == '\n') {
      prompt_shown = 0;
    }
    if (!isspace((int) cmd_buf[i])) {
      break;
    }
    i++;
  }
  memmove(cmd_buf, &cmd_buf[i], cmd_buf_size - i);
  cmd_buf_size -= i;
}

static void strip_end(char *buf) {
  int p = strlen(buf) - 1;
  if (p < 0) {
    return;
  }
  for (; p >= 0; p--) {
    if (!isspace(buf[p])) {
      break;
    }
    buf[p] = 0;
  }
}

static int get_rest_of_line(char *buf, int maxlen) {
  int i;
  for (i = 0; i < cmd_buf_size; i++) {
    if (cmd_buf[i] == '\n') {
      cmd_buf[i] = 0;
      strlcpy(buf, cmd_buf, maxlen);
      memmove(cmd_buf, &cmd_buf[i+1], cmd_buf_size - (i + 1));
      cmd_buf_size -= i + 1;
      return 1;
    }
  }
  return 0;
}

/* shifts command buffer left from position p by l characters */
static void shift_left(char *buf, int *s, int p, int l) {
  if (p < 0 || p >= *s) {
    fprintf (stderr, "uade: shift_left error: p = %d\n", p);
    return;
  }
  if (l < 0) {
    fprintf (stderr, "uade: shift_left error: l < 0\n");
    return;
  }
  if (l == 0) {
    return;
  }
  if ((p + l) >= *s) {
    *s = p;
    return;
  }
  memmove(&buf[p], &buf[p+l], *s - (p + l));
  *s -= l;
}

static void insert_to_buf(char *d, int *ds, int maxl, int p, char *s, int ss) {
  if (ss == 0 || maxl == 0) {
    return;
  }
  if (*ds < 0 || maxl < 0 || p < 0 || ss < 0) {
    fprintf(stderr, "uade: insert_to_buf: *ds = %d maxl = %d p = %d ss = %d\n", *ds, maxl, p, ss);
    return;
  }
  if (p >= maxl) {
    fprintf(stderr, "uade: insert_to_buf: p >= maxl\n");
    return;
  }
  if ((*ds + ss) > maxl) {
    fprintf(stderr, "uade: insert_to_buf: cmd_buf overflow\n");
    return;
  }
  if (p < *ds) {
    memmove(&d[p + ss], &d[p], (*ds - p));
  } else {
    *ds = p;
  }
  memcpy(&d[p], s, ss);
  *ds += ss;
}

/* get a word from cmd_buf */
static int pop_word(char *buf, int maxlen) {
  int i, n, left;
  strip_whitespace();
  for (i = 0; i < cmd_buf_size; i++) {
    if (isspace((int) cmd_buf[i])) {
      n = i;
      if (n >= maxlen) {
	n = maxlen - 1;
      }
      strncpy(buf, cmd_buf, n);
      buf[n] = 0;
      left = (cmd_buf[i] == '\n') ? i : (i + 1); /* don't shift '\n' away */
      shift_left(cmd_buf, &cmd_buf_size, 0, left);
      return 1;
    }
  }
  return 0;
}

static int scan_cmd_buf(void) {
  int i;
  strip_whitespace();
  for (i=0; i<cmd_buf_size; i++) {
    if (cmd_buf[i] == '\n') {
      return INTERACTION_EOL;
    } else if (cmd_buf[i] == '\t') {
      tab_pos = i;
      shift_left(cmd_buf, &cmd_buf_size, i, 1);
      return INTERACTION_TAB;
    }
  }
  return 0;
}

static int compselect(const struct dirent *e) {
  return !strncmp(e->d_name, auto_pattern, strlen(auto_pattern));
}

static void auto_completion(void) {
  int p = tab_pos - 1;
  int nchars;
  char pattern[UADE_PATH_MAX];
  char path[UADE_PATH_MAX];
  char addition[UADE_PATH_MAX];
  int nadd;
  char *slash;
  struct dirent **namelist;
  int ret;
  DIR *d;
  int i;

  if (p < 0) {
    return;
  }
  nchars = 0;
  while (p > 0 && !isspace(cmd_buf[p])) {
    nchars++;
    p--;
  }
  if (nchars == 0 || p == 0) {
    return;
  }
  p++;
  if (nchars >= ((int) sizeof(pattern))) {
    fprintf(stderr, "uade: unexpected buffer shortage\n");
    return;
  }
  memcpy(pattern, &cmd_buf[p], nchars);
  pattern[nchars] = 0;
  /* fprintf(stderr, "would complete %s\n", pattern); */

  slash = strrchr(pattern, (int) '/');
  if (slash) {
    int l = (int) (slash - pattern);
    int pattlen = strlen(pattern) + 1;
    memcpy(path, pattern, l + 1);
    path[l + 1] = 0;
    shift_left(pattern, &pattlen, 0, l + 1);
    /* fprintf(stderr, "path = %s pattern = %s\n", path, pattern); */
  } else {
    strlcpy(path, ".", sizeof(path));
  }

  /* find matching patterns */
  strlcpy(auto_pattern, pattern, sizeof(auto_pattern));
  ret = scandir(path, &namelist, compselect, alphasort);

  /* check pattern results */
  if (ret >= 0) {
    switch (ret) {
    case 0:
      /* no matching patterns. shit. */
      break;
    case 1:
      /* straight auto-completion */
      strlcat(path, namelist[0]->d_name, sizeof(path));
      d = opendir(path);
      nchars = strlen(namelist[0]->d_name);
      nadd = 0;
      for (p = strlen(pattern); p < nchars ; p++) {
	printf("%c", namelist[0]->d_name[p]);
	addition[nadd] = namelist[0]->d_name[p];
	nadd++;
      }
      if (d) {
	printf("/");
	addition[nadd] = '/';
	nadd++;
	closedir(d);
      } else {
	printf(" ");
	addition[nadd] = ' ';
	nadd++;
      }
      /* fprintf(stderr, "inserting '%s' to '%s'\n", addition, pattern); */
      insert_to_buf(cmd_buf, &cmd_buf_size, sizeof(cmd_buf), tab_pos, addition, nadd);
      fflush(stdout);
      tab_state = 0;
      break;
    default:
      /* print all alternatives (more than one) */
      printf("\n");
      for (i = 0; i < ret; i++) {
	printf("%s\n", namelist[i]->d_name);
	free(namelist[i]);
      }
      printf("uade %d> ", cmd_number);
      for (i = 0; i < tab_pos ; i++) {
	printf("%c", cmd_buf[i]);
      }
      fflush(stdout);
      break;
    }
    free(namelist);
  }
}

static int from_buf(char *c, int i) {
  if (i >= cmd_buf_size) {
    return 0;
  }
  *c = cmd_buf[i];
  return 1;
}

static void clean_up_parts(char **buf, int n) {
  int i;
  if (buf) {
    for (i = 0; i < n; i++) {
      if (buf[i]) {
	free (buf[i]);
      }
    }
    free (buf);
  }
}

/* divides the command line into parts separated with white space. is aware of
   '"' characters. */
static int divide_line(char ***buf) {
  char token[UADE_PATH_MAX];
  int i, j, n;
  char c, nc;
  int brace;
  int maxn;
  maxn = 1;
  *buf = malloc(maxn * sizeof(char *));
  if (!(*buf)) {
    fprintf (stderr, "uade: out of memory for command line parsing\n");
    return 0;
  }
  n = 0;
  i = 0;
  while (1) {
    if (!from_buf(&c, i)) {
      break;
    }
    if (isspace(c)) {
      if (c == '\n') {
	break;
      }
      i++;
      continue;
    }

    brace = 0;
    j = 0;
    while (1) {
      if (!from_buf(&c, i)) {
	break;
      }
      if (c == '\"') {
	if (brace) {
	  if (from_buf(&nc, i + 1)) {
	    if (isspace(nc)) {
	      i += 2;
	      break;
	    } else {
	      goto copy;
	    }
	  } else {
	    i++;
	    break;
	  }
	}
	brace = 1;
	i++;
	continue;

      } else if (c == '\\') {
	if (from_buf(&nc, i + 1)) {
	  c = nc;
	  i++; /* skip '\\' */
	}
      } else if (isspace(c)) {
	if (!brace) {
	  i++;
	  break;
	}
      }

    copy:
      if (j >= ((int) sizeof(token))) {
	fprintf(stderr, "uade: command editing buffer exceeded\n");
	break;
      }
      token[j++] = c;
      i++;
    }
    token[j] = 0;

    if (n >= maxn) {
      char **newbuf;
      maxn *= 2;
      newbuf = realloc(*buf, maxn * sizeof(char *));
      if (!newbuf) {
	goto line_clean_up;
      }
      *buf = newbuf;
    }
    (*buf)[n] = strdup(token);
    if (!(*buf)[n]) {
      goto line_clean_up;
    }
    n++;
    /* fprintf(stderr, "part '%s'\n", token); */
  }
  return n;

 line_clean_up:
  clean_up_parts(*buf, n);
  return 0;
}

static int usi_interaction(struct uade_command *cmd, int wait_for) {
  struct timeval nulltime;
  fd_set ifds;

  int ret, maxlen;
  int i, j;
  int bytes_read;
  char **parts = 0;
  int nparts = 0;

  bzero(cmd, sizeof(*cmd));

  if (!prompt_shown) {
    printf("uade %d> ", cmd_number);
    fflush(stdout);
    prompt_shown = 1;
    cmd_number++;
  }

  bytes_read = 0;

  FD_ZERO(&ifds);
  FD_SET(0, &ifds);
  nulltime.tv_sec = 0;
  nulltime.tv_usec = 0;

  ret = select(1, &ifds, 0, 0, wait_for ? 0 : &nulltime);

  if (ret > 0) {
    maxlen = sizeof(cmd_buf) - cmd_buf_size;
    if (maxlen > 0) {
      ret = read(0, &cmd_buf[cmd_buf_size], maxlen);
      if (ret < 0) {
	if (errno != EINTR) {
	  goto no_interaction;
	}
      } else if (ret == 0) {
	goto no_interaction;
      } else {
	cmd_buf_size += ret;
	bytes_read = ret;
      }
    }

  } else if (ret < 0) {
    if (errno != EINTR) {
      goto no_interaction;
    }
  }

  for (i = cmd_buf_size - bytes_read; i < cmd_buf_size;) {
    if (cmd_buf[i] == '\t') {
      break;
    }
    if (cmd_buf[i] == 0x7f) {
      if (i > 0) {
	printf("%c%c%c", 0x1b, 0x5b, 0x44);
	printf(" ");
	printf("%c%c%c", 0x1b, 0x5b, 0x44);
	shift_left(cmd_buf, &cmd_buf_size, i - 1, 2);
	i = i - 1;
      } else {
	shift_left(cmd_buf, &cmd_buf_size, i, 1);
      }
      continue;
    }
    printf("%c", cmd_buf[i]);
    i++;
  }
  if (bytes_read > 0) {
    fflush(stdout);
  }

  ret = scan_cmd_buf();
  if (!ret) {
    tab_state = 0;
    return 0;

  } else if (ret == INTERACTION_EOL) {
    tab_state = 0;

  } else if (ret == INTERACTION_TAB) {
    tab_state++;
    auto_completion();
    return 0;

  } else {
    fprintf(stderr, "uade: odd interaction input\n");
    return 0;
  }

  nparts = divide_line(&parts);

  /* remove the rest of the line from cmd_buf[] */
  i = 0;
  while (i < cmd_buf_size) {
    if (cmd_buf[i] == '\n') {
      i++;
      break;
    }
    i++;
  }
  shift_left(cmd_buf, &cmd_buf_size, 0, i);

  if (!nparts) {
    return 0;
  }

  if (!strcmp("next", parts[0]) || !strcmp("n", parts[0])) {
    cmd->type = UADE_SONG_END;
    goto got_command;
  } else if (!strcmp("m", parts[0])) {
    cmd->type = UADE_REBOOT;
    goto got_command;
  }

  i = 0;
  while (parts[0][i]) {
    if (!isdigit((int) parts[0][i])) {
      goto not_a_number;
    }
    i++;
  }
  cmd->type = UADE_SETSUBSONG;
  cmd->ret = malloc(sizeof(int));
  if (!cmd->ret) {
    goto no_interaction;
  }
  *((int *) cmd->ret) = (int) atoi(parts[0]);
  goto got_command;

 not_a_number:

  if (!strcmp("play", parts[0])) {
    glob_t pglob;
#ifndef GLOB_TILDE
#define GLOB_TILDE 0
#endif
    for (i = 1; i < nparts; i++) {
      if (!glob(parts[i], GLOB_NOCHECK | GLOB_TILDE, 0, &pglob)) {
	for (j = 0;  j < ((int) pglob.gl_pathc); j++) {
	  if (!playlist_add(&playlist, pglob.gl_pathv[j], 1)) {
	    fprintf(stderr, "uade: couldn't add %s into playlist\n", pglob.gl_pathv[j]);
	  }
	}
	globfree(&pglob);
      }
    }
    prompt_shown = 0;
    goto no_command;
  }
  
  if (!strcmp("cd", parts[0])) {
    glob_t pglob;
    for (i = 1; i < nparts; i++) {
      if (!glob(parts[i], GLOB_NOCHECK | GLOB_TILDE, 0, &pglob)) {
	for (j = 0; j < ((int) pglob.gl_pathc); j++) {
	  if (chdir(pglob.gl_pathv[j])) {
	    fprintf (stderr, "can't change to directory '%s'\n", pglob.gl_pathv[j]);
	  } else {
	    break;
	  }
	}
	globfree(&pglob);
      }
    }
    prompt_shown = 0;
    goto no_command;
  }
  
  if (!strcmp("ls", parts[0])) {
    char path[UADE_PATH_MAX];
    struct dirent **namelist;
    int n;
    char *name;
    if (nparts > 1) {
      strlcpy(path, parts[1], sizeof(path));
    } else {
      strlcpy(path, ".", sizeof(path));
    }
    n = scandir(path, &namelist, 0, alphasort);
    if (n < 0) {
      fprintf(stderr, "can't scan directory '%s'\n", path);
    } else {
      for (i = 0; i < n; i++) {
	name = namelist[i]->d_name;
	if (strcmp(".", name) && strcmp("..", name)) {
	  printf("%s\n", name);
	}
	free(namelist[i]);
      }
      free(namelist);
    }
    prompt_shown = 0;
    goto no_command;
  }

  if (!strcmp("flush", parts[0])) {
    playlist_flush(&playlist);
    prompt_shown = 0;
    goto no_command;
  }

  if (!strcmp("exit", parts[0]) || !strcmp("quit", parts[0])) {
    printf("Exit requested.\n");
    uade_exit(-1);
  }

  /* toggle random play */
  if (!strcmp("random", parts[0]) || !strcmp("r", parts[0])) {
    int rstate = playlist_random(&playlist, -1);
    printf("Random play %s.\n", rstate ? "enabled" : "disabled");
    prompt_shown = 0;
    goto no_command;
  }

  if (!strcmp("help", parts[0]) || !strcmp("h", parts[0])) {
    printf("<number>\tGo to subsong <number>\n");
    printf("cd <dir>\tSwitch to directory <dir>\n");
    printf("exit, quit\tExit UADE\n");
    printf("f\t\tToggle LED Filter\n");
    printf("flush\t\tFlush playlist\n");
    printf("h, help\t\tDisplay help\n");
    printf("m\t\tNext song\n");
    printf("n\t\tNext (sub)song\n");
    printf("play <pattern>\tEnqueue songs matching shell pattern\n");
    printf("r, random\tToggle random play\n");
    prompt_shown = 0;
    goto no_command;
  }

  if (strlen(parts[0]) == 1) {
    switch (parts[0][0]) {
    case 'f':
      cmd->type = UADE_TOGGLE_LED;
      goto got_command;
    default:
      break;
    }
  }

  printf("unknown command for interaction\n");
  prompt_shown = 0;

 no_command:
  clean_up_parts(parts, nparts);
  return 0;

 got_command:
  clean_up_parts(parts, nparts);
  prompt_shown = 0;
  return 1;

 no_interaction:
  clean_up_parts(parts, nparts);
  cmd->type = UADE_NO_INTERACTION;
  return 1;
}


static int usi_get_next(struct uade_song *song)
{
  while (playlist_empty(&playlist)) {
    struct uade_command cmd;
    usi_interaction(&cmd, 1);
    if (cmd.type == UADE_NO_INTERACTION)
      return 0;
  }

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


void usi_functions(struct uade_slave *slave)
{
  me = slave;
  me->setup = usi_setup;
  me->get_next = usi_get_next;
  me->list_empty = usi_list_empty;
  me->skip_to_next_song = usi_skip_to_next_song;
  me->song_end = usi_song_end;
  me->subsinfo = usi_subsinfo;
  me->got_playername = usi_got_playername;
  me->got_modulename = usi_got_modulename;
  me->got_formatname = usi_got_formatname;
  me->interaction = usi_interaction;
}
