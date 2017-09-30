#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "effects.h"

/* tmp is a host allocated space for variables that should be remembered
   between calls into uade_filter(). The host must preserve the allocated
   memory in tmp! 128 bytes of temporary space should be enough. On the
   first call tempory space must be zeros.
*/
void uade_effect_filter(short *sm, int frames, int stereo, int filtermode,
			void *tmp, int tmpsize)
{
  int *osl, *osr, *ol, *or, *buffer;
  int i, l, r;

  if (!stereo) {
    fprintf(stderr, "uade: mono stream filtering not supported\n");
    return;
  }

  buffer = tmp;
  osl = &buffer[0];
  osr = &buffer[6];
  ol = &buffer[12];
  or = &buffer[18];
  if (tmpsize < ((int) (24*sizeof(int)))) {
    fprintf(stderr, "uade: not enough tmp space for filters\n");
  }

  switch (filtermode) {
    
    /* case 1 and case 2 filters based on tfmxplay's lp filter (IIR filter):
       Y(n) = A * X(n) + B * Y(n-1)/z
       to see frequency response in octave:
       plot(linspace(0,1,256),abs(freqz(A, [1, -B], 256)))
       for example: in case 1: A=3/4 B=1/4
    */
  case 1:
    for (i=0;i<frames;i++) {
      /* case 1 is the least destructive IIR filter */
      ol[0] = (((int) sm[0])*3 + ol[0]) >> 2;
      or[0] = (((int) sm[1])*3 + or[0]) >> 2;
      sm[0] = (short) ol[0];
      sm[1] = (short) or[0];
      sm += 2;
    }
    break;
  case 2:
    for (i=0;i<frames;i++) {
      /* case 2 filter */
      ol[0] = (((int) sm[0]) + ol[0]) >> 1;
      or[0] = (((int) sm[1]) + or[0]) >> 1;
      sm[0] = (short) ol[0];
      sm[1] = (short) or[0];
      sm += 2;
    }
    break;
    
    /* case 3 and 4 mimick amiga led filter frequency response */
    /* A = 1.0000   -0.9433    0.2811
       B = 0.1405    0.0951    0.0644 */
  case 3:
    for (i=0;i<frames;i++) {
      memmove(&osl[1], &osl[0], sizeof(int)*5);
      memmove(&osr[1], &osr[0], sizeof(int)*5);
      osl[0] = (int) sm[0];
      osr[0] = (int) sm[1];
      l = 0.1405*osl[0] + 0.0951*osl[1] + 0.0644*osl[2];
      l += -0.9433*ol[0] + 0.2811*ol[1];
      r = 0.1405*osr[0] + 0.0951*osr[1] + 0.0644*osr[2];
      r += -0.9433*or[0] + 0.2811*or[1];
      l = 0.65*l;
      r = 0.65*r;
      memmove(&ol[1], &ol[0], sizeof(int)*5);
      memmove(&or[1], &or[0], sizeof(int)*5);
      ol[0] = l;
      or[0] = r;
      sm[0] = (short) l;
      sm[1] = (short) r;
      sm += 2;
    }
    break;
    
  case 4:
    /* A = 1.0000   -1.6197    1.2703   -0.5663    0.1349
       B = 0.1289   -0.0030    0.0487    0.0133    0.0202
       for non filtered sound use volume boost of approx. 0.05-0.06
       yulewalk order 4
    */
    for (i=0;i<frames;i++) {
      memmove(&osl[1], &osl[0], sizeof(int)*5);
      memmove(&osr[1], &osr[0], sizeof(int)*5);
      osl[0] = (int) sm[0];
      osr[0] = (int) sm[1];
      l = 0.1289*osl[0] - 0.0030*osl[1] + 0.0487*osl[2] + 0.0133*osl[3] + 0.0202*osl[4];
      l += -1.6197*ol[0] + 1.2703*ol[1] - 0.5663*ol[2] + 0.1349*ol[3];
      r = 0.1289*osr[0] - 0.0030*osr[1] + 0.0487*osr[2] + 0.0133*osr[3] + 0.0202*osr[4];
      r += -1.6197*or[0] + 1.2703*or[1] - 0.5663*or[2] + 0.1349*or[3];
      l /= 4;
      r /= 4;
      memmove(&ol[1], &ol[0], sizeof(int)*5);
      memmove(&or[1], &or[0], sizeof(int)*5);
      ol[0] = l;
      or[0] = r;
      sm[0] = (short) l;
      sm[1] = (short) r;
      sm += 2;
    }
    break;
    
  default:
    /* this is an error but it will have an effect of no filtering */
    break;
  }
}

/* this is b0rken. have support for 8 bit samples also. */
void uade_effect_pan(short *sm, int frames, int bytes_per_sample, float val)
{
  int i, l, r, m;
  int mixpar = (int) (val * (256.0f/2.0f));
  static int did_warn = 0;
  if (bytes_per_sample == 2) {
    for (i = 0; i < frames; i++) {
      l = (int) sm[0];
      r = (int) sm[1];
      m = (r-l) * mixpar;
      sm[0] = (short) ( ((l<<8) + m) >> 8 );
      sm[1] = (short) ( ((r<<8) - m) >> 8 );
      sm += 2;
    }
  } else {
    if (!did_warn) {
      fprintf(stderr, "uade: panning not supported with %d bytes per sample\n", bytes_per_sample);
      did_warn = 1;
    }
  }
}

/* this is b0rken. have support for 8 bit samples also. */
void uade_effect_volume_gain(short *sm, int frames, int nchannels, float val)
{
  int mixpar = (int) (val*256.0f);
  int i, n = frames * nchannels;
  int s;

  if (mixpar == 256)
    return;

  for (i = 0; i < n; i++) {
    s = (((int) sm[i]) * mixpar ) >> 8;
    if (mixpar > 256) {
      if (s > 32767) {
	s = 32767;
      } else if (s < -32768) {
	s = -32768;
      }
    }
    sm[i] = (short) s;
  }
}
