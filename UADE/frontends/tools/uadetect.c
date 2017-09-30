/* file: amimustype.c
 * license: GNU Public License (GPL)
 * descr:
 *        A command line tool for the uade player that detects 
 *        some of the real module-formats and prints out the right
 *	  prefix
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int main(int argc, char**argv) {
  char pre[256],post[256];
  char *c, *filename2;
  char *filename;
  int i,verbose = 0;

  i=1;

  if(argc>i) {
    if(strcmp("-v",argv[i])==0) {
      verbose = 1;
      i++;
    }
  }
  
  for(;i<argc;i++) {
    
    pre[0]='\0';
    post[0]='\0';
    
    filename = argv[i];

    filename2=strrchr(filename,'/');
    if(filename2==0){
      filename2=filename;
    } else {
      filename2++;
    }
    
    c=strchr(filename2,'.');

    if (c==0) {
      fprintf(stderr,"%s: unknown format\n", argv[i]);
      continue;
    }

    strncpy(pre,filename2,c-filename2);
    pre[c-filename2]='\0';
    c=strrchr(filename2,'.');
    strcpy(post,c+1);

    scanfile(argv[i], pre, post);

    if(verbose)
      fprintf(stdout,"%s: ",argv[i]);

    if (strcasecmp(pre,"\0")){
      fprintf(stdout,"%s\n",pre);
    } else {
      fprintf(stdout,"%s\n",post);
    }
  }
  return(0);
}
