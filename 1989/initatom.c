/****************************************************************
*
*           S O F T W A R E   S O U R C E   F I L E
*
*****************************************************************
*
*   Name of file   : initatom.c
*   Author         : Guido Hoss
*   Project        : ETH Diploma Thesis (SS 1989)
*   Creation Date  : 01/05/89
*   Type of file   : C Language File
*
*   Description
*   -----------
*   This program initializes the atomic weights file. This 
*   process is very simple, since we only need to count the number
*   of documents for each atomic concept in the <atom-docs> file.
*
*   Call Format
*   -----------
*	init_atomwgts <atom-docs>
*
*   where <atom-docs> is the file which contains the documents, 
*   indexed by atomic concept. The results are written to the
*   standard output.
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

#define PROG	"Initialization of Atomic Weights (gh, 01/05/89)\n"
#define USAGE	"Usage: init_atomwgts <atom-docs> [QUIET]\n"

#include <stdio.h>
#include <string.h>
#include <math.h>
#ifdef MSDOS
#include <process.h>
#endif
#include <assert.h>

#include "boolean.h"
#include "util.h"

#define LINE_LENGTH	100


double
  log_2;
FILE
  *counter;	/* Virtual file used to output running counts */


/****************************************************************
**  Forward declarations (compiler type checking)
****************************************************************/   

#ifndef BSDUNIX
int main ( int, char *[] );
void read_concepts ( FILE * );
double calc_idf ( double, int );
#else
int main ();
void read_concepts ();
double calc_idf ();
#endif


/****************************************************************
**  calc_idf
**
**  Calculates the inverse document frequency based on the
**  total number of documents and the document frequency.
**
**  IN  : total = total number of documents
**        df    = document frequency of atomic concept
**
**  OUT : IDF ( atomic concept )
****************************************************************/ 

double calc_idf
         ( total, df )
double
  total;
int
  df;
{
  double
    t;

  if ( df != 0 ) {
    t = log ( total / (double) df ) / log_2;
    return ( t * t );
  }
  else {
    return ( 0.0 );
  }
}


/****************************************************************
**  read_concepts
**
**  Reads the documents containing each atomic concept and
**  counts them.
**
**  IN  : f = handle to opened atom-idf file.
****************************************************************/ 

void read_concepts
       ( f )
FILE
  *f;
{
  BOOL
    first = TRUE,
    ok;
  char
    line [ LINE_LENGTH ];
  double
    totaldocs;
  int
    d, curratom,
    df, n, res;

  /* The first line of the file must contain the number of documents */
  ok = ( fgets ( line, LINE_LENGTH, f ) != NULL );
  assert ( ok );
  res = sscanf ( line, " %d", &n );
  assert ( res == 1 );
  fprintf ( counter, "%d document(s)\n", n );
  totaldocs = (double) n;
  
  /* Read file line by line */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    /* Read a number */
    n = sscanf ( line, " %d", &d );
    assert ( n == 1 );
    
    /* If number followed by a colon, then this is an atomic concept */
    if ( strchr ( line, ':' ) != NULL ) {
      if ( ! first ) {
        /* don't do anything after first document index */
        printf ( "%d\t%f\n", curratom, calc_idf ( totaldocs, df ) );
        fprintf ( counter, "%d\r", curratom );
      }
      else {
        first = FALSE;
      }
      curratom = d;
      df = 0;
    }
    else {
      /* no colon -> this is a document index; increase count */
      df ++;
    }
  }
  /* Process last document */
  printf ( "%d\t%f\n", curratom, calc_idf ( totaldocs, df ) );
  
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
  log_2 = log ( 2.0 );

  /* Get verbose or quiet mode */
  if ( ( argc == 3 ) && ( *argv [2] == 'Q' ) ) {
    /* in case of quiet mode: redirect running counts to /dev/null */
    counter = fopen ( "/dev/null", "r" );
    assert ( counter != NULL );
  }
  else {
    /* verbose mode: redirect running counts to stderr */
    counter = stderr;
  }

  /* Check arguments */
  if ( argc < 2 ) {
    fprintf ( stderr, USAGE );
    return ( 1 );
  }
  
  /* Open idf file */
  f = open_file ( argv [1] );
  
  /* Process */
  fprintf ( stderr, "Processing concepts.\n" );
  read_concepts ( f );
  fclose ( f );

  return ( 0 );
}
