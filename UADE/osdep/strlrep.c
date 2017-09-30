#include <string.h>

/* see OpenBSD man pages for strlcat and strlcpy */

static size_t strlcpy(char *dst, const char *src, size_t size);
static size_t strlcat(char *dst, const char *src, size_t size);


static size_t
strlcpy(char *dst, const char *src, size_t size)
{
  size_t slen = strlen(src);
  if(slen < size)
    strcpy(dst, src);
  else {
    strncpy(dst, src, size-1);
    dst[size-1] = 0;
  }
  return slen;
}


static size_t
strlcat(char *dst, const char *src, size_t size)
{
  size_t slen = strlen(src);
  size_t dlen = 0;
  while(dlen < size) {
    if(dst[dlen] == 0)
      break;
    dlen++;
  }

  if(dlen == size) {
    return slen + dlen;
  }

  if((dlen + slen) < size)
    strcat(dst, src);
  else {
    int left = size - dlen - 1;
    if(left > 0) {
      strncat(dst, src, left);
    }
    dst[size-1] = 0;
  }
  return slen + dlen;
}
