#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "xsd.h"

int main(int argc,char **argv) {
  xsd_init();
  ++argv; --argc;

  if(!*argv) goto USAGE;
  if(strcmp(*argv,"equal")==0) {
    if(argc!=4) goto USAGE;
    return !xsd_equal(*(argv+1),*(argv+2),*(argv+3),strlen(*(argv+3)));
  } else if(strcmp(*argv,"allows")==0) {
    int len,i;
    char *ps,*p,*a;
    if(argc<3||!(argc&1)) goto USAGE;
    len=argc-2; for(i=2;i!=argc-1;++i) len+=strlen(*(argv+i));
    ps=(char*)malloc(len); ps[len-1]='\0';
    p=ps; for(i=2;i!=argc-1;++i) {
      a=*(argv+i);
      while((*(p++)=*(a++)));
    }
    return !xsd_allows(*(argv+1),ps,*(argv+argc-1),strlen(*(argv+argc-1)));
  }
USAGE:
  fprintf(stderr,"xsdck: invalid arguments\n");
  return 255;
}