 /*
  * UADE
  *
  * Support for AmigaOS/MorphOS
  * 
  * Copyright 2003 Harry Sintonen
  */

#include <uade-os-amiga.h>
#include <uadeconfig.h>
#include <sys/types.h>
#include <sys/time.h>
#include <dirent.h>
#include <ctype.h>

#include <errno.h>
#include <dos/dos.h>
#include <devices/timer.h>
#include <workbench/startup.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "strlrep.c"

long __stack = 264144; /* 256k stack */

char *uade_base_dir = 0;

static int os_debug = 0;

static int unix_soundfd;
static int unix_usingoutpipe = 0;
void (*sig2handler)(int) = NULL;

/* replace linklib CTRL-C handler */
void __chkabort(void);
void __chkabort(void)
{
  if (SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) {
    if (sig2handler) {
      sig2handler(0);
    }
  }
}

int uade_create_signalhandler(void (*sighandler)(int), int sigtype) {
  if (sigtype == 2)
  {
    sig2handler = sighandler;
    return 1;
  }
  errno = ENOSYS;
  return 0;
}


void uade_create_var_pid(void) {
  errno = ENOSYS;
}

void uade_exit(int status) {
  switch (status) {
    case -1:
    case  1:
      status = RETURN_ERROR; break;
  }

  exit(status);
}

/* first element of args is the executable file */
/* last element of args table has to be a NULL pointer */
pid_t uade_fork_exec(char **args) {
  errno = ENOSYS;
  return -1;
}

#ifdef __amigaos4__
extern struct WBStartup *__WBenchMsg;
#else
extern struct WBStartup *_WBenchMsg;
#endif

