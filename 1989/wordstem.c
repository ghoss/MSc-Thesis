/*****************************************************************
*
*           S O F T W A R E   S O U R C E   F I L E
*
*****************************************************************
*
*   Name of file   : wordstem.c
*   Author         : Guido Hoss
*   Creation Date  : 21/12/88
*   Type of file   : C Language File
*                    (Microsoft C 5.00 Compiler)
*
*   Description    : Implements an algorithm to stem English
*                    words down to the root. The algorithm was
*                    taken from
*
*                    M.F. Porter: An algorithm for suffix stripping
*                    Program, Vol. 14, no. 3, pp 130-137, July 1980
*
*****************************************************************
*
*   COPYRIGHT (C) 1989, 2016 BY GUIDO HOSS.
*
*   This program is free software: you can redistribute it and/or 
*   modify it under the terms of the GNU General Public License
*   as published by the Free Software Foundation, either version 3
*   of the License, or (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public
*   License along with this program.  If not, see
*   <http://www.gnu.org/licenses/>.
*
*   Git repository home: <https://github.com/ghoss/Thesis>
*
*****************************************************************
* Date        :
* Description :
****************************************************************/   

#include <string.h>
#include "wordstem.h"
#include "boolean.h"

#define MAX_WORDLEN	100

/* Rules used */
#define S1a	1
#define S1b	2
#define S1bb	4
#define S1c	8
#define S2	16
#define S3	32
#define S4	64
#define S5a	128
#define S5b	256
#define S5c	512


int
  ruleSet,
  endw, i, h;
  
char
  ch, ck;
  
char *
  word;
  
BOOL
  b;
   
    
/****************************************************************
**  Vowel
**
**  Returns TRUE if ch is a vowel.
****************************************************************/   
  
BOOL vowel 
       ( ch ) 
char
  ch;
{
  return ( ( ch == 'A' ) || ( ch == 'E' ) || ( ch == 'I' ) || ( ch =='O' )
           || ( ch == 'U' ) );
}


/****************************************************************
**  Step1a
**
**  SSES -> SS, IES -> Y, SS   -> SS, S ->
**                     ^ ** CHANGED! **
****************************************************************/   

void Step1a
       ( )
{
  if ( *( word + endw ) == 'S' ) {
    -- endw;
    if ( *( word + endw ) == 'S' ) { 
      ++ endw;
    } 
    else if ( ( *( word + endw ) == 'E' ) && 
              ( ( *( word + endw - 1 ) == 'I' ) ||
                ( ( *( word + endw - 1 ) == 'S' ) && 
                  ( endw  > 1 ) && ( *( word + endw - 2 ) == 'S' ) ) ) ) { 
      -- endw;
    }

#ifndef FULL_PORTER    
    /* change to original algorithm (since we only do plural stemming) */
    if ( *( word + endw ) == 'I' ) {
      *( word + endw ) = 'Y';
    }
#endif
  
    *( word + endw + 1 ) = '\0';
    ruleSet |= S1a;
  }
}
  
  
/****************************************************************
**  MeasureM
**
**  Parses 'word' and returns the measure m, where
**  word = [C](VC)^m[V] ,
**  C={c} , V={v} , v= (A|E|I|O|U)      | Y , if preceded by c 
**		      c= (B|C|D|F|G...|Z) | Y ,if not preceded by c
**
**  The parses is a finite state machine with 3 states. 'until'
**  points to the position behind the last letter.
****************************************************************/   

#define STAT_NULL	0
#define STAT_WASC	1
#define STAT_WASV	2

int MeasureM
       ( until )
int
  until;
{
  int
    status;
    
  i = 0; 
  h = 0;
  status = STAT_NULL;
  while ( TRUE ) {
    if ( i >= until ) break;
    ch = *( word + i );
    switch ( status ) {
    
      case STAT_NULL :
         if ( vowel ( ch ) ) {
           status = STAT_WASV;
         }
         else {
           status = STAT_WASC; 
         }
         break;
           
      case STAT_WASC :
         if ( vowel ( ch ) || ( ch == 'Y' ) ) {
           status = STAT_WASV; 
         }
         break;
           
      case STAT_WASV :
         if ( ! vowel ( ch ) ) { 
           h ++;
           status = STAT_WASC; 
         }
         break;
    } 
    i ++;
  }
  return ( h );
}
  
  
/****************************************************************
**  HasVowel
**
**  Checks if a vowel occurs in the word stem. 'until' points
**  to the next byte after the last position.
****************************************************************/   
  
