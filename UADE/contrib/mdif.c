#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
  FILE **f;
  int i,x,n;
  char *c;
  if(argc==1) return 0;
  n=argc-1;
  f = malloc(sizeof(FILE*)*n);
  c = malloc(sizeof(char)*n);
  for(i=1;i<argc;i++) {
    if(!(f[i-1]=fopen(argv[i],"r"))) {
      fprintf(stderr,"couldn't open file %s\n",argv[i]);
      return 0;
    }
  }
  while(1) {
    for(i=0;i<n;i++) {
      x = fread(&c[i],1,1,f[i]);
      if(x==0 || x==EOF)
	return 0;
    }
    for(i=1;i<n;i++) {
      if(c[i]!=c[i-1])
	break;
    }
    if(i==n)
      printf("%c",c[0]);
    else
      printf("%c",'\0');
  }
  return 0;
}
