#ifndef __UADE_EFFECTS_H_
#define __UADE_EFFECTS_H_

void uade_effect_filter(short *sm, int samples, int stereo, int filtermode, void *tmp, int tmpsize);
void uade_effect_pan(short *sm, int samples, int bytes_per_sample, float val);
void uade_effect_volume_gain(short *sm, int samples, int nchannels, float val);

#endif
