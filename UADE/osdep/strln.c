#include <string.h>

/* strlncat is an extension to strlcat. it uses at most 'srcsize' bytes from
   the source */

static size_t strlncat(char *dst, const char *src, size_t dstsize, size_t srcsize);

static size_t
strlncat(char *dst, const char *src, size_t dstsize, size_t srcsize)
{
  size_t slen = strlen(src);
  size_t dlen = 0;

  if(slen > srcsize)
    slen = srcsize;

  while(dlen < dstsize) {
    if(dst[dlen] == 0)
      break;
    dlen++;
  }

  if(dlen == dstsize) {
    return slen + dlen;
  }

  if((dlen + slen) < dstsize) {
    strncat(dst, src, slen);
    dst[dlen + slen] = 0;
  } else {
    int left = dstsize - dlen - 1;
    if(left > 0) {
      strncat(dst, src, left);
    }
    dst[dstsize-1] = 0;
  }
  return slen + dlen;
}