static int ami_getprogdir(char *buf, int maxsize)
{
#ifdef __amigaos4__
  if (__WBenchMsg) {
    if (NameFromLock(__WBenchMsg->sm_ArgList[0].wa_Lock, buf, maxsize)) {
#else
  if (_WBenchMsg) {
    if (NameFromLock(_WBenchMsg->sm_ArgList[0].wa_Lock, buf, maxsize)) {
#endif
      return 1;
    }
  }
  else {
    BPTR lock = GetProgramDir();
    if (lock && NameFromLock(lock, buf, maxsize)) {
      return 1;
    }
  }

  return 0;
}


static
UBYTE *DynNameFromLock(BPTR lock)
{
  UBYTE *dirname;
  ULONG dirnamelen = 32;

  for (;;) {
    dirname = AllocVec(dirnamelen + 1, MEMF_PUBLIC);
    if (!dirname)
      break;

    if (NameFromLock(lock, dirname, dirnamelen))
      break;

    FreeVec(dirname); dirname = NULL;

    if (IoErr() != ERROR_LINE_TOO_LONG)
      break;

    dirnamelen <<= 1;
  }

  return dirname;
}

#ifdef __libnix__

/* srandom/random wrappers fox libnix */

void srandom(unsigned x)
{
  srand(x);
}

long random(void)
{
  return rand();
}

#endif


/* minimal & lacking ftw() implementation */

enum
{
  MY_FTW_F,
  MY_FTW_D,
  MY_FTW_DNR,
  MY_FTW_SL,
  MY_FTW_NS
};

int my_ftw(const char *dir,
           int (*fn)(const char *file, const struct stat *sb,
                     int flag),
           int nopenfd);

int my_ftw(const char *dir,
           int (*fn)(const char *file, const struct stat *sb,
                     int flag),
           int nopenfd)
{
  int ret = -1;
  BPTR lock;

  /*Printf("my_ftw: <%s>\n", dir);*/

  lock = Lock(dir, ACCESS_READ);
  if (lock) {
    struct FileInfoBlock *fib;

    fib = AllocMem(sizeof(*fib), MEMF_PUBLIC);
    if (fib) {
      if (Examine(lock, fib)) {
        if (fib->fib_DirEntryType > 0) {
          UBYTE *dirname = DynNameFromLock(lock);
          if (dirname) {
            ULONG namelen = strlen(dirname) + 1 + sizeof(fib->fib_FileName);
            UBYTE *name;
            name = AllocMem(namelen, MEMF_PUBLIC);
            if (name) {
              struct stat st;
              ret = 0;

              while (ExNext(lock, fib)) {
                int flag;

                strcpy(name, dirname);
                AddPart(name, fib->fib_FileName, namelen);

                if (!stat(name, &st)) {
                  switch (fib->fib_EntryType) {
                    case ST_FILE:
                    case ST_LINKFILE: /* hardlink to file */
                    case ST_PIPEFILE:
                      flag = MY_FTW_F; break;
                    case ST_ROOT:
                    case ST_USERDIR:
                    case ST_LINKDIR: /* hardlink to dir */
                      flag = MY_FTW_D; break;
                    case ST_SOFTLINK:
                      flag = MY_FTW_SL; break;
                    default:
                      flag = MY_FTW_NS; break;
                  }
                } else {
                   flag = MY_FTW_NS;
                }

                ret = fn(name, &st, flag);
                if (ret)
                  break;

#warning "TODO: Add to tail of dirlist that is processed after dir is completed!"
                if (flag == MY_FTW_D) {
                  /*Printf("my_ftw: enter dir <%s>\n", name);*/
                  ret = my_ftw(name, fn, nopenfd);
                  if (ret)
                    break;
                }
              }

              FreeMem(name, namelen);
            }

            FreeVec(dirname);
          }
        }
      }

      FreeMem(fib, sizeof(*fib));
    }

    UnLock(lock);
  }

  /*Printf("my_ftw return %ld\n", ret);*/

  return ret;
}


static void *uade_ftw_arg;
static int (*uade_ftw_func) (const char *file, const struct stat *sb, int flag, void *arg);

static int ftw_func(const char *file, const struct stat *sb, int flag) {
  switch (flag) {
  case MY_FTW_F: flag = UADE_FTW_F; break;
  case MY_FTW_D: flag = UADE_FTW_D; break;
  case MY_FTW_DNR: flag = UADE_FTW_DNR; break;
  case MY_FTW_SL: flag = UADE_FTW_SL; break;
  case MY_FTW_NS: flag = UADE_FTW_NS; break;
  default:
    fprintf(stderr, "uade: unknown ftw flag!\n");
    flag = 666;
  }
  return uade_ftw_func(file, sb, flag, uade_ftw_arg);
}

int uade_ftw(const char *dir,
	     int (*fn) (const char *file, const struct stat *sb,
			int flag, void *arg),
	     int depth, void *arg)
{
  uade_ftw_arg = arg;
  uade_ftw_func = fn;
  return my_ftw(dir, ftw_func, depth);
}

int uade_get_path(char *path, int item, int maxlen) {
  char post[1024];
  FILE *testfile;
  DIR *testdir;
  post[0] = 0;
  path[0] = 0;
  if (uade_base_dir) {
    strlcpy(post, uade_base_dir, sizeof(post));
  } else if(!ami_getprogdir(post, sizeof(post))) {
    fprintf(stderr, "uade: warning: can't get PROGDIR. using '' instead\n");
    post[0] = 0;
  }
  switch(item) {
  case UADE_PATH_SCORE:
    if (!AddPart(post, "score", sizeof(post)))
      post[0] = 0;
    break;
  case UADE_PATH_UAERC:
    if (!AddPart(post, "uaerc", sizeof(post)))
      post[0] = 0;
    break;
  case UADE_PATH_UADE:
    if (!AddPart(post, "uade", sizeof(post)))
      post[0] = 0;
    break;
  case UADE_PATH_PLAYERDIR:
    if (!AddPart(post, "players/", sizeof(post))) {
      post[0] = 0;
      break;
    }
    testdir = opendir(post);
    if(testdir) {
      closedir(testdir);
      strlcpy(path, post, maxlen);
      if (AddPart(post, "uadeformats", sizeof(post))) {
	testfile = fopen(post, "r");
	if(testfile) {
	  fclose(testfile); testfile = 0;
	  if(os_debug)
	    fprintf(stderr, "success: path = %s\n", path);
	  return 1;
	}
      }
    }
    fprintf(stderr, "uade: couldn't get uade playerdir path\n");
    return 0;
  case UADE_PATH_FORMATSFILE:
    if (!AddPart(post, "players/uadeformats", sizeof(post)))
      post[0] = 0;
    break;
  default:
    fprintf(stderr, "uade: failed to get path of %d\n", item);
    return 0;
  }
  if(post[0]) {
    testfile = fopen(post, "r");
    if(testfile) {
      fclose(testfile); testfile = 0;
      strlcpy(path, post, maxlen);
      if(os_debug)
	fprintf(stderr, "uade: success: path = %s\n", path);
      return 1;
    }
  }
  fprintf(stderr,"uade: failed to get path of %s\n", post);
  return 0;
}


int uade_get_postfix(char *dst, char *filename, int maxlen) {
  char *baseptr;
  if (maxlen == 0)
    return 0;
  baseptr = strrchr(filename, '/');
  if (baseptr) {
    baseptr++;
  } else {
    baseptr = filename;
  }
  baseptr = strrchr(baseptr, (int) '.');
  if (!baseptr) {
    return 0;
  }
  baseptr++;
  if (((int) strlen(baseptr)) >= maxlen) {
    return 0;
  }
  strcpy(dst, baseptr); /* will not overflow */
  return 1;
}


int uade_get_prefix(char *dst, char *filename, int maxlen) {
  int i, ret;
  char *baseptr;
  if (maxlen == 0)
    return 0;
  baseptr = strrchr(filename, '/');
  if (baseptr) {
    baseptr++;
  } else {
    baseptr = filename;
  }
  if (!strchr(baseptr, (int) '.')) {
    return 0;
  }
  ret = 0;
  for (i=0;;i++) {
    if (i >= maxlen)
      break;
    if (baseptr[i] == '.') {
      ret = 1;
      break;
    }
    dst[i] = baseptr[i];
  }
  dst[i] = 0;
  return ret;
}


int uade_get_temp_name(char *tempname, int maxlen) {
  errno = ENOSYS;
  return -1;
}


int uade_is_regular_file(const char *filename) {
  UBYTE _fib[sizeof(struct FileInfoBlock) + 3];
  struct FileInfoBlock *fib = (void *) (((ULONG) _fib + 3) & ~3);
  BPTR lock;
  int ret = 0;

  lock = Lock(filename, ACCESS_READ);
  if (lock) {
    if (Examine(lock, fib)) {
      if (fib->fib_DirEntryType < 0) {
        ret = 1;
      }
    }
    UnLock(lock);
  }

  return ret;
}

FILE *uade_open_amiga_file(char *aname) {
  char filename[256]; /* special max file name size for amiga os */
  char playerdir[256];
  int maxsize;

  if (strlen(aname) >= sizeof(filename)) {
    fprintf(stderr, "uade: WARNING: amiga tried to open a very long filename\nPlease REPORT THIS!\n");
  }

  if (strchr(aname, (int) ':')) {
    if (!uade_get_path(playerdir, UADE_PATH_PLAYERDIR, sizeof(playerdir))) {
      fprintf(stderr, "uade: open_amiga_file: playerdir not found (%s)\n", aname);
      return 0;
    }

    if(!strncasecmp(aname, "S:", 2)) {
      snprintf(filename, sizeof(filename), "%sS/%s", playerdir, aname + 2);
    } else if (!strncasecmp(aname, "ENV:", 4)) {
      snprintf(filename, sizeof(filename), "%sENV/%s", playerdir, aname + 4);
    } else {
      fprintf(stderr, "uade: open_amiga_file: unknown amiga volume (%s)\n", aname);
      return 0;
    }
  } else {
    strlcpy(filename, aname, sizeof(filename));
  }


  return fopen(filename, "rb");
}


void uade_init_outpipe(char *parameter) {
  unix_usingoutpipe = 1;
  unix_soundfd = atoi(parameter);
}


int uade_send_signal(pid_t pid, int sigtype) {
  errno = ENOSYS;
  return -1;
}

#ifdef __libnix__

/* posix emulation - VERY limited edition */

#define MAXTEMPS 4 /* maximum number of simultanously open tempfiles */
static int atexittemp = 0;
static struct tmpslot {
  int fd;
  char *name;
} tmpslots[MAXTEMPS];

static void mkstemp_clean(void) {
  int i;

  for (i = 0; i < MAXTEMPS; i++) {
    if (tmpslots[i].fd != -1) {
      close(tmpslots[i].fd);
    }
    if (tmpslots[i].name) {
      remove(tmpslots[i].name);
      free(tmpslots[i].name);
    }
  }
}

int mkstemp(char *template) {

  if (template) {
    int i;
    int len;

    if (!atexittemp) {
      atexittemp = 1;
      for (i = 0; i < MAXTEMPS; i++) {
        tmpslots[i].fd   = -1;
        tmpslots[i].name = NULL;
      }
      if (atexit(mkstemp_clean) != 0) {
        atexittemp = 0;
        errno = ENOMEM;
        return -1;
      }
    }

    len = strlen(template);
    if (len >= 12 && memcmp(template + len - 6, "XXXXXX", 6) == 0) {
      int id = (int) FindTask(NULL) & 0xffffff;
      int fd = -1;
      /* a bit crude, try to open every file until find a free one */
      for (i = 0; fd == -1 && i < MAXTEMPS; i++) {
        sprintf(template, "T:ua%06x%02u", id, i);
        fd = open(template, O_TRUNC | O_RDWR | O_CREAT);
      }
      if (fd != -1 && i < MAXTEMPS) {
        tmpslots[i].fd = fd;
        if (tmpslots[i].name) {
          free(tmpslots[i].name);
        }
        tmpslots[i].name = strdup(template);

        return fd;
      }
    }
    else {
      errno = EINVAL;
    }
  }
  return -1;
}

FILE *popen(const char *command, const char *type) {

#warning FIXME: Use fopen with APIPE: ?

  errno = ENOSYS;
  return NULL;
}

int pclose(FILE *stream) {
  errno = ENOSYS;
  return -1;
}

#endif /* __libnix__ */


static int ami_timerinit = 0;
static struct timerequest *ami_tr;
static int ami_timerdev = -1;

static void ami_timercleanup(void) {
  if (ami_timerdev == 0) {
    ami_tr->tr_node.io_Message.mn_ReplyPort = NULL;
    CloseDevice(&ami_tr->tr_node);
    ami_timerdev = -1;
  }
  if (ami_tr) {
    DeleteIORequest(&ami_tr->tr_node);
    ami_tr = NULL;
  }
}

static void ami_sleep(ULONG unit, struct timeval *tv) {
  struct MsgPort replyport;
  ULONG sigs;

  replyport.mp_Node.ln_Type = NT_MSGPORT;
  replyport.mp_Flags        = PA_SIGNAL;
  replyport.mp_SigBit       = SIGB_SINGLE;
  replyport.mp_SigTask      = FindTask(NULL);
  NewList(&replyport.mp_MsgList);

  if (!ami_timerinit) {
    ami_timerinit = 1;

    do {
      if (atexit(ami_timercleanup)) {
        ami_tr = CreateIORequest(&replyport, sizeof(*ami_tr));
        if (ami_tr) {
          ami_timerdev = OpenDevice(TIMERNAME, unit, &ami_tr->tr_node, 0);
          if (ami_timerdev == 0) {
            // all ok
            break;
          }
        }
      }
      exit(-1);

    } while (0);
  }

  ami_tr->tr_node.io_Message.mn_ReplyPort = &replyport;
  ami_tr->tr_node.io_Command = TR_ADDREQUEST;
  ami_tr->tr_time = *tv;
  SetSignal(0, SIGF_SINGLE);
  SendIO(&ami_tr->tr_node);

  sigs = Wait(SIGBREAKF_CTRL_C | SIGF_SINGLE);
  if (!(sigs & SIGF_SINGLE)) {
    AbortIO(&ami_tr->tr_node);
  }
  WaitIO(&ami_tr->tr_node);

  if (sigs & SIGBREAKF_CTRL_C) {
    if (sig2handler) {
      sig2handler(0);
    }
  }
}

void uade_sleep(int seconds) {
  struct timeval tv = {seconds, 0};
  ami_sleep(UNIT_VBLANK, &tv);
}


void uade_usleep(int microseconds) {
  struct timeval tv = {microseconds / 1000000, microseconds % 1000000};
  ami_sleep(UNIT_MICROHZ, &tv);
}


/* returns number of milliseconds from the last call
   exception: on the first time this returns zero    */
int uade_timer_poll(void) {
  errno = ENOSYS;
  return 0;
}

char *uade_version(void) {
  return strdup(UADEVERSIONSTRING);
}

void uade_write_to_outpipe(void *ptr, int size) {
  if(unix_usingoutpipe) {
    write(unix_soundfd, ptr, size);
  }
}

