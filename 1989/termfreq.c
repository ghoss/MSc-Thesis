/****************************************************************
*
*           S O F T W A R E   S O U R C E   F I L E
*
*****************************************************************
*
*   Name of file   : termfreq.c
*   Author         : Guido Hoss
*   Project        : ETH Diploma Thesis (SS 1989)
*   Creation Date  : 06/05/89
*   Type of file   : C Language File
*
*   Description
*   -----------
*   Reads a list of RSV values for queries and documents and
*   generates satisfied and unsatisfied preferences, which are
*   required by the optimization algorithm.
*
*   Call Format
*   -----------
*	termfreq <DOC-DESCR> [QUIET]
*
*   If the string QUIET is specified as the last parameter,
*   running counts are not written to the screen.
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

#define PROG	"Term Frequency Calculation (09/08/89, gh)\n"
#define USAGE	"termfreq <doc-descr> [QUIET]\n"

#define LINE_LENGTH	100

#include <stdio.h>
#include <assert.h>
#include <malloc.h>

#include "boolean.h"
#include "list.h"
#include "util.h"


typedef
  struct {
    int   sign;	/* Sign number */
    float freq;	/* Document frequency of sign */
  } FREQ_STRUCT;
 
 
FILE
  *counter;	/* Virtual file used to output running counts */
float
  numdocs;	/* Number of documents in collection */
LIST
  table;	/* Term frequency table */
  
  
/****************************************************************
**  load_signs
**
**  Reads sign weights and updates term frequency for each
**  sign.
**
**  IN  : f = handle to open doc-descr file.
****************************************************************/   

int comp_freq
      ( e1, e2 )
ELEMENT
  e1;
ELEMENT
  e2;
{
  return ( ((FREQ_STRUCT *) e1) -> sign - ((FREQ_STRUCT *) e2) -> sign );
}
 
 
void load_signs 
       ( f )
FILE 
  *f;
{
  char
    line [ LINE_LENGTH ];
  float 
    w;
  int
    n, d;
  FREQ_STRUCT
    *freq;
    
  /* Create document list */
  table = create_list ();
  assert ( table != NULL );

  numdocs = 0.0; 
  
  /* Read file line by line */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    /* Interpret as much fields as possible */
    n = sscanf ( line, " %d %f", &d, &w );
    if ( n == 2 ) {
      /* Two fields -> sign index and weight */
           
      /* Find sign in sign_list */
      freq = (FREQ_STRUCT *) malloc ( sizeof ( FREQ_STRUCT ) );
      assert ( freq != NULL );
      freq -> sign = d;
      freq -> freq = 0.0;
      
      /* Structure will be destroyed automatically if sign already in list */
      freq = (FREQ_STRUCT *) add_list ( table, (ELEMENT) freq, comp_freq );
      assert ( freq != NULL );
      
      freq -> freq ++;
    }
    else {
      /* document number; running count */
      fprintf ( counter, "%d\r", d );
      numdocs ++;
    }
  }
  
  fprintf ( counter, "\n" );
}


/****************************************************************
**  output_freq
**
**  Output the contents of the distribution table.
****************************************************************/   

BOOL enum_freq
       ( e )
ELEMENT
  e;
{
  /* Only print term frequency, not sign */
  printf ( "%f\n", ((FREQ_STRUCT *) e ) -> freq / numdocs );
  return ( TRUE );
}


void output_freq
       ( )
{
  enum_list ( table, enum_freq, ENUM_FORWARD );
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
  
  /* Open sign file */
  f = open_file ( argv [1] );
  
  /* Load signs into memory */
  fprintf ( stderr, "Building distribution table.\n" );
  load_signs ( f );
  fclose ( f );

  fprintf ( stderr, "Starting output.\n" );
  output_freq ();  

  return ( 0 );
}
