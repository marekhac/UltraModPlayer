 /* 
  * UAE - The Un*x Amiga Emulator
  * 
  * Support for Linux/USS sound
  * 
  * Copyright 1997 Bernd Schmidt
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "config.h"
#include "options.h"
#include "memory.h"
#include "custom.h"
#include "gensound.h"
#include "sd-sound.h"

#include <sys/ioctl.h>

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#elif defined HAVE_MACHINE_SOUNDCARD_H
#include <machine/soundcard.h>
#elif defined HAVE_SOUNDCARD_H
#include <soundcard.h>
#else
#error "Something went wrong during configuration."
#endif

#ifdef __FreeBSD__
#ifndef AFMT_S16_NE
#include <machine/endian.h>
#if BYTE_ORDER == LITTLE_ENDIAN
#   define AFMT_S16_NE AFMT_S16_LE              /* native endian signed 16 */
#else
#   define AFMT_S16_NE AFMT_S16_BE
#endif
#endif
#endif

#include "uade.h"

int sound_fd;
static int have_sound = 0;

uae_u16 sndbuffer[44100];
uae_u16 *sndbufpt;
int sndbufsize;

int sound_bytes_per_sample;

char *sound_device_oss = "/dev/dsp";

static int exact_log2 (int v)
{
    int l = 0;
    while ((v >>= 1) != 0)
	l++;
    return l;
}

void close_sound (void)
{
    if (have_sound)
	close (sound_fd);
}

/* Try to determine whether sound is available.  This is only for GUI purposes.  */
int setup_sound (void)
{
  int tmp;
  if (uade_local_sound) {
    sound_available = 0;
    if (uade_unix_sound_device) {
      sound_device_oss = uade_unix_sound_device;
    }
    sound_fd = open (sound_device_oss, O_WRONLY);
    if (sound_fd < 0) {
      fprintf (stderr, "uade: can't open sound device '%s': %s\n", sound_device_oss, strerror(errno));
      if (errno == EBUSY) {
	/* We can hope, can't we ;) */
	sound_available = 1;
	return 1;
      }
      return 0;
    }
    if (ioctl (sound_fd, SNDCTL_DSP_GETFMTS, &tmp) == -1) {
      perror ("uade: ioctl failed - can't use sound");
      close (sound_fd);
      return 0;
    }
    close(sound_fd);
  }
  sound_available = 1;
  return 1;
}

int init_sound (void)
{
  int tmp;
  int channels;
  int rate;
  int dspbits;
  int format;

  if (currprefs.sound_maxbsiz < 128 || currprefs.sound_maxbsiz > 16384) {
    fprintf (stderr, "Sound buffer size %d out of range.\n", currprefs.sound_maxbsiz);
    currprefs.sound_maxbsiz = 8192;
  }

  dspbits = currprefs.sound_bits;
  rate    = currprefs.sound_freq;
  sound_bytes_per_sample = dspbits / 8;

  if (uade_local_sound) {
    sound_fd = open (sound_device_oss, O_WRONLY);
    have_sound = !(sound_fd < 0);
    if (!have_sound) {
      perror ("uade: can't open /dev/dsp");
      if (errno != EBUSY)
	sound_available = 0;
      return 0;
    }

    format = (dspbits == 16) ? AFMT_S16_NE : AFMT_U8;
    tmp = format;
    if (ioctl (sound_fd, SNDCTL_DSP_SETFMT, &tmp) == -1) {
      fprintf(stderr, "uade: can't init sound with %d bits\n", dspbits);
      close (sound_fd);
      have_sound = 0;
      return 0;
    }
    if (tmp != format) {
      fprintf(stderr, "uade: endianess problem noticed. report this. continuing anyway. you may get horrible sounds, but please report to us!\n");
    }

    channels = currprefs.stereo ? 2 : 1;
    tmp = channels;
    if (ioctl (sound_fd, SNDCTL_DSP_CHANNELS, &tmp) == -1) {
      fprintf(stderr, "uade: can't set requested amount of channels.\n");
      close (sound_fd);
      have_sound = 0;
      return 0;
    }
    if (tmp != channels) {
      fprintf(stderr, "uade: can't set number of channels to %s, setting to %d\n", channels, tmp);
      currprefs.stereo = (tmp == 2) ? 1 : 0;
    }

    ioctl (sound_fd, SNDCTL_DSP_SPEED, &rate);

    ioctl (sound_fd, SOUND_PCM_READ_RATE, &rate);
    /* Some soundcards have a bit of tolerance here. */
    if (rate < currprefs.sound_freq * 90 / 100 || rate > currprefs.sound_freq * 110 / 100) {
      fprintf (stderr, "uade: can't use sound with desired frequency %d\n", currprefs.sound_freq);
      close (sound_fd);
      have_sound = 0;
      return 0;
    }

    /* max fragments = 4, fragment size = log2(maxbsiz)  */
    tmp = 0x00040000 + exact_log2 (currprefs.sound_maxbsiz);
    ioctl (sound_fd, SNDCTL_DSP_SETFRAGMENT, &tmp);
    ioctl (sound_fd, SNDCTL_DSP_GETBLKSIZE, &sndbufsize);

  } else {
    have_sound = 1;
    sndbufsize = 4096;
  }

  sample_evtime = (long) maxhpos * maxvpos * 50 / rate;
  if (dspbits == 16) {
    init_sound_table16 ();
    sample_handler = currprefs.stereo ? sample16s_handler : sample16_handler;
  } else {
    init_sound_table8 ();
    sample_handler = currprefs.stereo ? sample8s_handler : sample8_handler;
  }
  sound_available = 1;

  /*  if (uade_local_sound) {
    fprintf (stderr,"Sound driver found and configured for %d bits %s at %d Hz, buffer is %d bytes\n", dspbits, currprefs.stereo ? "stereo" : "mono", rate, sndbufsize);
  }
  */

  sndbufpt = sndbuffer;
#ifdef FRAME_RATE_HACK
  vsynctime = vsynctime * 9 / 10;
#endif	
  return 1;
}

/* this should be called between subsongs when remote slave changes subsong */
void flush_sound (void)
{
  sndbufpt = sndbuffer;
}
