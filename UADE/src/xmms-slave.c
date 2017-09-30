/* Copyright 2003 Heikki Orsila <heikki.orsila@iki.fi>
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "../osdep/strl.c"

#include "uade.h"
#include "uade-os.h"
#include "xmms-slave.h"
#include "xmms-slave-msg.h"

struct uade_msgstruct *uade_struct;

static void uade_signalhandler(int);

static const int bufsize = sizeof(uade_struct->soundbuffer);

static int free_in_buffer(void) {
  int fs;
  int readoffset = uade_struct->sbuf_readoffset;
  /* Assumption is that write is always ahead or at the same position
     (this assumption means that if r=w then there is 64k free)
     cases are:
     1: r=w: free = bufsize - 1 (else case)
     2: w>r: free = r + bufsize - w - 1 (else case) (consistent with case 1)
     3: w<r: free = r - w - 1 (first case in if structure)
  */
  if (uade_struct->sbuf_writeoffset < readoffset) {
    /* case 3 */
    fs = readoffset - uade_struct->sbuf_writeoffset - 1;
  } else {
    /* case 1 and 2 */
    fs = bufsize + readoffset - uade_struct->sbuf_writeoffset - 1;
  }
  return fs;
}

static void xmms_slave_write(void *sndbuffer, int bytes) {
  const int sleepunit = 10000;
  const int maxsleep = 1000000;
  int fs, sleepcounter, firstsize;

  if (!uade_struct->dontwritebit) {

    for (sleepcounter = 0; sleepcounter < maxsleep;) {
      fs = free_in_buffer();
      if (fs >= bytes)
	break;
      if (uade_struct->dontwritebit)
	break;
      uade_usleep(sleepunit);
      sleepcounter += sleepunit;
    }

    while (uade_struct->plugin_pause_boolean)
      uade_usleep(sleepunit);

    /* copy sndbuffer to uade shared memory sound buffer (take into
       account that the buffer is circular) */
    if((uade_struct->sbuf_writeoffset + bytes) > bufsize) {
      firstsize = bufsize - uade_struct->sbuf_writeoffset;
      memcpy(uade_struct->soundbuffer + uade_struct->sbuf_writeoffset, sndbuffer, firstsize);
      memcpy(uade_struct->soundbuffer, ((char *) sndbuffer) + firstsize, bytes - firstsize);
    } else {
      memcpy(uade_struct->soundbuffer + uade_struct->sbuf_writeoffset, sndbuffer, bytes);
    }
    /* increase write counter */
    uade_struct->sbuf_writeoffset = (uade_struct->sbuf_writeoffset + bytes) % bufsize;
  }
}

static void xmms_slave_signalhandler(int sig)
{
  int msg = uade_struct->touaemsgtype;

  sig = sig; /* no warning */

  uade_struct->touaemsgtype = 0;

  switch (msg) {
  case UADE_REBOOT:
    /* reboot, switch to next song, ... */
    uade_reboot = 1;
    break;

  case UADE_SETSUBSONG:
    /* this is used to set new subsong during playback */
    uade_change_subsong(uade_struct->subsong);
    uade_struct->set_subsong = 0;
    break;

  case UADE_PLAYERNAME:
    /* this signal informs that (playername, modulename, scorename)
       has been set into uade_struct */
    break;

  case UADE_NTSC:
    /* switch between ntsc/pal mode */
    uade_set_ntsc(uade_struct->ntscbit);
    uade_send_amiga_message(UADE_NTSC);
    break;

  case UADE_SONG_END:
    /* this does mean song end. sets automatic song end 'on' or 'off' */
    uade_set_automatic_song_end(uade_struct->songendpossible);
    break;

  default:
    fprintf(stderr,"uade: signal handler: msg was not recognized (%d)\n", msg);
    break;
  }
}

