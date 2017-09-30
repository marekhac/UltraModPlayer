 /* 
  * UAE - The Un*x Amiga Emulator
  * 
  * Support for Linux/USS sound
  * 
  * Copyright 1997 Bernd Schmidt
  */

#include <unistd.h>
#include <errno.h>
#include "uade.h"
//#include "uade-os.h"
//#include "effects.h"

extern int sound_fd;
extern uae_u16 sndbuffer[];
extern uae_u16 *sndbufpt;
extern int sndbufsize;
extern int sound_bytes_per_sample;
extern void finish_sound_buffer (void);

static __inline__ void check_sound_buffers (void) {

  if ((char *)sndbufpt - (char *)sndbuffer >= sndbufsize) {

    if ((char *)sndbufpt - (char *)sndbuffer > sndbufsize) {
      fprintf(stderr, "uade: A bug in sound buffer writing. Report this!\n");
    }

    if (uade_check_sound_buffers(sndbuffer, sndbufsize, sound_bytes_per_sample)) {
      finish_sound_buffer();
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
