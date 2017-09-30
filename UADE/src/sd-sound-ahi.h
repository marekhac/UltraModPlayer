 /* 
  * UAE - The Un*x Amiga Emulator
  * 
  * Support for AHI AmigaOS/MorphOS
  * 
  * Copyright 2003 Harry Sintonen
  */

#include <unistd.h>
#include "uade.h"
#include "uade-os.h"

#include <dos/dos.h>
#include <devices/ahi.h>
#include <proto/exec.h>

extern int sound_bytes_per_sample;
extern ULONG ahimsgportmask;
extern ULONG activereq;
extern struct AHIRequest *ahireq[2];
extern struct AHIRequest *prevahireq;
extern ULONG sampletype;
extern ULONG samplerate;
extern uae_u16 sndbuffer0[];
extern uae_u16 sndbuffer1[];
extern uae_u16 *sndbuffer;
extern uae_u16 *sndbufpt;
extern int sndbufsize;
extern void finish_sound_buffer (void);
extern void (*sig2handler)(int);

static __inline__ void check_sound_buffers (void) {

  if ((char *)sndbufpt - (char *)sndbuffer >= sndbufsize) {

    if ((char *) sndbufpt - (char *) sndbuffer > sndbufsize) {
      fprintf(stderr, "uade: A bug in sound buffer writing. Report this!\n");
    }

    if (uade_check_sound_buffers(sndbuffer, sndbufsize, sound_bytes_per_sample)) {
      ahireq[activereq]->ahir_Std.io_Data    = sndbuffer;
      ahireq[activereq]->ahir_Std.io_Length  = sndbufsize;
      ahireq[activereq]->ahir_Std.io_Offset  = 0;
      ahireq[activereq]->ahir_Type           = sampletype;
      ahireq[activereq]->ahir_Frequency      = samplerate;
      ahireq[activereq]->ahir_Volume         = 0x10000;    /* full volume */
      ahireq[activereq]->ahir_Position       = 0x8000;     /* centered */
      ahireq[activereq]->ahir_Link           = prevahireq;
      SendIO((struct IORequest *) ahireq[activereq]);

      /* wait completion of the previous req */
      if (prevahireq) {
        ULONG signals = Wait(ahimsgportmask | SIGBREAKF_CTRL_C);
        if (!(signals & ahimsgportmask)) {
          AbortIO((struct IORequest *) prevahireq);
        }
        WaitIO((struct IORequest *) prevahireq);
        if (signals & SIGBREAKF_CTRL_C) {
          if (sig2handler) {
            sig2handler(0);
          }
        }
      }
      prevahireq = ahireq[activereq];
      activereq ^= 1;
      sndbuffer = activereq ? sndbuffer1 : sndbuffer0;
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


