/****************************************************************
*
*           S O F T W A R E   S O U R C E   F I L E
*
*****************************************************************
*
*   Name of file   : atomdocs.c
*   Author         : Guido Hoss
*   Project        : ETH Diploma Thesis (SS 1989)
*   Creation Date  : 01/05/89
*   Type of file   : C Language File
*
*   Description
*   -----------
*   For each atomic concept 'a', this program generates a list
*   of all documents where 'a' occurs.
*
*   Call Format
*   -----------
*	calc_atomdocs <sign-weights> <concepts>
*
*   where <sign-weights> is the name of the file where the sign
*   weights are listed, and <concepts> is the name of the file
*   containing the list of atomic concepts. The resulting list
*   is written to the standard output.
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

#define PROG	"Atomic Concepts -> Doc List Generation (gh, 01/05/89)\n"
#define USAGE   "Usage: calc_atomdocs <doc-descr> <concepts> [QUIET]\n"

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

#define LINE_LENGTH	100

typedef
  struct {
    int		sign;     /* sign index */
    LIST	docs;     /* list of documents where this sign occurs */
  } SIGN_STRUCT;

typedef
  struct {
    int		atom;     /* atom index */
    LIST	signs;    /* list of signs where this atom occurs */
  } ATOM_STRUCT;

typedef
  struct {
    int		doc;      /* document number */
  } DOC_STRUCT;

typedef
  struct {
    int		sign;      /* sign index */
  } SIGN_PTR;


LIST
  temp_list,
  atom_list,   /* list of all atoms, element type ATOM_STRUCT */
  sign_list;   /* list of all signs, element type SIGN_STRUCT */

FILE
  *counter;	/* Virtual file used to output running counts */


/****************************************************************
**  Forward declarations (compiler type checking)
****************************************************************/   

#ifndef BSDUNIX
int main ( int, char *[] );
void read_weights ( FILE * );
void handle_concepts ( FILE * );
void calc_results ( void );
void add_doc ( LIST, int );
int comp_doc ( ELEMENT, ELEMENT );
int comp_signs ( ELEMENT, ELEMENT );
int comp_atom ( ELEMENT, ELEMENT );
int comp_sptr ( ELEMENT, ELEMENT );
BOOL copy_docs ( ELEMENT );
BOOL enum_signs ( ELEMENT );
BOOL enum_docs ( ELEMENT );
BOOL enum_atoms ( ELEMENT );
#else
int main ();
void read_weights ();
void handle_concepts ();
void calc_results ();
void add_doc ();
int comp_doc ();
int comp_signs ();
int comp_atom ();
int comp_sptr ();
BOOL copy_docs ();
BOOL enum_signs ();
BOOL enum_docs ();
BOOL enum_atoms ();
#endif


/****************************************************************
**  add_doc
**
**  Adds a document index to a list.
**
**  IN  : d    = document index.
**        list = list where d should be added.
****************************************************************/ 

int comp_doc
      ( d1, d2 )
ELEMENT
  d1;
ELEMENT
  d2;
{
  return ( ((DOC_STRUCT *) d1) -> doc - ((DOC_STRUCT *) d2) -> doc );
}


void add_doc
       ( list, d )
LIST
  list;
int
  d;
{
  DOC_STRUCT
    *elt;
    
  elt = (DOC_STRUCT *) malloc ( sizeof ( DOC_STRUCT ) );
  assert ( elt != NULL );
  elt -> doc = d;
  elt = (DOC_STRUCT *) add_list ( list, (ELEMENT) elt, comp_doc );
  assert ( elt != NULL );
}


/****************************************************************
**  read_weights
**
**  IN  : f = handle to opened weight file.
****************************************************************/ 
  
int comp_signs
      ( s1, s2 )
ELEMENT
  s1;
ELEMENT
  s2;
{
  return ( ((SIGN_STRUCT *) s1) -> sign - ((SIGN_STRUCT *) s2) -> sign );
}


void read_weights
       ( f )
FILE 
  *f;
{
  int
    totaldocs = 0,
    currdoc,
    n, d;
  double
    w;
  SIGN_STRUCT
    *sgn;
  char
    line [ LINE_LENGTH ];

  /* Create sign list */
  sign_list = create_list ();
  assert ( sign_list != NULL );

  /* Read file line by line */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    /* Interpret as much fields as possible */
    n = sscanf ( line, " %d %f", &d, &w );
    switch ( n ) {
      
      case 1 :
        /* One field -> new document number; update running count */
	totaldocs ++;
        currdoc = d;
        fprintf ( counter, "%d\r", currdoc );
        break;
        
      case 2 :
        /* Two fields -> sign index and weight; add doc to list of sign */
        sgn = (SIGN_STRUCT *) malloc ( sizeof ( SIGN_STRUCT ) );
        assert ( sgn != NULL );
        sgn -> sign = d;
        sgn -> docs = NULL;
        sgn = (SIGN_STRUCT *) add_list ( sign_list, (ELEMENT) sgn, 
                                         comp_signs );
        assert ( sgn != NULL );
        if ( sgn -> docs == NULL ) {
          /* Create new document list */
          sgn -> docs = create_list ();
          assert ( sgn -> docs != NULL );
        }
        add_doc ( sgn -> docs, currdoc );
        break;
    }
  }
  
  /* The total number of documents is the first line in the output */
  printf ( "%d documents\n", totaldocs );

  fprintf ( counter, "\n" );
}


