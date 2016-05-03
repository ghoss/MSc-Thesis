/****************************************************************
*
*           S O F T W A R E   S O U R C E   F I L E
*
*****************************************************************
*
*   Name of file   : calcswgt.c
*   Author         : Guido Hoss
*   Project        : ETH Diploma Thesis (SS 1989)
*   Creation Date  : 29/04/89
*   Type of file   : C Language File
*
*   Description
*   -----------
*   Calculates the weight of each sign in a domain algebra based
*   on the document description and a list of signs. The weight
*   is calculated according to the formula
*
*	wgt(d,t) = tf(d,t) / sqrt(sum(1..N, tf(d,t[i]) * idf(t[i])))
*
*   where tf(d,t) is the term frequency of t in d, and idf(t)
*   is the inverse document frequency of t.
*
*   Call Format
*   -----------
*	calc_docdescr <doc_freq> <sign_file>
*
*   where <doc_freq> is the name of the document term frequency
*   file and <sign_file> is the name of the sign file.
*   The calculated weights are written to the standard output.
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

#define PROG	"Sign Weight Calculation (gh, 29/04/89)\n"
#define USAGE	"Usage: calc_docdescr <doc_freq> <sign_file> [QUIET]\n"

#include <ctype.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <math.h>
#ifdef MSDOS
#include <process.h>
#endif
#include <assert.h>

#include "boolean.h"
#include "list.h"
#include "util.h"

#define MAX_WORDLEN	100
#define LINE_LENGTH	100


typedef
  struct {
    int    idx;     /* sign index */
    char   *term;   /* term interpretation of sign t=a(s) */
    int    df;      /* document frequency df(t) */
    double idf;     /* inverse document frequency idf(t) */
  } SIGN_STRUCT;

typedef
  struct {
    int sign;      /* sign index */
    int freq;      /* term frequency */
  } WGT_STRUCT;


LIST
  doc_signs = NULL,
  sign_list;
int
  docnum = 0;   /* Number of documents in collection */
double
  norm = 0.0;   /* global variable used by enum_wgts */
FILE
  *counter;	/* Virtual file used to output running counts */
  
  
/****************************************************************
**  Forward declarations (compiler type checking)
****************************************************************/   

#ifndef BSDUNIX
int main ( int, char *[] );
void load_signs ( FILE * );
int comp_sign ( ELEMENT, ELEMENT );
void read_descr ( FILE *, BOOL );
void dump_weights ( int );
BOOL enum_wgts ( ELEMENT );
int comp_wgts ( ELEMENT, ELEMENT );
SIGN_STRUCT *find_sign ( char * );
WGT_STRUCT *find_wgt ( int );
#else
int main ();
void load_signs ();
int comp_sign ();
void read_descr ();
void dump_weights ();
BOOL enum_wgts ();
int comp_wgts ();
SIGN_STRUCT *find_sign ();
WGT_STRUCT *find_wgt ();
#endif


/****************************************************************
**  load_signs
**
**  Loads all signs from the sign file into memory.
**
**  IN  : f = Handle to open sign file.
**
**  OUT : Global variable 'sign_list' is filled.
****************************************************************/   

int comp_sign 
      ( s1, s2 )
ELEMENT
  s1;
ELEMENT
  s2;
{
  /* Order of sign structures = lexicographical order of strings */
  return ( strcmp ( ((SIGN_STRUCT *) s1) -> term,
                    ((SIGN_STRUCT *) s2) -> term ) );
}


void load_signs 
       ( f )
