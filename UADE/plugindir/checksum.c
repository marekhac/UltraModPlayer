/* This source file is public domain (do whatever you wish with it).
   Created by Heikki Orsila <heikki.orsila@iki.fi>
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "md5.h"
#include "checksum.h"

static int checksum_size_check = 0;

/* md5 checksum takes at least 33 byte buffer as input */

int strchecksum(char *md5sum, const char *string) {
  unsigned char buf[17];
  MD5_CTX c;
  if (checksum_size_check == 0) {
    checksum_size_check = 1;
    if (sizeof(uade_uint32_t) != 4) {
      fprintf(stderr,"uade: string checksum: warning: sizeof(uade_uint32_t) != 4\n");
    }
  }
  if (!md5sum) {
    fprintf(stderr,"uade: string checksum: md5sum pointer = 0\n");
    return 0;
  }
  if (!string) {
    fprintf(stderr,"uade: string checksum: string pointer = 0\n");
    md5sum[0] = 0;
    return 0;
  }
  MD5Init(&c);
  MD5Update(&c, string, strlen(string));
  MD5Final((unsigned char *) buf, &c);
  snprintf(md5sum, 33, "%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10],buf[11],buf[12],buf[13],buf[14],buf[15]);
  return 1;
}

int filechecksum(char *md5sum, const char *filename) {
  FILE *in;
  MD5_CTX c;
  int nbytes;
  const unsigned int bufsize = 4096;
  unsigned char buf[bufsize];
  int totalbytes = 0;
  if (checksum_size_check == 0) {
    checksum_size_check = 1;
    if (sizeof(uade_uint32_t) != 4) {
      fprintf(stderr,"uade: file checksum: warning: sizeof(uade_uint32_t) != 4\n");
    }
  }
  if (!md5sum) {
    fprintf(stderr,"uade: file checksum: md5sum pointer zero (file %s)!\n",filename);
    return 0;
  }
  if (!(in = fopen(filename, "rb"))) {
    return 0;
  }
  MD5Init(&c);
  while (1) {
    nbytes = fread(buf, 1, bufsize, in);
    if (nbytes == 0 || nbytes == EOF)
      break;
    totalbytes += nbytes;
    MD5Update(&c, buf, nbytes);
  }
  fclose(in);
  MD5Final((unsigned char *) buf, &c);
  snprintf(md5sum, 33, "%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10],buf[11],buf[12],buf[13],buf[14],buf[15]);
  return totalbytes;
}