/****************************************************************
**  handle_concepts
**
**  IN  : f = handle to opened concept file.
****************************************************************/ 
  
int comp_atom
      ( s1, s2 )
ELEMENT
  s1;
ELEMENT
  s2;
{
  return ( ((ATOM_STRUCT *) s1) -> atom - ((ATOM_STRUCT *) s2) -> atom );
}


int comp_sptr
      ( s1, s2 )
ELEMENT
  s1;
ELEMENT
  s2;
{
  return ( ((SIGN_PTR *) s1) -> sign - ((SIGN_PTR *) s2) -> sign );
}


void handle_concepts
       ( f )
FILE 
  *f;
{
  ATOM_STRUCT
    *atm;
  SIGN_PTR
    *sgn;
  int
    n, d, curr_sign;
  char
    line [ LINE_LENGTH ];

  /* Create list of atomic concepts */
  atom_list = create_list ();
  assert ( atom_list != NULL );

  /* Read file line by line */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    /* Read a number */
    n = sscanf ( line, " %d", &d );
    assert ( n == 1 );
    
    /* If number followed by a colon, then this is a sign index */
    if ( strchr ( line, ':' ) != NULL ) {
      curr_sign = d;
      fprintf ( counter, "%d\r", d );
    }
    else {
      /* no colon -> this is an atomic concept; add current sign to
         list of this concept */
      atm = (ATOM_STRUCT *) malloc ( sizeof ( ATOM_STRUCT ) );
      assert ( atm != NULL );
      atm -> atom = d;
      atm -> signs = NULL;
      atm = (ATOM_STRUCT *) add_list ( atom_list, (ELEMENT) atm, comp_atom );
      assert ( atm != NULL );
      if ( atm -> signs == NULL ) {
        /* Create new sign list */
        atm -> signs = create_list ();
        assert ( atm -> signs != NULL );
      }
      sgn = (SIGN_PTR *) malloc ( sizeof ( SIGN_PTR ) );
      assert ( sgn != NULL );
      sgn -> sign = curr_sign;
      sgn = (SIGN_PTR *) add_list ( atm -> signs, (ELEMENT) sgn, comp_sptr );
      assert ( sgn != NULL );
    }
  }
  
  fprintf ( counter, "\n" );
}


/****************************************************************
**  calc_results
**
**  Processes atomic concepts. For each concept, builds a list
**  of documents where this concept occurs.
****************************************************************/   

BOOL copy_docs
       ( d )
ELEMENT 
  d;
{
  /* Add document 'd' to temp_list */
  add_doc ( temp_list, ((DOC_STRUCT *) d) -> doc );
  return ( TRUE );
}


BOOL enum_signs
       ( s )
ELEMENT 
  s;
{
  SIGN_STRUCT
    temp,
    *sgn;

  /* Get entry in sign list belonging to this sign */
  temp.sign = ((SIGN_PTR *) s) -> sign;
  sgn = (SIGN_STRUCT *) lookup_list ( sign_list, (ELEMENT) &temp, comp_signs );
  
  /* Add contents of this sign's document list to temp_list */
  if ( sgn != NULL ) { 
    /* some signs don't occur in any document */
    enum_list ( sgn -> docs, copy_docs, ENUM_FORWARD );
  }
  return ( TRUE );
}


BOOL enum_docs
       ( d )
ELEMENT 
  d;
{
  printf ( "\t%d\n", ((DOC_STRUCT *) d) -> doc );
  return ( TRUE );
}


BOOL enum_atoms
       ( a )
ELEMENT 
  a;
{
  /* Create new temporary list */
  temp_list = create_list ();
  
  /* Enumerate list of signs belonging to this concept */
  enum_list ( ((ATOM_STRUCT *) a) -> signs, enum_signs, ENUM_FORWARD );
  
  /* Print contents of temporary list */
  printf ( "%d :\n", ((ATOM_STRUCT *) a) -> atom );
  enum_list ( temp_list, enum_docs, ENUM_FORWARD );
  
  destroy_list ( &temp_list );
  return ( TRUE );
}


void calc_results
       ( )
{
  /* Write out documents for each atom */
  enum_list ( atom_list, enum_atoms, ENUM_FORWARD );
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
  
  /* Open weight file */
  f = open_file ( argv [1] );
  
  /* Create a list of documents per sign */
  fprintf ( stderr, "Reading weight table.\n" );
  read_weights ( f );
  fclose ( f );
  
  /* Open concept space file */
  f = open_file ( argv [2] );
  
  /* Process atomic concepts */
  fprintf ( stderr, "Processing atomic concepts\n" );
  handle_concepts ( f );
  fclose ( f );
  
  /* Print results */
  calc_results ();

  return ( 0 );
}
