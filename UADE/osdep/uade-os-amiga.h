#ifndef _UADE_OS_AMIGA_H_
#define _UADE_OS_AMIGA_H_
 /*
  * UADE
  *
  * Support for AmigaOS/MorphOS
  * 
  * Copyright 2003 Harry Sintonen
  */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include "uademsg.h"

#ifndef __amigaos4__
#include <exec/lists.h>
#include <proto/alib.h>
#endif
#ifdef __MORPHOS__
#define NewList(a) NEWLIST(a)
#endif

#ifdef __amigaos4__
#define random(x) rand(x)
#define srandom(x) srand(x)
#endif

#define UADE_SIGALIVE (0)
#define UADE_SIGHUP (1)
#define UADE_SIGINT (2)

#define UADE_PATH_SCORE       (1)
#define UADE_PATH_UAERC       (2)
#define UADE_PATH_UADE        (3)
#define UADE_PATH_PLAYERDIR   (4)
#define UADE_PATH_FORMATSFILE (5)

#define UADE_FTW_F (1)
#define UADE_FTW_D (2)
#define UADE_FTW_DNR (3)
#define UADE_FTW_SL (4)
#define UADE_FTW_NS (5)

#define UADE_PATH_MAX (1024)

char *uade_base_dir;

#define UADE_SIGNALHANDLER(x) void x(int signum)

int uade_create_signalhandler(void (*sighandler)(int), int sigtype);

void uade_create_var_pid(void);

pid_t uade_fork_exec(char **args);

int uade_ftw(const char *dir, int (*fn) (const char *file, const struct stat *sb, int flag, void *arg), int depth, void *arg);

int uade_get_path(char *path, int item, int maxlen);

int uade_get_postfix(char *dst, char *filename, int maxlen); 
int uade_get_prefix(char *dst, char *filename, int maxlen); 

int uade_get_temp_name(char *tempname, int maxlen);

int uade_is_regular_file(const char *filename);

void uade_init_outpipe(char *parameter);

FILE *uade_open_amiga_file(char *amigafilename);

int uade_send_signal(pid_t pid, int sigtype);

void uade_sleep(int seconds);

void uade_usleep(int microseconds);

int uade_timer_poll(void);

char *uade_version(void);

void uade_write_to_outpipe(void *ptr, int size);

void uade_exit(int status);

#endif
