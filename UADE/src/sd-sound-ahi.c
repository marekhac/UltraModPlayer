 /* 
  * UAE - The Un*x Amiga Emulator
  * 
  * Support for AHI AmigaOS/MorphOS
  * 
  * Copyright 2003 Harry Sintonen
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
#include "uade-os.h"

#include <devices/ahi.h>
#include <proto/exec.h>


#define SNDBUFSIZE	22050


ULONG sampletype;
ULONG samplerate;

uae_u16 sndbuffer0[SNDBUFSIZE];
uae_u16 sndbuffer1[SNDBUFSIZE];
uae_u16 *sndbuffer;
uae_u16 *sndbufpt;
int sndbufsize;

int sound_bytes_per_sample;

static int added_atexit = 0;
static struct MsgPort ahimsgport = {{NULL,NULL,0,0,NULL}, 0, -1, NULL, {NULL,NULL,NULL,0,0}};
ULONG ahimsgportmask;
ULONG activereq = 0;
struct AHIRequest *ahireq[2] = { NULL, NULL };
struct AHIRequest *prevahireq = NULL;


void close_sound (void) {
  if (prevahireq) {
    AbortIO((struct IORequest *) prevahireq);
    WaitIO((struct IORequest *) prevahireq);
    prevahireq = NULL;
  }
  if (ahireq[1]) {
    FreeMem(ahireq[1], sizeof(struct AHIRequest));
    ahireq[1] = NULL;
  }
  if (ahireq[0]) {
    if (ahireq[0]->ahir_Std.io_Device) {
      CloseDevice((struct IORequest *) ahireq[0]);
      ahireq[0]->ahir_Std.io_Device = NULL;
    }
    DeleteIORequest((struct IORequest *) ahireq[0]);
    ahireq[0] = NULL;
  }
  if (ahimsgport.mp_SigBit != (UBYTE) -1) {
    FreeSignal(ahimsgport.mp_SigBit);
    ahimsgport.mp_SigBit = -1;
    ahimsgportmask = 0;
  }
}

/* Try to determine whether sound is available.  This is only for GUI purposes.  */
int setup_sound (void) {

  /* We can hope, can't we ;) */
  sound_available = 1;
  return 1;
}


int init_sound (void) {
  int stereo;
  int dspbits;

  dspbits    = currprefs.sound_bits;
  stereo     = currprefs.stereo;
  samplerate = currprefs.sound_freq;

  sound_bytes_per_sample = dspbits / 8;

  switch (dspbits) {
    case 8:
      sampletype = stereo ? AHIST_S8S : AHIST_M8S;
      break;

    case 16:
      sampletype = stereo ? AHIST_S16S : AHIST_M16S;
      break;

    default:
      fprintf(stderr, "init_sound: unknown dspbits %d\n", dspbits);
      return 0;
  }

  if (uade_local_sound) {

    /* add cleanup routine, but only once */
    if (!added_atexit) {
      added_atexit = 1;
      atexit(close_sound);
    }

    ahimsgport.mp_SigBit = AllocSignal(-1);
    if (ahimsgport.mp_SigBit == (UBYTE) -1) {
      fprintf(stderr, "init_sound: no free signal...\n");
      return 0;
    }
    ahimsgport.mp_Node.ln_Type = NT_MSGPORT;
    ahimsgport.mp_Flags        = PA_SIGNAL;
    ahimsgport.mp_SigTask      = FindTask(NULL);
    NewList(&ahimsgport.mp_MsgList);

    ahimsgportmask = 1UL << ahimsgport.mp_SigBit;

    ahireq[0] = (struct AHIRequest *) CreateIORequest(&ahimsgport, sizeof(struct AHIRequest));
    if (!ahireq[0]) {
      fprintf(stderr, "init_sound: no memory for ahireq #0...\n");
      FreeSignal(ahimsgport.mp_SigBit);
      ahimsgport.mp_SigBit = -1;
      return 0;
    }
    ahireq[0]->ahir_Version = 4;
    if (OpenDevice(AHINAME, AHI_DEFAULT_UNIT, (struct IORequest *) ahireq[0], 0) != AHIE_OK) {
      fprintf(stderr, "init_sound: could not open ahi.device V4...\n");
      DeleteIORequest((struct IORequest *) ahireq[0]);
      ahireq[0] = NULL;
      FreeSignal(ahimsgport.mp_SigBit);
      ahimsgport.mp_SigBit = -1;
      return 0;
    }
    ahireq[1] = (struct AHIRequest *) AllocMem(sizeof(struct AHIRequest), MEMF_PUBLIC);
    if (!ahireq[1]) {
      fprintf(stderr, "init_sound: no memory for ahireq #1...\n");
      CloseDevice((struct IORequest *) ahireq[0]);
      DeleteIORequest((struct IORequest *) ahireq[0]);
      ahireq[0] = NULL;
      FreeSignal(ahimsgport.mp_SigBit);
      ahimsgport.mp_SigBit = -1;
      return 0;
    }

    /* set static data */
    ahireq[0]->ahir_Std.io_Command = CMD_WRITE;

    /* make a copy of the request (for double buffering) */
    CopyMem(ahireq[0], ahireq[1], sizeof(struct AHIRequest));

    sndbufsize = SNDBUFSIZE * sizeof(uae_u16);

  } else {
    sndbufsize = 4096;
  }

  sample_evtime = (long)maxhpos * maxvpos * 50 / samplerate;

  if (dspbits == 16) {
    init_sound_table16();
    sample_handler = stereo ? sample16s_handler : sample16_handler;
  } else {
    init_sound_table8();
    sample_handler = stereo ? sample8s_handler : sample8_handler;
  }

  sound_available = 1;

  sndbuffer = sndbufpt = sndbuffer0;

#ifdef FRAME_RATE_HACK
  vsynctime = vsynctime * 9 / 10;
#endif

  return 1;
}

void flush_sound(void)
{
  sndbufpt = sndbuffer;
}
