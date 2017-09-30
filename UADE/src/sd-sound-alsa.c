/* 
 * UADE
 * 
 * Support for ALSA sound
 * 
 * Copyright 2004 Heikki Orsila <heikki.orsila@iki.fi>
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include "config.h"
#include "options.h"
#include "memory.h"
#include "custom.h"
#include "gensound.h"
#include "sd-sound.h"

#include <alsa/asoundlib.h>

#include "uade.h"

uae_u16 sndbuffer[44100];
uae_u16 *sndbufpt;
int sndbufsize;

int sound_bytes_per_sample;
int alsa_to_frames_divisor = 4;

snd_pcm_t *alsa_playback_handle = 0;


void close_sound (void)
{
   if (alsa_playback_handle)
   	snd_pcm_close (alsa_playback_handle);
}

/* Try to determine whether sound is available.  This is only for GUI purposes.  */
int setup_sound (void)
{
   sound_available = 1;
   return 1;
}

int init_sound (void)
{
   int channels;
   int dspbits;
   unsigned int rate;

   if (currprefs.sound_maxbsiz < 128 || currprefs.sound_maxbsiz > 16384) {
      fprintf (stderr, "Sound buffer size %d out of range.\n", currprefs.sound_maxbsiz);
      currprefs.sound_maxbsiz = 8192;
   }
   sndbufsize = 8192;

   dspbits = currprefs.sound_bits;
   rate    = currprefs.sound_freq;
   sound_bytes_per_sample = dspbits / 8;
   channels = currprefs.stereo ? 2 : 1;

   if (uade_local_sound) {
      int alsamode;
      int err;
      snd_pcm_uframes_t buffer_frames;
      snd_pcm_hw_params_t *hw_params;
      char *alsa_device = uade_unix_sound_device ? uade_unix_sound_device : "default";

      if ((err = snd_pcm_open (&alsa_playback_handle, alsa_device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
	 fprintf (stderr, "cannot open audio device (%s)\n", snd_strerror (err));
	 goto nosound;
      }
      
      if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
	 fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
		  snd_strerror (err));
	 goto nosound;
      }

      if ((err = snd_pcm_hw_params_any (alsa_playback_handle, hw_params)) < 0) {
	 fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
		  snd_strerror (err));
	 goto nosound;
      }
      
      if ((err = snd_pcm_hw_params_set_access (alsa_playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
	 fprintf (stderr, "cannot set access type (%s)\n",
		  snd_strerror (err));
	 goto nosound;
      }

      switch (dspbits) {
       case 8:
	 alsamode = SND_PCM_FORMAT_U8;
	 break;
       case 16:
	 alsamode = SND_PCM_FORMAT_S16;
	 break;
       default:
	 fprintf(stderr, "uade: %d bit samples not supported with alsa\n", dspbits);
	 goto nosound;
      }
      
      if ((err = snd_pcm_hw_params_set_format (alsa_playback_handle, hw_params, alsamode)) < 0) {
	 fprintf (stderr, "cannot set sample format (%s)\n",
		  snd_strerror (err));
	 goto nosound;
      }

      alsa_to_frames_divisor = channels * dspbits / 8;

      if ((err = snd_pcm_hw_params_set_channels (alsa_playback_handle, hw_params, channels)) < 0) {
	 fprintf (stderr, "cannot set channel count (%s)\n",
		  snd_strerror (err));
	 goto nosound;
      }
      
      if ((err = snd_pcm_hw_params_set_rate_near (alsa_playback_handle, hw_params, &rate, 0)) < 0) {
	 fprintf (stderr, "cannot set sample rate (%s)\n",
		  snd_strerror (err));
	 goto nosound;
      }

      buffer_frames = sndbufsize / alsa_to_frames_divisor;
      snd_pcm_hw_params_set_period_size_near(alsa_playback_handle, hw_params, &buffer_frames, 0);
      /* fprintf(stderr, "buffer frames = %d\n", buffer_frames); */

      if ((err = snd_pcm_hw_params (alsa_playback_handle, hw_params)) < 0) {
	 fprintf (stderr, "cannot set parameters (%s)\n",
		  snd_strerror (err));
	 goto nosound;
      }
      
      snd_pcm_hw_params_free (hw_params);
      
      if ((err = snd_pcm_prepare (alsa_playback_handle)) < 0) {
	 fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
		  snd_strerror (err));
	 goto nosound;
      }

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

   sndbufpt = sndbuffer;
#ifdef FRAME_RATE_HACK
   vsynctime = vsynctime * 9 / 10;
#endif	
   return 1;

   nosound:
   return 0;
}

/* this should be called between subsongs when remote slave changes subsong */
void flush_sound (void)
{
   sndbufpt = sndbuffer;
}
