/* $Id$ */

#include <fcntl.h> /* open, read, close */
#include <unistd.h> /* read, close */
#include <string.h> /* memcpy */
#include <stdlib.h> /* calloc,malloc,free */
#include <stdio.h> /*stderr*/
#include <stdarg.h> /*va_list,va_arg,va_end*/

#include "u.h"
#include "rn.h"
#include "er.h"
#include "rnc.h"

#define NKWD 19
static char *kwdtab[NKWD]={
  "attribute", "datatypes", "default", "div", "element", "empty", "external", 
  "grammar", "include", "inherit", "list", "mixed", "namespace", "notAllowed", 
  "parent", "start", "string", "text", "token"};

#define SYM_EOF -1

#define SYM_ATTRIBUTE 0
#define SYM_DEFAULT 1
#define SYM_DATATYPES 2
#define SYM_DIV 3
#define SYM_ELEMENT 4
#define SYM_EMPTY 5
#define SYM_EXTERNAL 6
#define SYM_GRAMMAR 7
#define SYM_INCLUDE 8
#define SYM_INHERIT 9
#define SYM_LIST 10
#define SYM_MIXED 11
#define SYM_NAMESPACE 12
#define SYM_NOT_ALLOWED 13
#define SYM_PARENT 14
#define SYM_START 15
#define SYM_STRING 16
#define SYM_TEXT 17
#define SYM_TOKEN 18

#define SYM_IDENT 19

#define SYM_ASGN 20
#define SYM_ASGN_ILEAVE 21
#define SYM_ASGN_CHOICE 22
#define SYM_SEQ 23 /* , */
#define SYM_CHOICE 24 
#define SYM_ILEAVE 25
#define SYM_OPTIONAL 26
#define SYM_ZERO_OR_MORE 27
#define SYM_ONE_OR_MORE 28
#define SYM_LPAR 29
#define SYM_RPAR 30
#define SYM_LCUR 31
#define SYM_RCUR 32
#define SYM_LSQU 33
#define SYM_RSQU 34
#define SYM_EXCEPT 35
#define SYM_QNAME 36   /* : */
#define SYM_CONCAT 37
#define SYM_NS_NAME 38 /* :* */
#define SYM_ANY_NAME SYM_ZERO_OR_MORE /* because they both are * */
#define SYM_QUOTE 39  /* \ */
#define SYM_FOLLOW_ANNOTATION 40 /* >> */
#define SYM_COMMENT 41 
#define SYM_LITERAL 42

#define BUFSIZE 1024
#define BUFTAIL 6

#define SRC_FREE 1
#define SRC_CLOSE 2
#define SRC_ERRORS 4

struct rnc_source {
  int flags;
  char *fn; int fd;
  char *buf; int i,n;
  int complete;
  int line,col;
  int u,v,w; int nx;
  char *s; int slen;
  int symline,symcol,nxtline,nxtcol;
  int sym,nxt;
};

static void rnc_init(struct rnc_source *sp);
static int rnc_read(struct rnc_source *sp);

int rnc_stropen(struct rnc_source *sp,char *fn,char *s,int len) {
  rnc_init(sp);
  sp->buf=s; sp->n=len; sp->complete=1; 
  return 0;
}

int rnc_bind(struct rnc_source *sp,char *fn,int fd) {
  rnc_init(sp);
  sp->fn=fn; sp->fd=fd;
  sp->buf=(char*)calloc(BUFSIZE,sizeof(char));
  sp->complete=sp->fd==-1;
  sp->flags=SRC_FREE;
  rnc_read(sp);
  return sp->fd;
}

int rnc_open(struct rnc_source *sp,char *fn) {
  sp->flags|=SRC_CLOSE;
  return rnc_bind(sp,fn,open(fn,O_RDONLY));
}

int rnc_close(struct rnc_source *sp) {
  int ret=0;
  if(sp->s) free(sp->s);
  if(sp->flags&SRC_FREE) free(sp->buf); 
  sp->buf=NULL;
  sp->complete=-1;
  if(sp->flags&SRC_CLOSE) {
    if(sp->fd!=-1) {
      ret=close(sp->fd); sp->fd=-1;
    }
  }
  return ret;
}