BOOL HasVowel
       ( until )
int
  until;
{
  i = 0;
  while ( i < until ) {
    if ( vowel ( *( word + i ) ) ||
         ( ( *( word + i ) == 'Y' ) && ( i > 0 ) && 
           ! vowel ( *( word + i - 1 ) ) ) ) { 
      return ( TRUE ); 
    }
    i ++;
  }
  return ( FALSE );
}
  
  
/****************************************************************
**  TestO
**
**  Checks if stem ends with c1 v c2, where c2 > w,x,y.
**  'until' points to the next byte after the last position.
****************************************************************/   

BOOL TestO
       ( until )
int
  until;
{
  if ( until < 3 ) return ( FALSE );
  h = until - 2;
  ch = *( word + h + 1 );
  if ( ! vowel ( *( word + h - 1 ) ) &&
       ( vowel ( *( word + h ) ) || ( *( word + h ) == 'Y' ) ) &&
       ( ! vowel ( ch ) && 
         ! ( ( ch == 'W' ) || ( ch == 'X' ) || ( ch == 'Y' ) ) ) ) {
    return ( TRUE ); 
  }
  else {
    return ( FALSE );
  }
}
  
  
/****************************************************************
**  HasSuffix
**
**  Checks 'word' for suffix 's' and measure > m.
****************************************************************/   

BOOL HasSuffix
       ( s, m )
char
  *s;
int
  m;
{
  char *
    ch;
  int 
    d;
   
  h = strlen ( s ) - 1;
  if ( h > endw ) return ( FALSE );
  d = endw - h; 
  i = 0;
  ch = word + d;
  
  do {
    if ( *ch != *s ) return ( FALSE );
    i ++;
    s ++;
    ch ++;
  } while ( i <= h );
  return ( MeasureM ( d ) > m );
}
  
  
/****************************************************************
**  Step1b_5c
**
**  Steps 1b to 5c.
****************************************************************/   

