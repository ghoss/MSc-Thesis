/****************************************************************
*
*           S O F T W A R E   S O U R C E   F I L E
*
*****************************************************************
*
*   Name of file   : generate_ida.c
*   Author         : Guido Hoss
*   Project        : ETH Diploma Thesis (SS 1989)
*   Creation Date  : 29/04/89
*   Type of file   : C Language File
*
*   Description
*   -----------
*   Builds an initial domain algebra from a document description.
*   Creates a SIGNS file containing all single words in the
*   document description. The empty situations and abstraction
*   files are not created by this program; they must be created
*   by the shell.
*
*   Call Format
*   -----------
*
*      generate_ida <doc-freq>
*
*   where <doc-freq> is the path of the document term frequency
*   file. The signs are written to the standard output.
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
*   Git repository home: <https://github.com/ghoss/MSc-Thesis>
*
*****************************************************************
* Date        :
* Description :
****************************************************************/   

#define PROG	"Initial Domain Algebra Generation (gh, 29/04/89)\n"
#define USAGE	"Usage: generate_ida <doc-freq> [QUIET]\n"

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#ifdef MSDOS
#include <process.h>
#endif
#include <assert.h>

#include "boolean.h"
#include "list.h"
#include "util.h"

#define MAX_WORDLEN	100
#define LINE_LENGTH	100


LIST
  wordlist;

FILE
  *counter;	/* Virtual file used to output running counts */


/****************************************************************
**  Forward declarations (compiler type checking)
****************************************************************/ 

#ifndef BSDUNIX
int main ( int, char* [] );
BOOL enumproc ( ELEMENT );
#else
int main ();
BOOL enumproc ();
#endif


/****************************************************************
**  enumproc
**
**  Enumeration procedure used to print out a list of all signs.
**  Called by 'enum_list'.
****************************************************************/

BOOL enumproc
       ( s )
ELEMENT
  s;
{
  static int
    index = 0;

  printf ( "%d\t%s\n", index, (char *) s );
  index ++;
  return ( TRUE );
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
    *doc_descr;
  int
    n, doc;
  char
    line [ LINE_LENGTH ],
    w1 [ MAX_WORDLEN ],
    w2 [ MAX_WORDLEN ];
  ELEMENT
    res;

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

  /* Check if number of parameters ok */
  if ( argc < 2 ) {
    fprintf ( stderr, USAGE );
    return ( 1 );
  }
  
  /* Open document description file */
  fprintf ( stderr, "Reading document description.\n" );
  doc_descr = open_file ( argv [1] );

  /* Create word list */
  wordlist = create_list ();
  assert ( wordlist != NULL );
  
  
  /* Read input file line by line */
  while ( fgets ( line, LINE_LENGTH, doc_descr ) ) {

    /* Try to interpret text; sscanf will assign as much fields as possible */
    n = sscanf ( line, " %d %s %s", &doc, w1, w2 );
    
    switch ( n ) {
    
      case 1 :
        /* Index of new document */
        fprintf ( counter, "%d\r", doc );
        break;
        
      case 2 :
        /* Frequency and one word; add word to list */
#ifndef BSDUNIX
        res = add_list ( wordlist, duplicate ( w1 ), 
                (int (*) ( ELEMENT, ELEMENT )) strcmp );
#else
        res = add_list ( wordlist, duplicate ( w1 ), strcmp );
#endif
        assert ( res != NULL );
        break;
      
      case 3 :
        /* Frequency and two words; we are not interested in those */
        break;
    }
  }

  /* Enumerate all words in list */
  enum_list ( wordlist, enumproc, ENUM_FORWARD );
  
  fclose ( doc_descr );
  fprintf ( stderr, "\nsigns: %d\n", count_list ( wordlist ) );
  destroy_list ( &wordlist );
  return ( 0 );
}
