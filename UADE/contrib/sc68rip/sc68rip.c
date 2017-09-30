/* file: sc68rip 0.0.1
 * license: GNU Public License (GPL)
 * descr:
 *        A command line tool for the uade player that extracts
 *	  the plain amiga music data from the sc68 fileformat
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <unistd.h>

int all =0;

static void rip_sc68 (char *filename);


int main(int argc, char**argv) {
  int i;

    fprintf (stdout, "sc68rip 0.0.1\n");

  i=1;
  if(argc>i) {

    if(strcmp("-a",argv[i])==0) {
      all = 1;
      i++;
    }

  }
  
  for(;i<argc;i++) {

    rip_sc68(argv[i]);    


  }
  return(0);
}

void rip_sc68 (char *filename)
{
    FILE *sc68file;
    FILE *outfile;
    unsigned char *packed, *unpacked;
    char fn[1024]="\0";
    //char mn[1024]="\0";
    char pre[256], post[256];
    char *filename2;
    int plen, unplen;
    int i,j,c;
    int begdata=0;
    int	enddata=0;
    int count=0;
    struct stat st;
    


    sc68file = fopen (filename, "r");

    if (!sc68file) {
        fprintf (stderr, "can not open file %s\n", filename);
        return;
	}
    fstat (fileno (sc68file), &st);
    plen = st.st_size;
    packed = malloc (plen);
    if (!packed){
	fprintf(stderr, "can't allocate memory for sc68 file...");
	return;
	}
    fread (packed, plen,1 ,sc68file);
    fclose (sc68file);    

    for (i = 0; i <= plen-4; i++) {

	if (packed [i]=='S' && packed [i+1]=='C' && packed [i+2] =='F' && packed [i+3]=='N')
	    {
	    strcpy(fn, packed+i+8);
	    fprintf (stdout, "\n%s:\n", fn);
	    }
	/*
	if (packed [i]=='S' && packed [i+1]=='C' && packed [i+2] =='M' && packed [i+3]=='N' && verbose)
	    {
	    strcpy(mn, packed+i+8);
	    fprintf (stdout, "%s:\n", mn);
	    }
	*/
	
	if (packed [i]=='S' && packed [i+1]=='C' && packed [i+2] =='D' && packed [i+3]=='A')
	    {
	    j=0;
	    strcpy (pre,"\0");
	    strcpy (post,"\0");

	    while (j <plen-i-9){
		if (packed[i+8+j] == 0x4e && packed [i+9+j] == 0x71)
		{
		    j=j+2;
		} else {
		    count++;
		    begdata = i + 8 +j;
		    enddata = (packed[i+4] + packed[i+5]*0x100 + packed [i+6] *0x10000 + packed [i+7] *0x1000000) - j;
		    i = i + 8 + enddata;
		    

		    if (all) strcpy (pre,"UNKWN\0");

		    filemagic (packed+begdata,pre,post, 5122);

		    if (strcasecmp (pre, "\0") ==0){
		        fprintf (stdout, "skipping unknown music data in sc68 file\n(use 'sc68rip -a' for ripping it anyway...)\n");
		    } else {		    
		        sprintf (filename2, "%s.%s-%ld", pre,fn,count); /*create Filename*/

			outfile = fopen (filename2, "w+b");
			if (outfile)
			    {
    		    	    fprintf (stdout, "-> %s\n", filename2);
			    fwrite (packed+begdata, enddata ,1, outfile);
			    fflush (outfile);
			    fclose (outfile);
			    }
		    }
		    break;
		}
	    }
	}
    }
    free (packed);
    return;
}
