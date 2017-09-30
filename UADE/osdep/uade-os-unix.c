#include "../config.h"

#include <uade-os-unix.h>
#include <uadeconfig.h>
#include <sys/types.h>
#include <sys/time.h>
#include <dirent.h>
#include <libgen.h>
#include <ctype.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <limits.h>

#if !defined(HAVE_MACOSX) && !defined(__FreeBSD__)
#include <ftw.h>
#endif

#include "strl.c"

char *uade_base_dir = 0;

static int unix_soundfd;
static int unix_usingoutpipe = 0;

int uade_create_signalhandler(void (*sighandler)(int), int sigtype) {
  int signum;
  struct sigaction *sigact;
  int ret;
  switch (sigtype) {
  case UADE_SIGHUP:
    signum = SIGHUP;
    break;
  case UADE_SIGINT:
    signum = SIGINT;
    break;
  default:
    fprintf(stderr,"%s/uade: uade_create_sighandler: no such signal\n", UADE_MODULE);
    return 0;
  }
  if (!(sigact = calloc(1, sizeof(struct sigaction)))) {
    return 0;
  }
  sigact->sa_handler = sighandler;
  ret = sigaction(signum, sigact, 0);
  free(sigact);
  return ret ? 0 : 1;
}


void uade_create_var_pid(void) {
  /* ADDED BY XIGH */
  int fd;
  char pidstr[10];
  char varname[UADE_PATH_MAX];
  char *user;
  if (!(user = getenv("USER"))) {
    fprintf(stderr, "uade: error: $USER not defined\n");
    return;
  }
  snprintf(varname, sizeof(varname), "/var/run/uade.%s.pid", user);
  sprintf(pidstr, "%d", (int) getpid());
  fd = open(varname, O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
    fprintf(stderr, "uade: could not create %s\n", varname);
  } else {
    write(fd, pidstr, strlen(pidstr));
    close(fd);
  }
}

void uade_exit(int code) {
  exit(code);
}

/* first element of args is the executable file */
/* last element of args table has to be a NULL pointer */
pid_t uade_fork_exec(char **args) {
  pid_t slavepid = fork();
  if (slavepid == 0) {
    execv(args[0], args);
    fprintf(stderr, "uade: OH SHIT! execv() has returned!\nPlease report this bug\n");
    fprintf(stderr, "uade: DON'T try to play with UADE now\n");
    abort();
  } else if (slavepid < 0) {
    fprintf(stderr, "uade: fork error\n");
    slavepid = 0;
  }
  return slavepid;
}

static void *uade_ftw_arg;
static int (*uade_ftw_func) (const char *file, const struct stat *sb, int flag, void *arg);

static int ftw_func(const char *file, const struct stat *sb, int flag) {
#if !defined(HAVE_MACOSX) && !defined(__FreeBSD__)
  switch (flag) {
  case FTW_F: flag = UADE_FTW_F; break;
  case FTW_D: flag = UADE_FTW_D; break;
  case FTW_DNR: flag = UADE_FTW_DNR; break;
  case FTW_SL: flag = UADE_FTW_SL; break;
  case FTW_NS: flag = UADE_FTW_NS; break;
  default:
    fprintf(stderr, "uade: unknown ftw flag!\n");
    flag = 666;
  }
#else
  fprintf(stderr, "uade: unknown ftw flag!\n");
  return 1;
#endif
  return uade_ftw_func(file, sb, flag, uade_ftw_arg);
}

/* WARNING: ftw() is not thread safe if 'arg' is used */
int uade_ftw(const char *dir,
	     int (*fn) (const char *file, const struct stat *sb,
			int flag, void *arg),
	     int depth, void *arg)
{
  uade_ftw_arg = arg;
  uade_ftw_func = fn;
#if !defined(HAVE_MACOSX) && !defined(__FreeBSD__)
  return ftw(dir, ftw_func, depth);
#else
  fprintf(stderr, "uade: error! macosx & FreeBSD doesn't have uade_ftw() implemented\n");
  return 0;
#endif
}

/* tries to get path of an uade component. first tries home directory, and
   then a global installation directory (or home directory again in case
   the configure was given --user) */
