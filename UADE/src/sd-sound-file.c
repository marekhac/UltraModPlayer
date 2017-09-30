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
#include "audio.h"
#include "gensound.h"
#include "sd-sound.h"

#include <sys/ioctl.h>
//#include <sys/soundcard.h>

#include "uade.h"

int sound_fd;
static int have_sound;
static unsigned long formats;

unsigned long sndbuf_written;

uae_u16 sndbuffer[44100];
uae_u16 *sndbufpt;
int sndbufsize;

int sound_bytes_per_sample;

static int exact_log2(int v)
{
    int l = 0;
    while ((v >>= 1) != 0)
	l++;
    return l;
}

void close_sound(void)
{
    int t;
    uae_u32 v;
    char buf[4];
    
    if (!have_sound)
	return;
    
    t = 0;
    v = sndbuf_written;
    buf[t] = v & 255;
    buf[t+1] = (v>>8) & 255;
    buf[t+2] = (v>>16) & 255;
    buf[t+3] = (v>>24) & 255;
    lseek (sound_fd, 40, SEEK_SET);
    write (sound_fd, buf, 4);

    v += 36;
    buf[t] = v & 255;
    buf[t+1] = (v>>8) & 255;
    buf[t+2] = (v>>16) & 255;
    buf[t+3] = (v>>24) & 255;
    lseek (sound_fd, 4, SEEK_SET);
    write (sound_fd, buf, 4);

    close(sound_fd);
}

int setup_sound(void)
{
    sound_fd = open ("sound.output.wav", O_CREAT|O_TRUNC|O_WRONLY, 0666);
    have_sound = !(sound_fd < 0);
    if (!have_sound)
	return 0;

    sound_available = 1;
    return 1;
}

int init_sound (void)
{
    int t;
    uae_u32 v;
    int tmp;
    char buf[200] = "RIFF    WAVEfmt                     data    ";

    uae_u16 endian = 0x1234;
    uae_u8 *ptr = (uae_u8 *) &endian;
    if (ptr[0] == 0x12) {
      if (currprefs.sound_bits == 16) {
	uade_swap_output_bytes = uade_swap_output_bytes ? 0 : 1;
	fprintf(stderr, "uade: big endian platform: swapping output endianess to %s endian\n", uade_swap_output_bytes ? "little" : "big");
      }
    }

    sound_bytes_per_sample = currprefs.sound_bits / 8;

    /* Prepare a .WAV header */
    sndbuf_written = 44;
    t = 16; v = 16;
    buf[t] = v & 255;
    buf[t+1] = (v>>8) & 255;
    buf[t+2] = (v>>16) & 255;
    buf[t+3] = (v>>24) & 255;

    t = 20; v = 0x00010001 + (currprefs.stereo ? 0x10000 : 0);
    buf[t] = v & 255;
    buf[t+1] = (v>>8) & 255;
    buf[t+2] = (v>>16) & 255;
    buf[t+3] = (v>>24) & 255;

    t = 24; v = currprefs.sound_freq;
    buf[t] = v & 255;
    buf[t+1] = (v>>8) & 255;
    buf[t+2] = (v>>16) & 255;
    buf[t+3] = (v>>24) & 255;
    t = 32; v = ((currprefs.sound_bits == 8 ? 1 : 2)
		 * (currprefs.stereo ? 2 : 1)) + 65536*currprefs.sound_bits;
    buf[t] = v & 255;
    buf[t+1] = (v>>8) & 255;
    buf[t+2] = (v>>16) & 255;
    buf[t+3] = (v>>24) & 255;
    t = 28; v = (currprefs.sound_freq * (currprefs.sound_bits == 8 ? 1 : 2)
		 * (currprefs.stereo ? 2 : 1));
    buf[t] = v & 255;
    buf[t+1] = (v>>8) & 255;
    buf[t+2] = (v>>16) & 255;
    buf[t+3] = (v>>24) & 255;
    write (sound_fd, buf, 44);

    sample_evtime = (long)maxhpos * maxvpos * 50 / currprefs.sound_freq;

    if (currprefs.sound_bits == 16) {
	init_sound_table16 ();
	sample_handler = currprefs.stereo ? sample16s_handler : sample16_handler;
    } else {
	init_sound_table8 ();
	sample_handler = currprefs.stereo ? sample8s_handler : sample8_handler;
    }
    sound_available = 1;
    sndbufsize = 44100;
    fprintf (stderr, "Writing sound into \"sound.output.wav\"; %d bits at %d Hz\n",
	     currprefs.sound_bits, currprefs.sound_freq, sndbufsize);
    sndbufpt = sndbuffer;
    return 1;
}