/* called once when uade process is started */
static int xmms_slave_setup(struct uade_song *song, int argc, char **argv)
{
  char *mapname;
  pid_t father;
  int i;

  mapname = 0;

  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--xmms-slave")) {
      if ((i + 1) >= argc) {
	fprintf(stderr, "uade: fatal error: not enough parameters for --xmms-slave\n");
	uade_exit(-1);
      }
      mapname = argv[i + 1];
      break;
   }
  }

  if (!mapname) {
    fprintf(stderr, "uade: fatal error: --xmms-slave parameter is needed!\n");
    uade_exit(-1);
  }

  uade_struct = (struct uade_msgstruct *) uade_mmap_file(mapname, sizeof(struct uade_msgstruct));
  if (!uade_struct) {
    fprintf(stderr, "uade.c/uade: couldn't mmap file (%s)\n", mapname);
    uade_exit(-1);
  }

  /* setup a signal handler for communication with xmms plugin */
  if (!uade_create_signalhandler(xmms_slave_signalhandler, UADE_SIGHUP)) {
    fprintf(stderr, "uade: fatal error: couldn't setup signal handler.\n");
    uade_exit(-1);
  }

  /* leave a watchdog that checks if XMMS plugin dies (polls plugin
     task with a kill(pluginpid, 0) signal every 0.5 seconds). If XMMS
     plugin dies, watchdog kills UADE and itself */
  father = getpid();
  if (!fork()) {
    while (1) {
      uade_sleep(2);
      if (uade_struct->masterpid) {
	if (kill(uade_struct->masterpid, 0) < 0) {
	  break;
	}
      }
      if (kill(father, 0) < 0) {
	fprintf(stderr, "uade: interesting... my father has died. i will go too...\n");
	break;
      }
    }
    /* remove the shared memory file that is used by uade and xmms plugin */
    (void) remove(mapname);
    kill(father, SIGKILL);
    uade_exit(-1);
  }

  uade_struct->min_subsong = 0;
  uade_struct->max_subsong = 0;

  /* no native sound output, let xmms take care of it :-) */
  uade_local_sound = 0;

  return 1;
}

static int xmms_slave_get_next(struct uade_song *song)
{
  /* RACE CONDITION BEGINS (PENALTY OF LOSING IS 2 SECONDS) */
  /* uade executable is ready to receive commands */
  uade_struct->uade_inited_boolean = 1;
  /* uade_sleep() is interrupted by a SIGHUP from XMMS plugin */
  while (uade_struct->loadnewsongboolean == 0) {
    uade_sleep(2);
  }
  /* RACE CONDITION ENDS */

  strlcpy(song->playername, uade_struct->playername, sizeof(song->playername));
  strlcpy(song->modulename, uade_struct->modulename, sizeof(song->modulename));
  strlcpy(song->scorename, uade_struct->scorename, sizeof(song->scorename));

  uade_struct->loadnewsongboolean = 0;

  song->force_by_default = uade_struct->force_by_default;
  if (uade_struct->set_subsong) {
    uade_struct->set_subsong = 0;
    song->set_subsong = 1;
    song->subsong = uade_struct->subsong;
  }
  uade_struct->min_subsong = uade_struct->max_subsong = 0;
  song->song_end_possible = uade_struct->songendpossible;

  song->use_ntsc = uade_struct->ntscbit;

  song->use_filter = uade_struct->use_filter;

  return 1;
}

static void xmms_slave_post_init(void)
{
  uade_struct->score_playername[0] = 0;
  uade_struct->score_playerauthor[0] = 0;
  uade_struct->score_modulename[0] = 0;
  uade_struct->score_formatname[0] = 0;
}

static void xmms_slave_flush_sound(void)
{
  memset(uade_struct->soundbuffer, 0, sizeof(uade_struct->soundbuffer));
}

static void xmms_slave_song_end(struct uade_song *song, char *reason, int kill_it)
{
  uade_struct->song_end = 1;
}

static void xmms_slave_subsinfo(struct uade_song *song, int mins, int maxs,
				int curs)
{
  fprintf(stderr, "uade: subsong info: minimum: %d maximum: %d current: %d\n", mins, maxs, curs);
  uade_struct->min_subsong = mins;
  uade_struct->max_subsong = maxs;
  uade_struct->subsong = curs;
}

static void xmms_slave_got_playername(char *playername)
{
  fprintf(stderr,"uade: playername: %s\n", playername);
  strlcpy(uade_struct->score_playername, playername, sizeof(uade_struct->score_playername));
}

static void xmms_slave_got_modulename(char *modulename)
{
  fprintf(stderr,"uade: modulename: %s\n", modulename);
  strlcpy(uade_struct->score_modulename, modulename, sizeof(uade_struct->score_modulename));
}

static void xmms_slave_got_formatname(char *formatname)
{
  fprintf(stderr,"uade: formatname: %s\n", formatname);
  strlcpy(uade_struct->score_formatname, formatname, sizeof(uade_struct->score_formatname));
}

static void xmms_slave_skip_to_next_song(void)
{
  uade_struct->song_end = 1;
}

void xmms_slave_functions(struct uade_slave *slave)
{
  slave->setup = xmms_slave_setup;
  slave->get_next = xmms_slave_get_next;
  slave->post_init = xmms_slave_post_init;
  slave->flush_sound = xmms_slave_flush_sound;
  slave->write = xmms_slave_write;
  slave->song_end = xmms_slave_song_end;
  slave->subsinfo = xmms_slave_subsinfo;
  slave->got_playername = xmms_slave_got_playername;
  slave->got_modulename = xmms_slave_got_modulename;
  slave->got_formatname = xmms_slave_got_formatname;
  slave->skip_to_next_song = xmms_slave_skip_to_next_song;
}
