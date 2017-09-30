/* 'dpi' prints info from given eagleplayer files
   USAGE: ./dpi eagleplayer_files ...
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dpiutil.h"

int main(int argc, char **argv) {
  int i;
  for(i = 1; i < argc; i++) {
    process_eagleplayer(0, argv[i], 0);
  }
  return 0;
}