void Step1b_5c
       ( )
{
  char
    *temp;
    
    if ( endw >= 3 ) { 
 
    /* GH
     * 
     * LY ->, TY ->
     */
     
    if ( *( word + endw ) == 'Y' ) {
      temp = ( word + endw - 1 );
      if ( ( *temp == 'L' ) || ( *temp == 'T' ) ) {
        endw -= 2;
        *temp = '\0';
      }
    }
    
    /*
     * Step 1b - a:
     * ====================
     * 1: (  m > 0 ) EED  -  >  EE
     * 2: (  *v* / ED   -  > 
     * 3: (  *v* / ING  -  > 
     */

     h = endw; b = FALSE;
     if ( *( word + h ) == 'D' ) {
       h --;
       if ( *( word + h ) == 'E' ) { 
         h --;
         if ( *( word + h ) == 'E' ) {
           if ( MeasureM ( h ) > 0 ) {
             *( word + endw ) = '\0'; 
             endw --;
             ruleSet |= S1b;
           }
         }
         else if ( HasVowel ( h + 1 ) ) { 
           endw --;
           *( word + endw ) = '\0'; 
           endw --;
           ruleSet |= S1b;
           b = TRUE;	     /*2*/
         }
       }
     } 
     else if ( ( *( word + h ) == 'G' ) && 
               ( *( word + h - 1 ) == 'N' ) && 
               ( *( word + h - 2 ) == 'I' ) &&
               HasVowel( h - 2 ) ) {
       *( word + h - 2 ) = '\0'; 
       endw = h - 3; 
       ruleSet |= S1b;
       b = TRUE;		     /*3*/
     }
    }
    
    if ( b && ( endw >= 2 ) ) {	
    
    /* 
     * Step 1b - b: 
     * ====================
     * if 2 od 3:	4: AT  -  >  ATE
     *			5: BL  -  >  BLE
     *			6: IZ  -  >  IZE
     *			7: (  *d and not (  *L or *S or *Z ) )  - >  
     *                     single letter
     *			8: (  m==1 and *o )   -  >  E
     */
     
     h = endw; 
     ch = *( word + h ); 
     ck = *( word + h - 1 );
     if ( ( ( ch == 'T' ) && ( ck == 'A' ) ) ||					     /*4*/
          ( ( ch == 'L' ) && ( ck == 'B' ) ) ||					     /*5*/
          ( ( ch == 'Z' ) && ( ck == 'I' ) ) ) {				     /*6*/
       ++ endw; 
       *( word + endw ) = 'E'; 
       *( word + endw + 1 ) = '\0'; 
       ruleSet |= S1bb;
     } 
     else if ( ( ch == ck ) &&  
               ! ( vowel ( ch ) || 
                   ( ch == 'L' ) || ( ch == 'S' ) || ( ch == 'Z' ) ) ) {
       *( word + h ) = '\0'; 
       endw --;
       ruleSet |= S1bb;
     } 
     else if ( ( MeasureM ( h + 1 ) == 1 ) && TestO( h + 1 ) ) {
       endw ++;
       *( word + endw ) = 'E'; 
       *( word + endw + 1 ) = '\0';
       ruleSet |= S1bb;
     }
    } 
    
    /* Step 1c: 
     * ================
     *  9: (  *v* / Y  -  >  I
     */
     
     if ( ( *( word + endw ) == 'Y' ) && HasVowel( endw ) ) {
       *( word + endw ) = 'I'; 
       ruleSet |= S1c;
     }
     
    if ( endw >= 4 ) {
    
    /* 
     * Step 2: 
     * ==============
     * 1: ( m > 0 ) ATIONAL -  >  ATE	11: ( m > 0 ) IZATION	 -  >  IZE
     * 2: ( m > 0 ) TIONAL -  >  TIO	12: ( m > 0 ) ATION	 -  >  ATE
     * 3: ( m > 0 ) ENCI -  >  ENC	13: ( m > 0 ) ATOR	 -  >  ATE
     * 4: ( m > 0 ) ANCI -  >  ANC	14: ( m > 0 ) ALISM	 -  >  AL
     * 5: ( m > 0 ) IZER -  >  IZE	15: ( m > 0 ) IVENESS	 -  >  IVE
     * 6: ( m > 0 ) ABLI -  >  ABL	16: ( m > 0 ) FULNESS 	 -  >  FUL
     * 7: ( m > 0 ) ALLI -  >  AL	17: ( m > 0 ) OUSNESS	 -  >  OUS
     * 8: ( m > 0 ) ENTL -  >  ENT	18: ( m > 0 ) ALITI	 -  >  AL
     * 9: ( m > 0 ) ELI	 - > E		19: ( m > 0 ) IVITI 	 -  >  IVE
     *10: ( m > 0 ) OUSLI -  >  OUS	20: ( m > 0 ) BILITI	 -  >  BLE
     */
     
     switch ( *( word + endw - 1 ) ) {
       /* testet auf den zweitletzten Buchstaben und teilt auf */

       case 'A' : 
         if ( HasSuffix ( "TIONAL", 0 ) ) {
           ruleSet |= S2;
           if ( ( *( word + endw - 6 ) == 'A' ) && 
                ( MeasureM ( endw - 6 ) > 0 ) ) {
             endw -= 4;
             *( word + endw ) = 'E'; 
             *( word + endw + 1 ) = '\0';    /*1*/
     	   }
     	   else {
     	     endw --;
     	     *( word + endw ) = '\0';  
     	     endw --;
     	   }
         }
         break;
         
       case 'C' : 
         if ( HasSuffix ( "ENCI", 0 ) || HasSuffix ( "ANCI",0 ) ) { 
     	   *( word + endw ) = 'E'; 
     	   ruleSet |= S2;
         }
         break;
         
       case 'E' : 
         if ( HasSuffix ( "IZER", 0 ) ) {
           *( word + endw ) = '\0'; 
           endw --;
     	   ruleSet |= S2;
         }
         break;
         
       case 'L' : 
         if ( HasSuffix ( "ABLI", 0 ) ) {
           *( word + endw ) = 'E';
     	   ruleSet |= S2;
         } 
         else if ( HasSuffix ( "LI", 0 ) ) { 
           endw -= 2;
           if ( HasSuffix ( "AL", 0 ) || HasSuffix ( "ENT", 0 ) ||   /*7,8*/
                HasSuffix ( "E", 0 ) || HasSuffix ( "OUS", 0 ) ) {  /*9,10*/
             *( word + endw + 1 ) = '\0'; 
     	     ruleSet |= S2;
           }
           else {
             endw += 2;
           }
         }
         break;
         
       case 'O' : 
         if ( HasSuffix ( "ATION", 0 ) ) { 
           endw -= 5;
           if ( HasSuffix ( "IZ", 0 ) ) { 
             endw ++;
           }
           else {
             endw += 3;
           }
           *( word + endw ) = 'E'; 
           *( word + endw + 1 ) = '\0';
     	   ruleSet |= S2;
         } 
         else if ( HasSuffix ( "ATOR", 0 ) ) {
           *( word + endw ) = '\0';  
           endw --;
           *( word + endw ) = 'E';    /*13*/
     	   ruleSet |= S2;
         }
         break;
         
       case 'S' : 
         if ( HasSuffix ( "ALISM",0 ) ) {    /*14*/
           endw -= 2;
           *( word + endw ) = '\0'; 
           endw --;
     	   ruleSet |= S2;
         } 
         else if ( HasSuffix ( "NESS", 0 ) ) { 
           endw -= 4;
           if ( HasSuffix ( "IVE", 0 ) || HasSuffix ( "FUL", 0 ) || /*15,16*/
                HasSuffix ( "OUS",  0 ) ) {    /*17*/
             *( word + endw + 1 ) = '\0';
             ruleSet |= S2;
           }
           else {
             endw += 4;
           }
         }
         break;
         
       case 'T' : 
         if ( HasSuffix ( "ITI", 0 ) ) { 
           endw -= 3;
           if ( HasSuffix ( "AL", 0 ) ) { 
             ruleSet |= S2;
           } 
           else if ( HasSuffix ( "IV", 0 ) ) {
             endw ++;
             *( word + endw ) = 'E';  	    /*19*/
             ruleSet |= S2;
           } 
           else if ( HasSuffix ( "BIL", 0 ) ) {	    /*20*/
             *( word + endw ) = 'E'; 
             *( word + endw - 1 ) = 'L';
             ruleSet |= S2;
           }
           else {
             endw += 3;
           }
           *( word + endw + 1 ) = '\0';
         }
         break;
     }
    }

    if (  endw >= 4 ) {	
    
    /*
     * Step 3:
     * ==============
     * 1: ( m > 0 ) ICATE -  >  IC	5: ( m > 0 ) ICAL -  >  IC
     * 2: ( m > 0 ) ATIVE -  >  	6: ( m > 0 ) FUL -  > 
     * 3: ( m > 0 ) ALIZE -  >  L	7: ( m > 0 ) NESS -  > 
     * 4: ( m > 0 ) ICITI -  >  IC	
     */
     
     ch = *( word + endw );
     if ( ch == 'E' ) { 
       endw --;
       if ( HasSuffix ( "ICAT", 0 ) || HasSuffix ( "ALIZ", 0 ) ) {
         endw --;
         *( word + endw ) = '\0'; 
         endw --;
         ruleSet |= S3;
       } 
       else if ( HasSuffix ( "ATIV",0 ) ) {
         endw -= 4; 
         *( word + endw + 1 ) = '\0';      /*2*/
         ruleSet |= S3;
       }
       else {
         endw ++;
       }
     } 
     else if ( ch == 'I' ) { 
       endw --;
       if ( HasSuffix ( "ICIT", 0 ) ) {
         endw --;
         *( word + endw ) = '\0'; 	     /*4*/
         endw --;
         ruleSet |= S3;
       }
       else {
         endw ++;
       }
     } 
     else if ( ch == 'L' ) { 
       endw --;
       if ( HasSuffix ( "ICA", 0 ) ) {
         *( word + endw ) = '\0';
         endw --;		     /*5*/
         ruleSet |= S3;
       } 
       else if ( HasSuffix ( "FU", 0 ) ) {
         endw -= 2; 
         *( word + endw + 1 ) = '\0';     /*6*/
         ruleSet |= S3;
       }
       else {
         endw ++;
       }
     } 
     else if ( ch == 'S' ) { 
       endw --;
       if ( HasSuffix ( "NES", 0 ) ) {
         endw -= 3; 
         *( word + endw + 1 ) = '\0';     /*7*/
         ruleSet |= S3;
       }
       else {
         endw ++;
       }
     }
    } /*Step3*/

    if ( endw >=5 ) {
    
    /*
     * Step 4:
     * ==============
     * 1: (m>1) AL   -  > 	 8: (m>1) ANT	 -  > 	14: (m>1) ISM -  >
     * 2: (m>1) ANCE -  > 	 9: (m>1) EMENT	 -  >  	15: (m>1) ATE -  > 
     * 3: (m>1) ENCE -  > 	10: (m>1) MENT	 -  > 	16: (m>1) ITI -  > 
     * 4: (m>1) ER	 -  > 	11: (m>1) ENT	 -  > 	17: (m>1) OUS -  > 
     * 5: (m>1) IC	 -  > 	12: (m>1) and (  *S OR	18: (m>1) IVE -  > 
     * 6: (m>1) ABLE -  > 	          *T ) ) ION  -  > 	19: (m>1) IZE	 -  > 
     * 7: (m>1) IBLE -  > 	13: (m>1) OU	 -  > 
     * 
     *  ***ERG3***	  1a: (m>1) IAL   -  > 	 1b: (m>1) UAL   -  > 
     *			 15a: (m>1) IATE  -  > 	15b: (m>1) UATE  -  > 
     */

     switch ( *( word + endw - 1 ) ) {
     
       case 'A' : 
         if ( ( *( word + endw ) == 'L' ) && ( MeasureM ( endw - 1 ) > 1 ) ) {
           /***ERG3***/
           if ( ( *( word + endw - 2 ) == 'I' ) || 
                ( *( word + endw - 2 ) == 'U' ) ) { 	 /*1a,1b*/
     	     endw --;
           }
           endw --; 
           *( word + endw ) = '\0';
           endw --; 
           ruleSet |= S4;
         }
         break;
         
       case 'C' : 
         if ( HasSuffix ( "ANCE", 1 ) || HasSuffix ( "ENCE",1 ) ) {
     	   endw -= 4; 
     	   *( word + endw + 1 ) = '\0';	   /*2,3*/
           ruleSet |= S4;
     	 }
     	 break;
     	 
       case 'E' : 
         if ( ( *( word + endw ) == 'R' ) && 
              ( MeasureM ( endw - 1 ) > 1 ) ) {
           endw --; 
           *( word + endw ) = '\0'; 
           endw --;   /*4*/
           ruleSet |= S4;
         }
         break;
         
       case 'I' : 
         if ( ( *( word + endw ) == 'C' ) && ( MeasureM ( endw - 1 ) > 1 ) ) {
     	   endw --; 
     	   *( word + endw ) = '\0'; 
     	   endw --;   /*5*/
           ruleSet |= S4;
         }
         break;
         
       case 'L' : 
         if ( HasSuffix ( "ABLE", 1 ) || HasSuffix ( "IBLE", 1 ) ) {
           endw -= 4; 
           *( word + endw + 1 ) = '\0'; 	   /*6,7*/
           ruleSet |= S4;
         }
         break;
         
       case 'N' : 
         if ( *( word + endw ) == 'T' ) { 
           endw -= 2;
       	   if ( HasSuffix ( "EME", 1 ) ) { 
       	     endw -= 3;
             ruleSet |= S4; /*9*/
       	   } 
       	   else if ( HasSuffix ( "ME", 1 ) ) { 
       	     endw -= 2;
             ruleSet |= S4; /* 10 */
       	   } 
       	   else if ( ( ( *( word + endw ) == 'A' ) || 
       	               ( *( word + endw ) == 'E' ) ) &&	  /*8,11*/
       		     ( MeasureM ( endw )  >  1 ) ) { 
             endw --;
             ruleSet |= S4;
           }
       	   else {
       	     endw += 2;
       	   }
       	   *( word + endw + 1 ) = '\0';
         }
         break;
         
       case 'O' : 
         if ( HasSuffix ( "ION",1 ) ) { 
           endw -= 3;
       	   if ( ( *( word + endw ) == 'S' ) || ( *( word + endw ) == 'T' ) ) {
             *( word + endw + 1 ) = '\0';     /*12*/
             ruleSet |= S4;
           }
       	   else {
       	     endw += 3;
           }
         } 
         else if ( ( *( word + endw ) == 'U' ) && 
                   ( MeasureM ( endw - 1 )  >  1 ) ) {
           endw --; 
           *( word + endw ) = '\0'; 
           endw --;  /*13*/
           ruleSet |= S4;
         }
         break;
         
       case 'S' : 
         if ( HasSuffix ( "ISM",1 ) ) {
     	   endw -= 3;
     	   *( word + endw + 1 ) = '\0';    /*14*/
           ruleSet |= S4;
         }
         break;
         
       case 'T' : 
         if ( HasSuffix ( "ATE", 1 ) || HasSuffix ( "ITI", 1 ) ) {
     	   endw -= 3;
     	   /***ERG3***/
     	   if ( ( *( word + endw + 1 ) =='A' ) && 
     	        ( ( *( word + endw ) =='I' ) || ( *( word + endw ) =='U' ) ) ) {
     	     endw --;       /*15a,15b*/
           }
           *( word + endw + 1 ) = '\0'; 
           ruleSet |= S4;		 /*15,16*/
         }
         break;
         
       case 'U' : 
         if ( HasSuffix ( "OUS",1 ) ) {
     	   endw -= 3;
           *( word + endw + 1 ) = '\0'; 
           ruleSet |= S4;		 /* 17 */
         }
         break;
         
       case 'V' : 
         if ( HasSuffix ( "IVE", 1 ) ) {
     	   endw -= 3;
           *( word + endw + 1 ) = '\0'; 
           ruleSet |= S4;		 /* 18 */
         }
         break;

       case 'Z' : 
         if ( HasSuffix ( "IZE", 1 ) ) {
     	   endw -= 3;
           *( word + endw + 1 ) = '\0'; 
           ruleSet |= S4;  /*19*/
         }
         break;
     }
    } /*Step4*/

    if (  endw >= 2 ) {	
    
    /* 
     * Step 5a - c:
     * ====================
     * a: 1: ( m > 1 ) E          -  > 		
     *    2: ( m==1 and not *o ) E   -  > 
     * 
     * b: 3: ( m > 1 and *d and *L )  -  > 
     *
     * c: 4: I  -  >  Y                     ***ERG1***
     */

     if ( *( word + endw ) == 'E' ) { 
       i = MeasureM ( endw );
       if ( ( i > 1 ) || ( ( i==1 ) && ! TestO( endw ) ) ) {
         *( word + endw ) = '\0'; 
         endw --; 
         ruleSet |= S5a;  /* 1, 2 */
       }
     }

     if ( ( MeasureM ( endw ) > 1 ) ) { 
       ch = *( word + endw );
       if ( ( ch == 'L' ) && ( ch == *( word + endw - 1 ) ) ) {
         *( word + endw ) = '\0';
         endw --;
         ruleSet |= S5b;  /* 3 */
       }
     }

     if ( ruleSet != 0 ) {	  /* ***ERG1*** */
       if ( *( word + endw ) == 'I' ) {
         *( word + endw ) = 'Y'; 	     /*5*/
         ruleSet |= S5c; 
       }
       if ( ruleSet == S1c | S5c ) { 
         ruleSet = 0;
       }
     }
    }
 }
    
    
/****************************************************************
**  StemEnglishWord
**
**  IN  : 'w' is a pointer to a null-terminated English word.
**        The word must be in uppercase letters.
**  OUT : 'w' contains the root.
****************************************************************/   

void StemEnglishWord
       ( w )
char
  *w;
{
   endw = strlen ( w );
   if ( ( endw > MAX_WORDLEN ) || ( endw < 3 ) ) return;
   endw --;
   word = w;
   
   ruleSet = 0; 
   Step1a ();	/* Stop after here if you only want to stem plurals */
#ifdef FULL_PORTER
   Step1b_5c (); 
   ruleSet = 0;
   Step1b_5c (); 
#endif
}
