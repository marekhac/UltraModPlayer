 /* 
  * UAE - The Un*x Amiga Emulator
  * 
  * Support for Solaris sound
  * 
  * Copyright 1996, 1997 Manfred Thole
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "config.h"
#include "options.h"
#include "memory.h"
#include "custom.h"
#include "audio.h"
#include "gensound.h"
#include "sd-sound.h"

#if !defined HAVE_SYS_AUDIOIO_H

/* SunOS 4.1.x */
#include <sys/ioctl.h>
#include <sys/audioio.h>
#ifndef AUDIO_ENCODING_LINEAR
#define AUDIO_ENCODING_LINEAR (3)
#endif

#else

/* SunOS 5.x/NetBSD */
#include <sys/audioio.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#endif

int sndbufsize;
int sound_fd;
static int have_sound;
uae_u16 sndbuffer[44100];
uae_u16 *sndbufpt;

int sound_bytes_per_sample;

void close_sound(void)
{
    if (have_sound)
	close(sound_fd);
}

int setup_sound(void)
{
  if (uade_local_sound) {
    sound_fd = open ("/dev/audio", O_WRONLY);
    have_sound = !(sound_fd < 0);
    if (!have_sound)
      return 0;
  }
  sound_available = 1;
  return 1;
}

int init_sound (void)
{
    int rate, dspbits, channels;

    struct audio_info sfd_info;

    if (currprefs.sound_maxbsiz < 128 || currprefs.sound_maxbsiz > 44100) {
	fprintf(stderr, "Sound buffer size %d out of range.\n", currprefs.sound_maxbsiz);
	currprefs.sound_maxbsiz = 8192;
    }

    rate = currprefs.sound_freq;
    dspbits = currprefs.sound_bits;

    sound_bytes_per_sample = dspbits / 8;

    channels = currprefs.stereo ? 2 : 1;
    if (dspbits == 8) {
      /* don't have ulaw stereo handler */
      channels = 1;
    }

    if (uade_local_sound) {

      AUDIO_INITINFO(&sfd_info);

      sfd_info.play.sample_rate = rate;
      sfd_info.play.precision = dspbits;
      sfd_info.play.encoding = (dspbits == 8 ) ? AUDIO_ENCODING_ULAW : AUDIO_ENCODING_LINEAR;

      sfd_info.play.channels = channels;
      if (ioctl(sound_fd, AUDIO_SETINFO, &sfd_info)) {
	fprintf(stderr, "uade: can't use sample rate %d with %d %d bit channels, %s!\n", rate,  channels, dspbits, (dspbits ==8) ? "ulaw" : "linear");
	channels = 1;
	sfd_info.play.channels = channels;
	if (ioctl(sound_fd, AUDIO_SETINFO, &sfd_info)) {
	  fprintf(stderr, "uade: can't use sample rate %d with %d %d bit channels, %s!\n", rate, channels, dspbits, (dspbits ==8) ? "ulaw" : "linear");
	  return 0;
	}
      }
    }

    sample_evtime = (long) maxhpos * maxvpos * 50 / rate;

    if (dspbits == 16) {
      init_sound_table16 ();
      sample_handler = (channels == 2) ? sample16s_handler : sample16_handler;
    } else if (dspbits == 8) {
      init_sound_table8();
      sample_handler = sample_ulaw_handler;
    } else {
      fprintf(stderr, "illegal number of sample bits (%d)\n", dspbits);
      return 0;
    }

    sndbufpt = sndbuffer;
    sound_available = 1;
    sndbufsize = currprefs.sound_maxbsiz;
    fprintf (stderr, "Sound driver found and configured for %d %d bit channels, %s at %d Hz, buffer is %d bytes\n", channels, dspbits, (dspbits ==8) ? "ulaw" : "linear", rate, sndbufsize);
    return 1;
}

void flush_sound(void)
{
    sndbufpt = sndbuffer;
}
