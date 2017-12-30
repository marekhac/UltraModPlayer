#include <stdio.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>

#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include "stdio.h"

#include <proto/dos.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>

void play(void);
void stopProc(void);
void startNewProc(STRPTR);
extern struct Process *childprocess;