int uade_get_path(char *path, int item, int maxlen) {
  char post[UADE_PATH_MAX];
  char glob[UADE_PATH_MAX];
  char temp[UADE_PATH_MAX];
  char home[UADE_PATH_MAX];
  DIR *testdir;
  int amode;
  char *env;
  char *base_dir = uade_base_dir ? uade_base_dir : UADECONFIG_DATADIR;

  post[0] = glob[0] = temp[0] = path[0] = 0;
  if ((env = getenv("HOME"))) {
    strlcpy(home, env, sizeof(home));
  } else {
    strlcpy(home, ".", sizeof(home));
    fprintf(stderr, "uade: warning: $HOME not defined. using '.' instead\n");
  }
  switch (item) {
  case UADE_PATH_SCORE:
    strlcpy(post, "score", sizeof(post));
    snprintf(glob, sizeof(glob), "%s/score", base_dir);
    amode = R_OK;
    break;
  case UADE_PATH_UAERC:
    strlcpy(post, "uaerc", sizeof(post));
    snprintf(glob, sizeof(glob), "%s/uaerc", base_dir);
    amode = R_OK;
    break;
  case UADE_PATH_UADE:
    strlcpy(post, "uade", sizeof(glob));
    strlcpy(glob, UADECONFIG_UADE, sizeof(glob));
    amode = X_OK;
    break;
  case UADE_PATH_PLAYERDIR:
    snprintf(glob, sizeof(glob), "%s/.uade/players/", home);
    if ((testdir = opendir(glob))) {
      closedir(testdir);
      strlcpy(path, glob, maxlen);
      return 1;
    }
    snprintf(glob, sizeof(glob), "%s/players/", base_dir);
    if ((testdir = opendir(glob))) {
      closedir(testdir);
      strlcpy(path, glob, maxlen);
      return 1;
    }
    fprintf(stderr, "uade: couldn't get uade playerdir path\n");
    return 0;
  case UADE_PATH_FORMATSFILE:
    strlcpy(post, "players/uadeformats", sizeof(post));
    snprintf(glob, sizeof(glob), "%s/players/uadeformats", base_dir);
    amode = R_OK;
    break;
  default:
    fprintf(stderr, "uade: failed to get path of %d\n", item);
    return 0;
  }

  if (post[0]) {
    snprintf(temp, sizeof(temp), "%s/.uade/%s", home, post);
    if (!access(temp, amode)) {
      strlcpy(path, temp, maxlen);
      return 1;
    }
  }
  if (glob[0]) {
    if (!access(glob, amode)) {
      strlcpy(path, glob, maxlen);
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
  int fd;
  char dir[UADE_PATH_MAX];
  char tmpdir[UADE_PATH_MAX];
  char user[1024] = "";

  if (!maxlen) {
    fprintf(stderr, "uade: ERROR: uade_get_temp_name: maxlen == 0\n");
    return 0;
  }

  /* find out temp directory */
  if (getenv("TEMP")) {
    strlcpy(tmpdir, getenv("TEMP"), sizeof(tmpdir));
  } else if (getenv("TMP")) {
    strlcpy(tmpdir, getenv("TMP"), sizeof(tmpdir));
  } else {
    strlcpy(tmpdir, "/tmp", sizeof(tmpdir));
  }

  if (getlogin()) {
    strcpy(user, getlogin());
  } else {
    sprintf(user, "%d", (int) getuid());
  }

  snprintf(dir, sizeof(dir), "%s/%s.uade", tmpdir, user);

  if (mkdir(dir, S_IRUSR | S_IWUSR | S_IXUSR)) {
    if (chmod(dir, S_IRUSR | S_IWUSR | S_IXUSR)) {
      fprintf(stderr, "uade: couldn't create directory for tmpfile\n");
      tempname[0] = 0;
      return 0;
    }
  }
  snprintf(tempname, maxlen, "%s/mmap.%d.XXXXXX", dir, (int) getpid());
  fd = mkstemp(tempname);
  if (fd < 0) {
    fprintf(stderr, "uade: couldn't generate tmp filename!\n");
    tempname[0] = 0;
    return 0;
  }
  close(fd);
  remove(tempname);
  return 1;
}


int uade_init_mmap_file(const char *mapfilename, int length) {
  char zerobuf[256];
  FILE *sharedmemFILE;
  int written, ret;
  if (!(sharedmemFILE = fopen(mapfilename, "w+"))) {
    fprintf(stderr,"uade: can not create mmap file (fopen)\n");
    return 0;
  }
  memset(zerobuf, 0, sizeof(zerobuf));
  written = 0;
  while (written < length) {
    if ((ret = fwrite(zerobuf, 1, sizeof(zerobuf), sharedmemFILE)) < 0) {
      fprintf(stderr, "uade: can not init mmap file (fwrite)\n");
      break;
    }
    written += ret;
  }
  fclose(sharedmemFILE);
  return 1;
}


int uade_is_regular_file(const char *filename) {
  struct stat s;
  if (stat(filename, &s) == 0) {
    if (S_ISREG(s.st_mode)) {
      return 1;
    }
  }
  return 0;
}

static int uade_amiga_scandir(char *real, char *dirname, char *fake, int ml) {
  DIR *dir;
  struct dirent *direntry;
  if (!(dir = opendir(dirname))) {
    fprintf(stderr, "uade: can't open dir (%s) (amiga scandir)\n", dirname);
    return 0;
  }
  while ((direntry = readdir(dir))) {
    if (!strcmp(fake, direntry->d_name)) {
      if (((int) strlcpy(real, direntry->d_name, ml)) >= ml) {
	fprintf(stderr, "uade: %s does not fit real", direntry->d_name);
	closedir(dir);
	return 0;
      }
      break;
    }
  }
  if (direntry) {
    closedir(dir);
    return 1;
  }
  rewinddir(dir);
  while ((direntry = readdir(dir))) {
    if (!strcasecmp(fake, direntry->d_name)) {
      if (((int) strlcpy(real, direntry->d_name, ml)) >= ml) {
	fprintf(stderr, "uade: %s does not fit real", direntry->d_name);
	closedir(dir);
	return 0;
      }
      break;
    }
  }
  closedir(dir);
  return direntry ? 1 : 0;
}

/* opens file in amiga namespace */
FILE *uade_open_amiga_file(char *aname) {
  char *separator;
  char *ptr;
  char copy[UADE_PATH_MAX];
  char dirname[UADE_PATH_MAX];
  char fake[UADE_PATH_MAX];
  char real[UADE_PATH_MAX];
  int len;
  DIR *dir;
  FILE *file;

  if (strlcpy(copy, aname, sizeof(copy)) >= sizeof(copy)) {
    fprintf(stderr, "uade: error: amiga tried to open a very long filename\nplease REPORT THIS!\n");
    return 0;
  }
  ptr = copy;
  /* fprintf(stderr, "uade: opening %s\n", ptr); */
  if ((separator = strchr(ptr, (int) ':'))) {
    char playerdir[UADE_PATH_MAX];
    if (!uade_get_path(playerdir, UADE_PATH_PLAYERDIR, sizeof(playerdir))) {
      fprintf(stderr, "uade: open_amiga_file: playerdir not found (%s)\n", aname);
      return 0;
    }
    len = (int) (separator - ptr);
    memcpy(dirname, ptr, len);
    dirname[len] = 0;
    if (!strcasecmp(dirname, "ENV")) {
      snprintf(dirname, sizeof(dirname), "%sENV/", playerdir);
    } else if (!strcasecmp(dirname, "S")) {
      snprintf(dirname, sizeof(dirname), "%sS/", playerdir);
    } else {
      fprintf(stderr, "uade: open_amiga_file: unknown amiga volume (%s)\n", aname);
      return 0;
    }
    if (!(dir = opendir(dirname))) {
      fprintf(stderr, "uade: can't open dir (%s) (volume parsing)\n", dirname);
      return 0;
    }
    closedir(dir);
    /* fprintf(stderr, "uade: opening from dir %s\n", dirname); */
    ptr = separator + 1;
  } else {
    if (*ptr == '/') {
      /* absolute path */
      strlcpy(dirname, "/", sizeof(dirname));
      ptr++;
    } else {
      /* relative path */
      strlcpy(dirname, "./", sizeof(dirname));
    }
  }

  while ((separator = strchr(ptr, (int) '/'))) {
    len = (int) (separator - ptr);
    if (!len) {
      ptr++;
      continue;
    }
    memcpy(fake, ptr, len);
    fake[len] = 0;
    if (uade_amiga_scandir(real, dirname, fake, sizeof(real))) {
      /* found matching entry */
      if (strlcat(dirname, real, sizeof(dirname)) >= sizeof(dirname)) {
	fprintf(stderr, "uade: too long dir path (%s + %s)\n", dirname, real);
	return 0;
      }
      if (strlcat(dirname, "/", sizeof(dirname)) >= sizeof(dirname)) {
	fprintf(stderr, "uade: too long dir path (%s + %s)\n", dirname, "/");
	return 0;
      }
    } else {
      /* didn't find entry */
      fprintf (stderr, "uade: %s not found from (%s) (dir scanning)\n", fake, dirname);
      return 0;
    }
    ptr = separator + 1;
  }
  /* fprintf(stderr, "uade: pass 3: (%s) (%s)\n", dirname, ptr); */

  if (!(dir = opendir(dirname))) {
    fprintf(stderr, "can't open dir (%s) (after dir scanning)\n", dirname);
    return 0;
  }
  closedir(dir);

  if (uade_amiga_scandir(real, dirname, ptr, sizeof(real))) {
    /* found matching entry */
    if (strlcat(dirname, real, sizeof(dirname)) >= sizeof(dirname)) {
      fprintf(stderr, "uade: too long dir path (%s + %s)\n", dirname, real);
      return 0;
    }
  } else {
    /* didn't find entry */
    fprintf (stderr, "uade: %s not found from %s\n", ptr, dirname);
    return 0;
  }
  if (!(file = fopen(dirname, "r"))) {
    fprintf (stderr, "uade: couldn't open file (%s) induced by (%s)\n", dirname, aname);
  }
  return file;
}

void uade_init_outpipe(char *parameter) {
  unix_usingoutpipe = 1;
  unix_soundfd = atoi(parameter);
}


void *uade_mmap_file(const char *filename, int length) {
  void *mmapptr;
  int fd;
  fd = open(filename,O_RDWR);
  if(fd < 0) {
    fprintf(stderr,"uade: can not open sharedmem file!\n");
    return 0;
  }
  mmapptr = mmap(0, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if(mmapptr == MAP_FAILED) {
    fprintf(stderr,"uade: can not mmap sharedmem file!\n");
    return 0;
  }
  return mmapptr;
}

int uade_send_signal(pid_t pid, int sigtype) {
  int signum;
  switch(sigtype) {
  case UADE_SIGALIVE:
    signum = 0;
    break;
  case UADE_SIGHUP:
    signum = SIGHUP;
    break;
  case UADE_SIGINT:
    signum = SIGINT;
    break;
  default:
    fprintf(stderr,"%s/uade: uade_send_signal: unknown signal %d\n", UADE_MODULE, sigtype);
    return -1;
  }
  if(pid > 0)
    return kill(pid, signum);
  else
    return -1;
}


void uade_sleep(int seconds) {
  sleep(seconds);
}


char *uade_strdup_basename(char *path) {
  char *copy;
  char *bn;
  if (!path)
    return 0;
  copy = strdup(path);
  if (!copy) {
    return 0;
  }
  bn = basename(copy);
  if (!bn) {
    goto error;
  }
  bn = strdup(bn);
 error:
  free(copy);
  return bn;
}


void uade_usleep(int microseconds) {
  usleep(microseconds);
}


/* returns number of milliseconds from the last call
   exception: on the first time this returns zero    */
int uade_timer_poll(void) {
  static struct timeval otv = {0,0};
  struct timeval tv;
  int ret = 0;
  gettimeofday(&tv, 0);
  if(otv.tv_sec != 0 || otv.tv_usec != 0) {
    ret = (tv.tv_sec-otv.tv_sec)*1000 + (tv.tv_usec-otv.tv_usec)/1000;
  }
  otv = tv;
  return ret;
}

char *uade_version(void) {
  return strdup(UADEVERSIONSTRING);
}

void uade_write_to_outpipe(void *ptr, int size) {
  int ret, written;
  if (unix_usingoutpipe) {
    written = 0;
    while (written < size) {
      ret = write(unix_soundfd, ptr, size);
      if (ret > 0) {
	written += ret;
      } else if (ret == 0) {
	break;
      } else {
	if (errno != EINTR) {
	  break;
	}
      }
    }
  }
}
