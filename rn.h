/* $Id$ */

#ifndef RN_H
#define RN_H 1

#include <assert.h>

/* Patterns */
#define P_ERROR 0
#define P_EMPTY 1
#define P_NOT_ALLOWED 2
#define P_TEXT 3
#define P_CHOICE 4
#define P_INTERLEAVE 5
#define P_GROUP 6
#define P_ONE_OR_MORE 7
#define P_LIST 8
#define P_DATA 9
#define P_DATA_EXCEPT 10
#define P_VALUE 11
#define P_ATTRIBUTE 12
#define P_ELEMENT 13
#define P_AFTER 14

/*
Patterns and nameclasses are stored in arrays of arrays of integers.
an integer is either an index in the same or another array,
or a value that denotes record type etc.

Each record has a macro that accesses its fields by assigning
them to variables in the local scope, and a creator.
*/

/* Pattern Bindings */
#define P_TYP(i) (rn_pattern[i][0]&0xff)
#define P_IS(i,x)  (P_##x==P_TYP(i))
#define P_CHK(i,x)  assert(P_IS(i,x))

#define P_FLG_NUL 0x0100
#define P_FLG_TXT 0x0200
#define P_FLG_CTE 0x0400
#define P_FLG_CTC 0x0800
#define P_FLG_CTS 0x1000

#define nullable(i) (rn_pattern[i][0]&P_FLG_NUL)
#define cdata(i) (rn_pattern[i][0]&P_FLG_TXT)
#define contentType(i) (rn_pattern[i][0]&0x1C00)

#define Empty(i) P_CHK(i,EMPTY)
#define NotAllowed(i) P_CHK(i,NOT_ALLOWED)
#define Text(i) P_CHK(i,TEXT)
#define Choice(i,p1,p2) P_CHK(i,CHOICE); p1=rn_pattern[i][1]; p2=rn_pattern[i][2]
#define Interleave(i,p1,p2) P_CHK(i,INTERLEAVE); p1=rn_pattern[i][1]; p2=rn_pattern[i][2]
#define Group(i,p1,p2) P_CHK(i,GROUP); p1=rn_pattern[i][1]; p2=rn_pattern[i][2]
#define OneOrMore(i,p1) P_CHK(i,ONE_OR_MORE); p1=rn_pattern[i][1]
#define List(i,p1) P_CHK(i,LIST); p1=rn_pattern[i][1]
#define Data(i,dt,ps) P_CHK(i,DATA); dt=rn_pattern[i][1]
#define DataExcept(i,dt,p1) P_CHK(i,DATA_EXCEPT); dt=rn_pattern[i][1]; p1=rn_pattern[i][2]
#define Value(i,dt,s) P_CHK(i,VALUE); dt=rn_pattern[i][1]; s=rn_pattern[i][2]
#define Attribute(i,nc,p1) P_CHK(i,ATTRIBUTE); nc=rn_pattern[i][1]; p1=rn_pattern[i][2]
#define Element(i,nc,p1) P_CHK(i,ELEMENT); nc=rn_pattern[i][1]; p1=rn_pattern[i][2]
#define After(i,qn,p1,p2) P_CHK(i,AFTER); qn=rn_pattern[i][1]; p1=rn_pattern[i][2]; p2=rn_pattern[i][3]

/* Name Classes */
#define NC_ERROR 0
#define NC_ANY_NAME 1
#define NC_ANY_NAME_EXCEPT 2
#define NC_NAME 3
#define NC_NSNAME 4
#define NC_NSNAME_EXCEPT 5
#define NC_CHOICE 6
#define NC_DATATYPE 7

/* Name Class Bindings  */
#define NC_TYP(i) (rn_nameclass[i][0]&0xff)
#define NC_IS(i,x) (NC_##x==NC_TYP(i))
#define NC_CHK(i,x) assert(NC_IS(i,x))

#define AnyName(i) NC_CHK(i,ANY_NAME) 
#define AnyNameExcept(i,nc) NC_CHK(i,ANY_NAME_EXCEPT); nc=rn_nameclass[i][1]
#define QName(i,uri,localname) NC_CHK(i,NAME); uri=rn_nameclass[i][1]; localname=rn_nameclass[i][2]
#define NsName(i,uri) NC_CHK(i,NSNAME); uri=rn_nameclass[i][1]
#define NsNameExcept(i,uri,nc) NC_CHK(i,NSNAME_EXCEPT); uri=rn_nameclass[i][1]; nc=rn_nameclass[i][2]
#define NameClassChoice(i,nc1,nc2) NC_CHK(i,CHOICE); nc1=rn_nameclass[i][1]; nc2=rn_nameclass[i][2]
#define Datatype(i,lib,dt) NC_CHK(i,DATATYPE); lib=rn_nameclass[i][1]; dt=rn_nameclass[i][2]

extern int rn_empty,rn_text,rn_notAllowed;

extern char *rn_string;

#define P_SIZE 4
#define NC_SIZE 3

extern int (*rn_pattern)[P_SIZE];
extern int (*rn_nameclass)[NC_SIZE];

extern int *rn_first, *rn_first_a, *rn_first_c, *rn_first_to; /* -1 if not computed, otherwise index in firsts */
extern int *rn_firsts; /* -1 terminates a list */

extern int newString(char *s);

extern int newEmpty();
extern int newNotAllowed();
extern int newText();
extern int newChoice(int p1,int p2);
extern int newInterleave(int p1,int p2);
extern int newGroup(int p1,int p2);
extern int newOneOrMore(int p1);
extern int newList(int p1);
extern int newData(int dt,int ps);
extern int newDataExcept(int dt,int p1);
extern int newValue(int dt,int s);
extern int newAttribute(int nc,int p1);
extern int newElement(int nc,int p1);
extern int newAfter(int qn,int p1,int p2);

extern int rn_groupable(int p1,int p2);
extern int rn_one_or_more(int p);
extern int rn_group(int p1,int p2);
extern int rn_choice(int p1,int p2);
extern int rn_ileave(int p1,int p2);

extern int newAnyName();
extern int newAnyNameExcept(int nc);
extern int newQName(int uri,int localname);
extern int newNsName(int uri);
extern int newNsNameExcept(int uri,int nc);
extern int newNameClassChoice(int nc1,int nc2);
extern int newDatatype(int lib,int dt);

extern void rn_init();

#endif

/*
 * $Log$
 * Revision 1.9  2003/12/04 00:37:03  dvd
 * refactoring
 *
 * Revision 1.8  2003/12/01 14:44:53  dvd
 * patterns in progress
 *
 * Revision 1.7  2003/11/29 20:51:39  dvd
 * nameclasses
 *
 * Revision 1.6  2003/11/29 17:47:48  dvd
 * decl
 *
 * Revision 1.5  2003/11/26 00:37:47  dvd
 * parser in progress, documentation handling removed
 *
 * Revision 1.4  2003/11/24 23:00:27  dvd
 * literal, error reporting
 *
 * Revision 1.3  2003/11/23 16:16:06  dvd
 * no roles for elements
 *
 * Revision 1.2  2003/11/20 07:46:16  dvd
 * +er, rnc in progress
 *
 * Revision 1.1  2003/11/17 21:33:28  dvd
 * +cimpl
 *
 */