static void rnc_init(struct rnc_source *sp) {
  sp->flags=0;
  sp->fn=sp->buf=NULL;
  sp->i=sp->n=0; 
  sp->complete=sp->fd=-1;
  sp->nx=-1;
  sp->u=-1; sp->v=0; sp->line=1; sp->col=1;
  sp->s=(char*)calloc(sp->slen=BUFSIZE,sizeof(char));
  sp->nxt=sp->sym=-2;
}

static int rnc_read(struct rnc_source *sp) {
  int ni;
  memcpy(sp->buf,sp->buf+sp->i,sp->n-=sp->i);
  sp->i=0;
  for(;;) {
    ni=read(sp->fd,sp->buf+sp->n,BUFSIZE-sp->n);
    if(ni>0) {
      sp->n+=ni;
      if(sp->n>=BUFTAIL) break;
    } else {
      close(sp->fd); sp->fd=-1;
      sp->complete=1;
      break;
    }
  }
  return ni;
}

static void error(struct rnc_source *sp,int er_no,...) {
  va_list ap;
  va_start(ap,er_no);
  (*ver_handler_p)(er_no,ap);
  va_end(ap);
  sp->flags|=SRC_ERRORS;
}

/* read utf8 */
static void getu(struct rnc_source *sp) {
  int n,u0=sp->u;
  for(;;) {
    if(!sp->complete&&sp->i>sp->n-BUFTAIL) {
      if(rnc_read(sp)==-1) {error(sp,ER_IO,sp->fn);}
    }
    if(sp->i==sp->n) {
      sp->u=(u0=='\n'||u0=='\r'||u0==-1)?-1:'\n'; 
      u0=-1;
      break;
    } /* eof */
    n=u_get(&sp->u,sp->buf+sp->i);
    if(n==0) { 
      error(sp,ER_UTF,sp->fn,sp->line,sp->col);
      ++sp->i;
      continue;
    } else if(n+sp->i>sp->n) { 
      error(sp,ER_UTF,sp->fn,sp->line,sp->col);
      sp->i=sp->n;
      continue;
    } else {
      sp->i+=n;
      if(u0=='\r'&&sp->u=='\n') {u0='\n'; continue;}
    }
    break;
  }
  if(u0!=-1) {
    if(u0=='\r'||u0=='\n') {++sp->line; sp->col=0;} 
    if(!(sp->u=='\r'||sp->u=='\n')) {++sp->col;}
  }
}

/* newlines are replaced with \0; \x{<hex>+} are unescaped.
the result is in sp->v
*/
static void getv(struct rnc_source *sp) {
  if(sp->nx>0) {
    sp->v='x'; --sp->nx;
  } else if(sp->nx==0) {
    sp->v=sp->w;
    sp->nx=-1;
  } else {
    getu(sp);
    switch(sp->u) {
    case '\r': case '\n': sp->v=0; break;
    case '\\':
      getu(sp);
      if(sp->u=='x') {
	sp->nx=0;
	do {
	  ++sp->nx;
	  getu(sp);
	} while(sp->u=='x');
	if(sp->u=='{') { 
	  sp->nx=-1;
	  sp->v=0; 
	  for(;;) {
	    getu(sp);
	    if(sp->u=='}') goto END_OF_HEX_DIGITS;
	    sp->v<<=4;
	    switch(sp->u) {
            case '0': break;
            case '1': sp->v+=1; break;
            case '2': sp->v+=2; break;
            case '3': sp->v+=3; break;
            case '4': sp->v+=4; break;
            case '5': sp->v+=5; break;
            case '6': sp->v+=6; break;
            case '7': sp->v+=7; break;
            case '8': sp->v+=8; break;
            case '9': sp->v+=9; break;
	    case 'A': case 'a': sp->v+=10; break;
	    case 'B': case 'b': sp->v+=11; break;
	    case 'C': case 'c': sp->v+=12; break;
	    case 'D': case 'd': sp->v+=13; break;
	    case 'E': case 'e': sp->v+=14; break;
	    case 'F': case 'f': sp->v+=15; break;
            default: 
	      error(sp,ER_XESC,sp->fn,sp->symline,sp->symcol);
	      goto END_OF_HEX_DIGITS;
            }
	  } END_OF_HEX_DIGITS:;
	} else {
	  sp->v='\\'; sp->w=sp->u;
	}
      } else {
	sp->nx=0;
	sp->v='\\'; sp->w=sp->u;
      }
      break;
    default:
      sp->v=sp->u;
      break;
    }
  }
}

