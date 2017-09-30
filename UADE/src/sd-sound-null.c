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

#include "uade.h"

static int have_sound = 0;

uae_u16 sndbuffer[44100];
uae_u16 *sndbufpt;
int sndbufsize;

int sound_bytes_per_sample;

void close_sound (void)
{
}

/* Try to determine whether sound is available.  This is only for GUI purposes.
 */
int setup_sound (void)
{
  sound_available = 1;
  return 1;
}

int init_sound (void)
{
  int dspbits;
  int rate;

  if (currprefs.sound_maxbsiz < 128 || currprefs.sound_maxbsiz > 16384) {
    fprintf (stderr, "Sound buffer size %d out of range.\n", currprefs.sound_maxbsiz);
    currprefs.sound_maxbsiz = 8192;
  }

  dspbits = currprefs.sound_bits;
  rate    = currprefs.sound_freq;
  sound_bytes_per_sample = dspbits / 8;

  have_sound = 1;
  sndbufsize = 4096;

  sample_evtime = (long) maxhpos * maxvpos * 50 / rate;
  if (dspbits == 16) {
    init_sound_table16 ();
    sample_handler = currprefs.stereo ? sample16s_handler : sample16_handler;
  } else {
    init_sound_table8 ();
    sample_handler = currprefs.stereo ? sample8s_handler : sample8_handler;
  }
  sound_available = 1;
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
