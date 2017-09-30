 /* 
  * UAE - The Un*x Amiga Emulator
  * 
  * Support for Mac OS X CoreAudio
  * 
  * (C) 2004-2005 Stuart Caie
  */

#include <unistd.h>
#include "uade.h"
#include "uade-os.h"
#include "effects.h"
#include "xmms-slave.h"

extern uae_u16 *sndbuffer, *sndbufpt;
extern int sndbufsize;
void finish_sound_buffer();

static __inline__ void check_sound_buffers(void) {
  int bytes = (char *)sndbufpt - (char *)sndbuffer;
  if (bytes >= sndbufsize) {
    if (bytes > sndbufsize) {
      fprintf(stderr, "uade: A bug in sound buffer writing. Report this!\n");
    }

    if (uade_check_sound_buffers(sndbuffer, sndbufsize, 2)) {
      finish_sound_buffer();
    }
    sndbufpt = sndbuffer;
  }
}

#define PUT_SOUND_BYTE(b) ( *sndbufpt++ = ((b) << 8) )
#define PUT_SOUND_WORD(b) ( *sndbufpt++ = (b) )
#define PUT_SOUND_BYTE_LEFT(b) PUT_SOUND_BYTE(b)
#define PUT_SOUND_WORD_LEFT(b) PUT_SOUND_WORD(b)
#define PUT_SOUND_BYTE_RIGHT(b) PUT_SOUND_BYTE(b)
#define PUT_SOUND_WORD_RIGHT(b) PUT_SOUND_WORD(b)
#define SOUND16_BASE_VAL 0
#define SOUND8_BASE_VAL 0

#define DEFAULT_SOUND_MINB 8192
#define DEFAULT_SOUND_MAXB 8192
#define DEFAULT_SOUND_BITS 16
#define DEFAULT_SOUND_FREQ 44100
#define HAVE_STEREO_SUPPORT