#define skip_comment(sp) 
/* why \r is not a new line by itself when escaped? it is when not. */
#define newline(v) ((v)==0||(v)=='\n')
#define whitespace(v) ((v)==' '||(v)=='\t')
#define name_start(v) (u_base_char(v)||u_ideographic(v)||(v)=='_')
#define name_char(v) (name_start(v)||u_digit(v)||u_combining_char(v)||u_extender(v)||(v)=='.'||(v)=='-')

static void realloc_s(struct rnc_source *sp) {
  char *s; int slen=sp->slen*2;
  s=(char*)calloc(slen,sizeof(char));
  memcpy(s,sp->s,sp->slen); free(sp->s); 
  sp->s=s; sp->slen=slen;
}

static char *sym2str(int sym) {
  switch(sym) {
  case SYM_EOF: return "EOF"; 
  case SYM_ATTRIBUTE: return "\"attribute\"";
  case SYM_DEFAULT: return "\"default\"";
  case SYM_DATATYPES: return "\"datatypes\"";
  case SYM_DIV: return "\"div\"";
  case SYM_ELEMENT: return "\"element\"";
  case SYM_EMPTY: return "\"empty\"";
  case SYM_EXTERNAL: return "\"external\"";
  case SYM_GRAMMAR: return "\"grammar\"";
  case SYM_INCLUDE: return "\"include\"";
  case SYM_INHERIT: return "\"inherit\"";
  case SYM_LIST: return "\"list\"";
  case SYM_MIXED: return "\"mixed\"";
  case SYM_NAMESPACE: return "\"namespace\"";
  case SYM_NOT_ALLOWED: return "\"notAllowed\"";
  case SYM_PARENT: return "\"parent\"";
  case SYM_START: return "\"start\"";
  case SYM_STRING: return "\"string\"";
  case SYM_TEXT: return "\"text\"";
  case SYM_TOKEN: return "\"token\"";
  case SYM_IDENT: return "identifier"; 
  case SYM_ASGN: return "\"=\"";
  case SYM_ASGN_ILEAVE: return "\"&=\"";
  case SYM_ASGN_CHOICE: return "\"|=\"";
  case SYM_SEQ: return "\",\"";
  case SYM_CHOICE: return "\"|\"";
  case SYM_ILEAVE: return "\"&\"";
  case SYM_OPTIONAL: return "\"?\"";
  case SYM_ZERO_OR_MORE /*SYM_ANY_NAME*/: return "\"*\"";
  case SYM_ONE_OR_MORE: return "\"+\"";
  case SYM_LPAR: return "\"(\"";
  case SYM_RPAR: return "\")\"";
  case SYM_LCUR: return "\"{\"";
  case SYM_RCUR: return "\"}\"";
  case SYM_LSQU: return "\"[\"";
  case SYM_RSQU: return "\"]\"";
  case SYM_EXCEPT: return "\"-\"";
  case SYM_QNAME: return "\":\"";
  case SYM_CONCAT: return "\"~\"";
  case SYM_NS_NAME: return "\":*\"";
  case SYM_QUOTE: return "\"\\\"";
  case SYM_FOLLOW_ANNOTATION: return "\">>\"";
  case SYM_COMMENT: return "\"#\"";
  case SYM_LITERAL: return "literal";
  default: assert(0);
  }
  return NULL;
}