FILE
  *f;
{
  int
    res;
  char
    line [ LINE_LENGTH ],
    *s;
  SIGN_STRUCT
    *elt;

  /* Create sign list */
  sign_list = create_list ();
  assert ( sign_list != NULL );
  
  /* Read sign file one line at a time */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    /* Get rid of trailing newline character (also included in string) */
    line [ strlen ( line ) - 1 ] = '\0';
    
    /* Create new list element for this sign */
    elt = (SIGN_STRUCT *) malloc ( sizeof ( SIGN_STRUCT ) );
    assert ( elt != NULL );
    elt -> df = 0;
    elt -> idf = 0.0;

    /* Extract sign index */
    res = sscanf ( line, " %d", &( elt -> idx ) );
    assert ( res == 1 );
    
    /* Extract term interpretation (skip line to beginning of word) */
    s = line;
    while ( ! isalpha ( *s ) ) {
      s ++;
    }
    elt -> term = duplicate ( s );
    
    /* Add 'elt' to list, sorted by terms */
    elt = (SIGN_STRUCT *) add_list ( sign_list, (ELEMENT) elt, comp_sign );
    assert ( elt != NULL );
    
    /* Running count */
    fprintf ( counter, "%d\r", elt -> idx );
  }
  fprintf ( counter, "\n" );
}


/****************************************************************
**  dump_weights
**
**  Prints out weight of each term in 'doc_signs'.
**
**  IN  : currdoc = number of current document.
****************************************************************/ 

BOOL enum_wgts
       ( w )
ELEMENT
  w;
{
  WGT_STRUCT
    *wg;
  double 
    wgt;

  /* Print sign value followed by weight */
  wg = (WGT_STRUCT *) w;
  
  if ( wg -> freq > 0 ) {
    wgt = (double) ( wg -> freq ) / norm;
    printf ( "\t%d\t%f\n", wg -> sign, wgt );
  }
  return ( TRUE );
}


void dump_weights
       ( currdoc )
int 
  currdoc;
{
  norm = sqrt ( norm );
  if ( doc_signs != NULL ) {
    printf ( "%d\n", currdoc );
  
    /* List all terms with non-zero weight */
    enum_list ( doc_signs, enum_wgts, ENUM_FORWARD );
  
    /* Clear list for next document */
    destroy_list ( &doc_signs );
  }
  
  doc_signs = create_list ();
  assert ( doc_signs != NULL );
}


/****************************************************************
**  find_sign
**
**  Finds the entry of the specified sign in 'sign_list'.
**
**  IN  : s = character representation of a sign.
**
**  OUT : The function returns a pointer to the sign's entry
**        or NULL if the sign is not in 'sign_list'.
****************************************************************/ 

SIGN_STRUCT *find_sign
               ( s )
char
  *s;
{
  SIGN_STRUCT
    temp, *sgn;

  temp.term = s;
  return ( (SIGN_STRUCT *) lookup_list ( sign_list, (ELEMENT) &temp,
                                         comp_sign ) );
}


/****************************************************************
**  find_wgt
**
**  Finds the entry of the specified sign in 'doc_signs'.
**
**  IN  : s = number of a sign.
**
**  OUT : The function returns a pointer to the sign's entry.
**        If the sign did not exist, it is created automatically.
****************************************************************/ 

WGT_STRUCT *find_wgt
              ( s )
int
  s;
{
  WGT_STRUCT
    *wgt;

  wgt = (WGT_STRUCT *) malloc ( sizeof ( WGT_STRUCT ) );
  assert ( wgt != NULL );
  wgt -> sign = s;
  wgt -> freq = 0;
  wgt = (WGT_STRUCT *) add_list ( doc_signs, (ELEMENT) wgt,
                                  comp_wgts );
  assert ( wgt != NULL );
  return ( wgt );
}


/****************************************************************
**  read_descr
**
**  Read document description file. Depending on mode, either
**  calculates df/idf or weights.
**
**  IN  : f    = Handle to open document description file.
**        mode = FALSE --> Calculate df/idf values.
**               TRUE  --> Calculate and print weights.
****************************************************************/ 

int comp_wgts
      ( w1, w2 )
ELEMENT
  w1;
ELEMENT 
  w2;
{
  int
    a, b;
    
  /* order pairs ( sign, term freq ) by sign index */
  a = ((WGT_STRUCT *) w1 ) -> sign;
  b = ((WGT_STRUCT *) w2 ) -> sign;
  
  return ( a - b );
}


void read_descr
       ( f, mode )
FILE
  *f;
