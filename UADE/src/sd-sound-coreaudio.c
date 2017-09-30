/* 
  * UAE - The Un*x Amiga Emulator
  * 
  * Support for Mac OS X CoreAudio
  * 
  * (C) 2004-2005 Stuart Caie
  * based on the Fink esound patch by Shawn Hsiao and Masanori Sekino
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "config.h"
#include "options.h"
#include "memory.h"
#include "custom.h"
#include "audio.h"
#include "gensound.h"

#include <CoreAudio/AudioHardware.h>
#include <pthread.h>
#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <sys/param.h>
#include <sys/sysctl.h>

#include "uade.h"

/* the UAE soundbuffer */
uae_u16 *sndbuffer = NULL, *sndbufpt;
int sndbufsize = 0;

static AudioDeviceID outputDevice;
static Float32 *outputBuf = NULL;
static int outputAvail = 0, outputSize = 0;
static pthread_mutex_t outputMutex;
static pthread_cond_t outputCond;


static OSStatus sound_callback(AudioDeviceID inDevice,
  const AudioTimeStamp *inNow, const AudioBufferList *inInputData,
  const AudioTimeStamp *inInputTime, AudioBufferList *outOutputData,
  const AudioTimeStamp *inOutputTime, void *inClientData)
{
  Float32 *buf = outOutputData->mBuffers[0].mData;
  pthread_mutex_lock(&outputMutex);
  memcpy(&buf[0], &outputBuf[0], outputAvail * sizeof(Float32));
  outputAvail = 0;
  pthread_mutex_unlock(&outputMutex);
  pthread_cond_signal(&outputCond);
  return kAudioHardwareNoError;
}

void finish_sound_buffer (void) {
  /* convert data to floats and put into buffer */
  uae_s16 *sndp = sndbuffer;
  Float32 scale = 1.0 / 32768.0;
  int i, type;

  pthread_mutex_lock(&outputMutex);

  /* wait for existing buffer data to be consumed */
  while (outputAvail) pthread_cond_wait(&outputCond, &outputMutex);

  /* refill buffer */
  if (currprefs.stereo) {
    /* converting from 16 bit signed ints to normalised floats */
    for (i = 0; i < outputSize; i++) {
      outputBuf[i] = *sndp++ * scale;
    }
  }
  else {
    /* as above, but write each mono sample twice for stereo */
    for (i = 0; i < outputSize; i += 2) {
      outputBuf[i] = outputBuf[i+1] = *sndp++ * scale;
    }
  }
  outputAvail = outputSize;
  pthread_mutex_unlock(&outputMutex);
}

/* dump any sound currently in the buffer */
void flush_sound (void) {
  sndbufpt = sndbuffer;
}

/* initialise sound device */
int init_sound(void) {
  struct thread_time_constraint_policy ttcpolicy;
  int bus_speed, mib[2] = { CTL_HW, HW_BUS_FREQ };
  AudioValueRange frameRange;
  UInt32 size, frames;

  /* get realtime priority */
  size = sizeof(bus_speed);
  sysctl(mib, 2, &bus_speed, &size, NULL, 0);
  ttcpolicy.period      = bus_speed / 120;
  ttcpolicy.computation = bus_speed / 2400;
  ttcpolicy.constraint  = bus_speed / 1200;
  ttcpolicy.preemptible = 1;
  thread_policy_set(mach_thread_self(), THREAD_TIME_CONSTRAINT_POLICY,
		    (int *) &ttcpolicy, THREAD_TIME_CONSTRAINT_POLICY_COUNT);

  /* get default output device ID */
  size = sizeof(outputDevice);
  if (AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice,
			       &size, &outputDevice)) return 0;

  /* get minimum and maximum frame sizes */
  size = sizeof(frameRange);
  if (AudioDeviceGetProperty(outputDevice, 0, 0,
                             kAudioDevicePropertyBufferFrameSizeRange,
                             &size, &frameRange)) return 0;

  /* Assume the user's buffer size preference is already 16 bit stereo, so
   * divide by 4 to get the number of frames (a frame is one sample, regardless
   * of bit width or number of channels) */
  frames = currprefs.sound_maxbsiz / 4;
  if (frames < (UInt32) frameRange.mMinimum) {
    frames = (UInt32) frameRange.mMinimum;
  }
  if (frames > (UInt32) frameRange.mMaximum) {
    frames = (UInt32) frameRange.mMaximum;
  }

  /* set hardware audio buffer size */
  if (AudioDeviceSetProperty(outputDevice, 0, 0, 0,
			     kAudioDevicePropertyBufferFrameSize,
			     sizeof(frames), &frames)) return 0;

  /* outputSize is the number of samples in our conversion buffer. This is
   * always twice the number of frames, as we are always generating stereo */
  outputSize = frames * 2;

  if (currprefs.sound_bits == 16) {
    init_sound_table16();
    sample_handler = currprefs.stereo ? sample16s_handler : sample16_handler;
  }
  else {
    init_sound_table8();
    sample_handler = currprefs.stereo ? sample8s_handler : sample8_handler;
  }
  sndbufsize = frames * sizeof (uae_s16) * (currprefs.stereo ? 2 : 1);
  sample_evtime = (long) (maxhpos * maxvpos * 50) / currprefs.sound_freq;

  if (!(sndbuffer = malloc(sndbufsize))) return 0;
  if (!(outputBuf = malloc(outputSize * sizeof(Float32)))) return 0;
  if (pthread_mutex_init(&outputMutex, NULL)) return 0;
  if (pthread_cond_init(&outputCond, NULL)) return 0;
  if (AudioDeviceAddIOProc(outputDevice, &sound_callback, NULL)) return 0;
  if (AudioDeviceStart(outputDevice, &sound_callback)) return 0;
  sndbufpt = sndbuffer;
  return sound_available = 1;
}

/* uninitialise sound device */
void close_sound() {
  AudioDeviceStop(outputDevice, &sound_callback);
  AudioDeviceRemoveIOProc(outputDevice, &sound_callback);
  pthread_cond_destroy(&outputCond);
  pthread_mutex_destroy(&outputMutex);
  free(outputBuf); outputBuf = NULL;
  free(sndbuffer); sndbuffer = NULL;
}

/* determine whether sound is available */
int setup_sound() {
  AudioStreamBasicDescription format;
  UInt32 size;

  if (!uade_local_sound) return sound_available = 1;

  /* get default output device ID */
  size = sizeof(outputDevice);
  if (AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice,
			       &size, &outputDevice)) return 0;

  /* get properties of default output device default channel stream */
  size = sizeof(format);
  if (AudioDeviceGetProperty(outputDevice, 0, 0,
			     kAudioDevicePropertyStreamFormat,
			     &size, &format)) return 0;

  /* We need an output device that takes lpcm float samples, and it has to
   * be at the sample rate of our output already, as we don't resample. */
  return sound_available =
    ( (format.mFormatID == kAudioFormatLinearPCM) &&
      (format.mFormatFlags & kLinearPCMFormatFlagIsFloat) &&
      (currprefs.sound_freq == ((int) format.mSampleRate)) &&
      (format.mChannelsPerFrame == 2) );
}