static void getsym(struct rnc_source *sp) {
  sp->sym=sp->nxt; sp->symline=sp->nxtline; sp->symcol=sp->nxtcol;
  sp->nxtline=sp->line; sp->nxtcol=sp->col;
  for(;;) {
    if(newline(sp->v)||whitespace(sp->v)) {getv(sp); continue;}
    switch(sp->v) {
    case -1: sp->nxt=SYM_EOF; return; 
    case '#': do getv(sp); while(!newline(sp->v)); getv(sp); continue;
    case '=': getv(sp); sp->nxt=SYM_ASGN; return;
    case ',': getv(sp); sp->nxt=SYM_SEQ; return;
    case '|': getv(sp); 
      if(sp->v=='=') {
	getv(sp); sp->nxt=SYM_ASGN_CHOICE; return;
      } sp->nxt=SYM_CHOICE; return;
    case '&': getv(sp); 
      if(sp->v=='=') {getv(sp); sp->nxt=SYM_ASGN_ILEAVE;} else sp->nxt=SYM_ILEAVE; return;
    case '?': getv(sp); sp->nxt=SYM_OPTIONAL; return;
    case '*': getv(sp); sp->nxt=SYM_ZERO_OR_MORE; return; /* SYM_ANY_NAME */
    case '+': getv(sp); sp->nxt=SYM_ONE_OR_MORE; return;
    case '-': getv(sp); sp->nxt=SYM_EXCEPT; return;
    case '~': getv(sp); sp->nxt=SYM_CONCAT; return;	   
    case '(': getv(sp); sp->nxt=SYM_LPAR; return;
    case ')': getv(sp); sp->nxt=SYM_RPAR; return;
    case '{': getv(sp); sp->nxt=SYM_LCUR; return;
    case '}': getv(sp); sp->nxt=SYM_RCUR; return;
    case '[': getv(sp); sp->nxt=SYM_LSQU; return;
    case ']': getv(sp); sp->nxt=SYM_RSQU; return;
    case ':': getv(sp); 
      if(sp->v=='*') {getv(sp); sp->nxt=SYM_NS_NAME;} else sp->nxt=SYM_QNAME; return;
    case '>': getv(sp); 
      if(sp->v!='>') error(sp,ER_LEXP,'>',sp->fn,sp->line,sp->col);
      getv(sp); sp->nxt=SYM_FOLLOW_ANNOTATION; return;
    case '"': case '\'': 
      { int q=sp->v;
	int triple=0;
	int i=0;
	getv(sp); 
	if(sp->v==q) {getv(sp);
	  if(sp->v==q) { // triply quoted string
	    triple=1; getv(sp);
	  } else {
	    sp->s[0]='\0'; sp->nxt=SYM_LITERAL; return;
	  }
	} 
	for(;;) {
	  if(sp->v==q) {
	    if(triple) {
	      if(i>=2 && sp->s[i-2]==q && sp->s[i-1]==q) {
		sp->s[i-2]='\0'; break;
	      } else sp->s[i]=(char)sp->v;
	    } else {sp->s[i]='\0'; break;}
	  } else if(sp->v<=0) {
	    if(sp->v==-1 || !triple) {
	      error(sp,ER_LLIT,sp->fn,sp->line,sp->col);
	      sp->s[i]='\0'; break;
	    } else sp->s[i]='\n';
	  } else sp->s[i]=(char)sp->v;
	  getv(sp);
	  if(++i==sp->slen) realloc_s(sp);
	}
	getv(sp); sp->nxt=SYM_LITERAL; return;
      } 
    default: 
      { int escaped=0;
        if(sp->v=='\\') {escaped=1; getv(sp);}
	if(name_start(sp->v)) {
	  int i=0;
	  for(;;) {
	    sp->s[i++]=sp->v;
	    if(i==sp->slen) realloc_s(sp);
	    getv(sp);
	    if(!name_char(sp->v)) {sp->s[i]='\0'; break;}
          }
	  if(!escaped) {
	    int n=0,m=NKWD-1,i,cmp;
	    for(;;) {
	      if(n>m) break;
	      i=(n+m)/2;
	      cmp=strcmp(sp->s,kwdtab[i]);
	      if(cmp==0) {sp->sym=i; return;} else if(cmp<0) m=i-1; else n=i+1;
	    }
	  } 
	  sp->nxt=SYM_IDENT; return;
	} else {
	  error(sp,ER_LILL,sp->v,sp->fn,sp->line,sp->col);
	  getv(sp);
	  continue;
	}
      }	
    }
  }
}

