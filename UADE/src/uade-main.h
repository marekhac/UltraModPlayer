#ifndef UADE_MAIN_H__
#define UADE_MAIN_H__

#include "uade-main.h"
#include "sysconfig.h"
#include "sysdeps.h"
#include <assert.h>

#include "config.h"
#include "options.h"
#include "uae.h"
#include "gensound.h"
#include "sd-sound.h"
#include "events.h"
#include "memory.h"
#include "custom.h"
#include "readcpu.h"
#include "newcpu.h"
#include "debug.h"
#include "osemu.h"
#include "compiler.h"

#include "uade.h"
#include "uadeconfig.h"
#include "uade-os.h"
#include "../osdep/strl.c"

extern void fix_options (void);

#endif  // UADE_MAIN_H__