/* Copyright (C) Heikki Orsila 2003 <heikki.orsila@iki.fi>
   Parses eagleplayer files (in amiga executable format).
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "dpiutil.h"
#include "../osdep/strl.c"

typedef long long sul_s64;

static unsigned int readbig(char *ptr)
{
  unsigned char *p = (unsigned char *) ptr;
  unsigned int x = p[3] + (p[2] << 8) + (p[1] << 16) + (p[0] << 24);
  return x;
}


static void string_replace(char *str, char oldc, char newc)
{
  int i = 0;
  while (str[i]) {
    if (str[i] == oldc)
      str[i] = newc;
    i++;
  }
}


static int read_file_plus_zero(void **data, sul_s64 *len, char *filename)
{
  FILE *f;
  struct stat st;
  sul_s64 rlen;
  sul_s64 bytes;
  char *new_data;
  ssize_t ret;

  if (!(f = fopen(filename, "r")))
    return -1;

  if (stat(filename, &st))
    goto err;

  rlen = st.st_size;
  if (!rlen) {
    *data = 0;
    *len = 0;
    fclose(f);
    return 0;
  }

  if (!(new_data = malloc(rlen + 1)))
    goto err;

  bytes = 0;
  while (bytes < rlen) {
    ret = fread(new_data + bytes, 1, rlen - bytes, f);
    if (ret == 0) {
      if (bytes == 0 && ferror(f))
	goto err;
      break;
    }
    bytes += ret;
  }

  /* add a zero byte after the last read byte */
  new_data[bytes] = 0;

  *data = new_data;
  *len = bytes;
  fclose(f);
  return 0;

 err:
  fclose(f);
  return -1;
}


int process_eagleplayer(char *credits, char *filename, int credits_len)
{
  char *filebuf;
  int i;
  sul_s64 data_len;
  unsigned int x, y;
  char *tag_table;
  char *hunk;
  int hunk_size;
  int table_size;
  int offset;

  int is_amplifier;
  int is_customplayer;
  int is_noteplayer;
  int has_songend;

  if (credits)
    *credits = 0;

  if (read_file_plus_zero((void *) &filebuf, &data_len, filename))
    return 0;

  if (data_len <= 0)
    return 0;

  if (readbig(filebuf) != 0x000003f3) {
    goto err;
  }

  /* search for hunk data start */
  for (i = 0; i < data_len; i++) {
    /* is data start? */
    if (readbig(filebuf + i) == 0x70ff4e75) {
      break;
    }
  }

  if (i == data_len)
    goto err;

  if ((i + 12) >= data_len)
    goto err;

  if (strncmp(filebuf + i + 4, "DELIRIUM", 8) != 0 &&
      strncmp(filebuf + i + 4, "EPPLAYER", 8) != 0    ) {
    goto err;
  }

  if (!credits)
    printf("filename:\t%s\n", filename);

  hunk = filebuf + i; /* hunk start */
  hunk_size = data_len - i;

  if ((16 + 5) >= hunk_size) {
    goto oob;
  }

  /* check if $VER is available */
  if (!memcmp(&hunk[16], "$VER:", 5)) {
    offset = 16 + 5;
    while (offset < hunk_size) {
      if (!isspace(hunk[offset]))
	break;
      offset++;
    }
    if (offset >= hunk_size)
      goto oob;
    if ((offset + strlen(hunk + offset) + 1) > ((unsigned int) hunk_size))
      goto oob;
    if (credits) {
      snprintf(credits, credits_len, "VERSION:\n%s\n\n", hunk + offset);
    } else {
      printf("version:\t%s\n", hunk + offset);
    }
  }

  is_amplifier = 0;
  is_customplayer = 0;
  is_noteplayer = 0;
  has_songend = 0;

  /* tag table at hunk start + hunk[12] */
  offset = readbig(hunk + 12);
  if (offset < 0) {
    goto err;
  }
  tag_table = hunk + offset;

  if (tag_table >= &filebuf[data_len]) {
    goto oob;
  }

  table_size = ((int) (&filebuf[data_len] - tag_table)) / 8;

  if (table_size <= 0) {
    goto oob;
  }

  /* check all tags in this loop */
  for (i = 0; i < table_size; i += 2) {
    x = readbig(tag_table + 4 * i);
    y = readbig(tag_table + 4 * (i + 1));

    if (!x)
      break;
    
    switch (x) {
    case 0x80004459:
      if (y >= ((unsigned int) hunk_size))
	goto oob;
      if ((y + strlen(hunk + y) + 1) > ((unsigned int) hunk_size))
	goto oob;
      if (credits) {
	strlcat(credits, "PLAYERNAME:\n", credits_len);
	strlcat(credits, hunk + y, credits_len);
	strlcat(credits, "\n\n", credits_len);
      } else { 
	printf("playername:\t%s\n", hunk + y);
      }
      break;

    case 0x8000445a:
      if (y >= ((unsigned int) hunk_size))
	goto oob;
      if ((y + strlen(hunk + y) + 1) > ((unsigned int) hunk_size))
	goto oob;
      if (credits) {
	strlcat(credits, "CREDITS:\n", credits_len);
	strlcat(credits, hunk + y, credits_len);
	strlcat(credits, "\n\n", credits_len);
      } else {
	/* '\n' is replaced with ' ' to make it easier to grep for authors
	   of eagleplayers */
	string_replace(hunk + y, '\n', ' ');
	printf("credits:\t%s\n", hunk + y);
      }
      
    case 0x80004474:
      if ((y & 0x2) != 0) {
	has_songend = 1;
      }
      break;
      
    case 0x80004573:
      is_amplifier = 1;
      break;
      
    case 0x80004455:
      is_customplayer = 1;
      break;
      
    case 0x80004479:
    case 0x8000447A:
      is_noteplayer = 1;
      break;
      
    default:
      break;
    }
  }
  
  if (!credits) {
    printf("song end:\t%s\n", has_songend ? "yes" : "no");
    if (is_amplifier)
      printf("amplifier:\tyes\n");
    if (is_customplayer)
      printf("custom player:\tyes\n");
    if (is_noteplayer)
      printf("noteplayer:\tyes\n");
    printf("\n");
  }

  return 1;

 oob:
  fprintf(stderr, "dpi: %s out of boundary\n", filename);
  return 0;

 err:
  fprintf(stderr, "dpi: %s is not an eagleplayer\n", filename);
  return 0;
}
