/* 
 * UADE
 * 
 * Support for ALSA sound
 * 
 * Copyright 2004 Heikki Orsila <heikki.orsila@iki.fi>
 */

#include <alsa/asoundlib.h>
#include <errno.h>
#include <string.h>

#include "uade.h"
#include "uade-os.h"

extern uae_u16 sndbuffer[];
extern uae_u16 *sndbufpt;
extern int sndbufsize;
extern int sound_bytes_per_sample;
extern snd_pcm_t *alsa_playback_handle;
extern int alsa_to_frames_divisor;

extern void finish_sound_buffer (void);

/* alsa_xrun_recovery() function is copied from ALSA manual. why the hell did
   they make ALSA this hard?! i bet 95% of ALSA programmers would like a
   simpler way to do error handling.. let the 5% use tricky APIs.
*/
static int alsa_xrun_recovery(snd_pcm_t *handle, int err) {
  if (err == -EPIPE) {
    /* under-run */
    err = snd_pcm_prepare(handle);
    if (err < 0)
      fprintf(stderr, "uade: no recovery with alsa from underrun, prepare failed: %s\n", snd_strerror(err));
    return 0;
  } else if (err == -ESTRPIPE) {
    while ((err = snd_pcm_resume(handle)) == -EAGAIN) {
      /* wait until the suspend flag is released */
      fprintf(stderr, "uade: sleeping for alsa.\n");
      sleep(1);
    }
    if (err < 0) {
      err = snd_pcm_prepare(handle);
      if (err < 0)
	fprintf(stderr, "uade: no recovery with alsa from suspend, prepare failed: %s\n", snd_strerror(err));
    }
    return 0;
  }
  return err;
}

static void check_sound_buffers (void) {
   if ((char *) sndbufpt - (char *) sndbuffer >= sndbufsize) {

      if ((char *) sndbufpt - (char *) sndbuffer > sndbufsize) {
	 fprintf(stderr, "uade: A bug in sound buffer writing. Report this!\n");
      }
      
      if (uade_check_sound_buffers(sndbuffer, sndbufsize, sound_bytes_per_sample)) {
	 int frames = sndbufsize / alsa_to_frames_divisor;
	 char *buf = (char *) sndbuffer;
	 int ret;
	 while (frames > 0) {
	   ret = snd_pcm_writei(alsa_playback_handle, buf, frames);
	   if (ret < 0) {
	     if (ret == -EAGAIN || ret == -EINTR)
	       continue;
	     if (alsa_xrun_recovery(alsa_playback_handle, ret) < 0) {
	       fprintf(stderr, "uade: write error with alsa: %s\n", snd_strerror(ret));
	       uade_exit(-1);
	     }
	     continue;
	   }
	   frames -= ret;
	   buf += ret * alsa_to_frames_divisor;
	 }
      }

      sndbufpt = sndbuffer;
   }
}

#define PUT_SOUND_BYTE(b) do { *(uae_u8 *)sndbufpt = b; sndbufpt = (uae_u16 *)(((uae_u8 *)sndbufpt) + 1); } while (0)
#define PUT_SOUND_WORD(b) do { *(uae_u16 *)sndbufpt = b; sndbufpt = (uae_u16 *)(((uae_u8 *)sndbufpt) + 2); } while (0)
#define PUT_SOUND_BYTE_LEFT(b) PUT_SOUND_BYTE(b)
#define PUT_SOUND_WORD_LEFT(b) PUT_SOUND_WORD(b)
#define PUT_SOUND_BYTE_RIGHT(b) PUT_SOUND_BYTE(b)
#define PUT_SOUND_WORD_RIGHT(b) PUT_SOUND_WORD(b)
#define SOUND16_BASE_VAL 0
#define SOUND8_BASE_VAL 128

#define DEFAULT_SOUND_MAXB 8192
#define DEFAULT_SOUND_MINB 8192
#define DEFAULT_SOUND_BITS 16
#define DEFAULT_SOUND_FREQ 44100
#define HAVE_STEREO_SUPPORT
