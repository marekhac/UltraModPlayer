/* This source file is public domain (do whatever you wish with it).
   Created by Heikki Orsila <heikki.orsila@tut.fi>
*/

#ifndef _UADE_CHECKSUM_H_
#define _UADE_CHECKSUM_H_

int filechecksum(char *md5sum, const char *filename);
int strchecksum(char *md5sum, const char *string);

#endif