BOOL
  mode;
{
  char
    w1 [ MAX_WORDLEN ],
    w2 [ MAX_WORDLEN ],
    line [ LINE_LENGTH ];
  SIGN_STRUCT
    *part1, *part2,
    *elt;
  WGT_STRUCT
    *wgt;
  int
    n, currdoc, doc;
  double
    log2, nrm;

  log2 = log ( 2.0 );

  /* Read input file line by line */
  while ( fgets ( line, LINE_LENGTH, f ) ) {

    /* Try to interpret text; sscanf will assign as much fields as possible */
    n = sscanf ( line, " %d %s %s", &doc, w1, w2 );
    
    switch ( n ) {
    
      case 1 :
        /* Index of new document in 'doc' */
        fprintf ( counter, "%d\r", doc );
        if ( ! mode ) {
          /* idf/df values need number of documents */
          docnum ++;
        }
        else {
          /* weight calculation; dump weights of previous doc and clear */
          dump_weights ( currdoc );
          norm = 0.0;
          currdoc = doc;
        }
        break;
        
      case 2 :
      case 3 :
        /* Frequency in 'doc' and one or two words */
        if ( n == 3 ) {
          /* two words; concatenate them */
          part1 = find_sign ( w1 );
          part2 = find_sign ( w2 );
          strcat ( w1, " " );
          strcat ( w1, w2 );
        }
        
        /* we now have a word in w1; make dummy sign structure */
        elt = find_sign ( w1 );
        if ( ! mode ) {
          /* idf/df calculation */
          if ( elt != NULL ) {
            /* w1 exists as a sign; increase document frequency */
            elt -> df ++;
	    if ( n == 3 ) {
	      /* Decrease document frequency of individual components */
	      assert ( part1 != NULL );
	      assert ( part2 != NULL );
	      part1 -> df --;
	      part2 -> df --;
	    }
          }
        }
        else {
          /* weight calculation; add term to list for this document */
          if ( elt != NULL ) {
          
            /* idf already calculated? */
            if ( ( elt -> df > 0 ) && ( elt -> idf == 0.0 ) ) {
              elt -> idf = log ( (double) docnum / (double) elt -> df ) / log2;
            }
            
            /* Calculate norm(d) of current document */
            nrm = (double) doc * elt -> idf;
            norm += nrm * nrm;
            
            /* Add term */
            wgt = find_wgt ( elt -> idx );
            wgt -> freq += doc;
            if ( n == 3 ) {
              /* This was a multiword term, decrement freqs of
                 individual components */
              wgt = find_wgt ( part1 -> idx );
              wgt -> freq -= doc;
              wgt = find_wgt ( part2 -> idx );
              wgt -> freq -= doc;
            }
          }
        }
        break;
    }
  }
  /* Dump last document */
  if ( mode ) {
    dump_weights ( currdoc );
  }
  fprintf ( counter, "\n" );
}

  
/****************************************************************
**  main
****************************************************************/   

int main
      ( argc, argv )
int
  argc;
char
  *argv [];
{
  FILE
    *f;

  /* Program title */
  fprintf ( stderr, PROG );

  /* Get verbose or quiet mode */
  if ( ( argc == 4 ) && ( *argv [3] == 'Q' ) ) {
    /* in case of quiet mode: redirect running counts to /dev/null */
    counter = fopen ( "/dev/null", "r" );
    assert ( counter != NULL );
  }
  else {
    /* verbose mode: redirect running counts to stderr */
    counter = stderr;
  }

  /* Check arguments */
  if ( argc < 3 ) {
    fprintf ( stderr, USAGE );
    return ( 1 );
  }
  
  /* Open sign file */
  f = open_file ( argv [2] );
  
  /* Load signs into memory */
  fprintf ( stderr, "Loading signs.\n" );
  load_signs ( f );
  fclose ( f );
  
  /* Open document description */
  f = open_file ( argv [1] );
  
  /* Read document description the first time */
  fprintf ( stderr, "Calculating df and idf values.\n" );
  read_descr ( f, FALSE );
  
  /* Read document description again */
  fprintf ( stderr, "Calculating weights.\n" );
  rewind ( f );
  read_descr ( f, TRUE );
  
  fclose ( f );
  return ( 0 );
}