static void chksym(struct rnc_source *sp,int sym) {
  if(sp->sym!=sym) error(sp,ER_SEXP,sym2str(sym),sp->fn,sp->line,sp->col);
  getsym(sp);
}

/* return 1 if there was a declaration, 0 otherwise */
static int decl(struct rnc_source *sp) {
  switch(sp->sym) {
  case SYM_NAMESPACE: return 1;
  case SYM_DEFAULT: return 1;
  case SYM_DATATYPES: return 1;
  default: return 0;
  }
}

static void annotationBody(struct rnc_source *sp,int allow_text) {
}

static void leadingAnnotation(struct rnc_source *sp) {
  annotationBody(sp,0);
}

static int grammarContent(struct rnc_source *sp) {
  if(sp->sym==SYM_IDENT && sp->nxt==SYM_LSQU) {
    return 1;
  } else if(sp->sym==SYM_LSQU) {
    leadingAnnotation(sp);
    switch(sp->sym) {
    case SYM_DIV:
    case SYM_INCLUDE:
    case SYM_START:
    case SYM_EOF:
      return 1;
    case SYM_IDENT:
      switch(sp->nxt) {
      case SYM_ASGN:
      case SYM_ASGN_CHOICE:
      case SYM_ASGN_ILEAVE:
	return 1;
      }
    } 
  }
  return 0;
}

static void pattern(struct rnc_source *sp) {
}

static void topLevel(struct rnc_source *sp) {
  while(decl(sp));
  if(grammarContent(sp)) {
    while(grammarContent(sp));
    chksym(sp,SYM_EOF);
  } else pattern(sp);
}

void rnc_parse(struct rnc_source *sp) {
  getsym(sp); getsym(sp);
  topLevel(sp);
 /* second pass here */ 
}

int main(int argc,char **argv) {
  struct rnc_source src;
  rnc_bind(&src,"stdin",0);
  getsym(&src);
  for(;;) {
    getsym(&src);
    switch(src.sym) {
    case SYM_EOF: return 0;
    case SYM_LITERAL: printf("(%i,%i) ``%s''\n",src.line,src.col,src.s); break;
    case SYM_IDENT: printf("(%i,%i) $%s\n",src.line,src.col,src.s); break;
    default: if(src.sym<NKWD) printf("(%i,%i) @%s\n",src.line,src.col,src.s); break;
    }
  }
  rnc_close(&src);
}

/*
 * $Log$
 * Revision 1.12  2003/11/26 00:37:47  dvd
 * parser in progress, documentation handling removed
 *
 * Revision 1.11  2003/11/25 13:14:21  dvd
 * scanner ready
 *
 * Revision 1.10  2003/11/25 10:33:53  dvd
 * documentation and comments
 *
 * Revision 1.9  2003/11/24 23:00:27  dvd
 * literal, error reporting
 *
 * Revision 1.8  2003/11/23 16:16:06  dvd
 * no roles for elements
 *
 * Revision 1.7  2003/11/21 00:20:06  dvd
 * lexer in progress
 *
 * Revision 1.4  2003/11/20 23:28:50  dvd
 * getu,getv debugged
 *
 * Revision 1.3  2003/11/20 16:29:08  dvd
 * x escapes sketched
 *
 * Revision 1.2  2003/11/20 07:46:16  dvd
 * +er, rnc in progress
 *
 * Revision 1.1  2003/11/19 00:28:57  dvd
 * back to lists of ranges
 *
 */
