/****************************************************************
*
*           S O F T W A R E   S O U R C E   F I L E
*
*****************************************************************
*
*   Name of file   : select.c
*   Author         : Guido Hoss
*   Project        : ETH Diploma Thesis (SS 1989)
*   Creation Date  : 15/06/89
*   Type of file   : C Language File
*
*   Description
*   -----------
*   Selects suitable preferences for optimization. A preference
*   is called suitable if the query and both documents contain at
*   least one common atomic concept.
*
*   Call Format
*   -----------
*	select <atom-docs> <eval-prefs> [QUIET]
*
*   Suitable preferences are written to the standard output.
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

#define PROG	"Preference Selection (gh, 15/06/89)\n"
#define USAGE	"Usage: select <atom-docs> [QUIET]\n"

#include <stdio.h>
#ifdef MSDOS
#include <process.h>
#endif
#include <assert.h>
#include <malloc.h>
#include <string.h>

#include "boolean.h"
#include "list.h"
#include "util.h"

#define LINE_LENGTH	100

typedef 
  struct {
    int		doc;		/* Document or query number */
    LIST	concepts;	/* List of contained atomic concepts */
  } DOC_STRUCT;

typedef
  struct {
    int		atom;		/* Atom number */
  } ATOM_STRUCT;


LIST
  glob_temp,
  query_list,	/* Queries and their concepts */
  doc_list;	/* Normal documents and their concepts */

FILE
  *counter;	/* Virtual file used for display of running counts */
  

/****************************************************************
**  Procedure declarations
****************************************************************/   

#ifndef BSDUNIX
int main ( int, char * [] );
void read_concepts ( FILE * );
void read_prefs ( FILE * );
BOOL enum_primary ( ELEMENT, ELEMENT );
void add_atom ( LIST, int );
int comp_doc ( ELEMENT, ELEMENT );
int comp_atom ( ELEMENT, ELEMENT );
#else
int main ();
void read_concepts ();
void read_prefs ();
BOOL enum_primary ();
void add_atom ();
int comp_doc ();
int comp_atom ();
#endif


/****************************************************************
**  add_atom
**
**  Adds an atomic concept to a specified list.
**
**  IN  : list = Handle to created list.
**        atom = Number of atom to add to list.
****************************************************************/   

int comp_atom
      ( a1, a2 )
ELEMENT
  a1;
ELEMENT
  a2;
{
  return ( ((ATOM_STRUCT *) a1) -> atom - ((ATOM_STRUCT *) a2) -> atom );
}


void add_atom
       ( list, atom )
LIST
  list;
int
  atom;
{
  ATOM_STRUCT
    *atm;
  
  atm = (ATOM_STRUCT *) malloc ( sizeof ( ATOM_STRUCT ) );
  assert ( atm != NULL );
  
  atm -> atom = atom;
  atm = (ATOM_STRUCT *) add_list ( list, (ELEMENT) atm, comp_atom );
  assert ( atm != NULL );
}


/****************************************************************
**  read_concepts
**
**  Reads ATOM_DOCS file. Builds inverted list "doc -> concept"
**  for documents and queries.
**
**  IN  : f = handle to open ATOM_DOCS file.
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


void read_concepts
       ( f )
FILE
  *f;
{
  char
    line [ LINE_LENGTH ];
  DOC_STRUCT
    *curr_doc;
  int
    curr_atom,
    d, n;

  /* Create empty lists */
  doc_list = create_list ();
  assert ( doc_list != NULL );
  query_list = create_list ();
  assert ( query_list != NULL );
  
  /* Ignore first line of ATOM_DOCS */
  fgets ( line, LINE_LENGTH, f );

  /* Read ATOM_DOCS file, line by line */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    n = sscanf ( line, " %d", &d );
    assert ( n == 1 );
    
    if ( strchr ( line, ':' ) == NULL ) {
      /* Number of new document */
      curr_doc = (DOC_STRUCT *) malloc ( sizeof ( DOC_STRUCT ) );
      assert ( curr_doc != NULL );
      curr_doc -> doc = d;
      curr_doc -> concepts = NULL;
      
      /* Add document entry to either doc_list or query_list */
      if ( d >= 0 ) {
        /* Document */
        curr_doc = (DOC_STRUCT *) add_list ( doc_list, (ELEMENT) curr_doc, 
					     comp_doc );
      }
      else {
        /* Query */
        curr_doc = (DOC_STRUCT *) add_list ( query_list, (ELEMENT) curr_doc,
					     comp_doc );
      }
      assert ( curr_doc != NULL );
      if ( curr_doc -> concepts == NULL ) {
        curr_doc -> concepts = create_list ();
        assert ( curr_doc -> concepts != NULL );
      }

      add_atom ( curr_doc -> concepts, curr_atom );
    }
    else {
      /* Number of concept */
      curr_atom = d;
      
      /* Running count */
      fprintf ( counter, "%d\r", d );
    }
  }
  
  fprintf ( counter, "\n" );
}


/****************************************************************
**  read_prefs
**
**  Reads preferences and checks each one for common concepts.
**
**  IN  : f = handle to open EVAL_PREF file.
****************************************************************/   

BOOL enum_primary
       ( a1, a2 )
ELEMENT
  a1;
ELEMENT
  a2;
{
  int
    atom;
  
  atom = ((ATOM_STRUCT *) a1) -> atom;
  
  /* Add each common atomic concept to glob_temp if
     the atom is negative (negative atoms will be optimized) */
  if ( atom < 0 ) {
    add_atom ( glob_temp, atom );
  }
  
  return ( TRUE );
}


void read_prefs
       ( f )
FILE
  *f;
{
  char
    type,
    line [ LINE_LENGTH ];
  int
    q, d1, d2, n, i;
  DOC_STRUCT
    t, *query, 
    *doc1, *doc2;

  i = 0;

  /* Read EVAL_PREF file, line by line */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    n = sscanf ( line, " %c %d %d %d", &type, &q, &d1, &d2 );
    assert ( n == 4 );
    
    if ( type == 'C' ) {
      /* Ignore preferences which are just necessary for cost function */
      printf ( "%c\t%d\t%d\t%d\n", type, q, d1, d2 );
      continue;
    }
    
    /* Get concept lists of query and both docs */
    t.doc = q;
    query = (DOC_STRUCT *) lookup_list ( query_list, (ELEMENT) &t, comp_doc );
    t.doc = d1;
    doc1 = (DOC_STRUCT *) lookup_list ( doc_list, (ELEMENT) &t, comp_doc );
    t.doc = d2;
    doc2 = (DOC_STRUCT *) lookup_list ( doc_list, (ELEMENT) &t, comp_doc );

    assert ( query != NULL );
    assert ( doc1 != NULL );
    assert ( doc2 != NULL );
    
    glob_temp = create_list ();
    assert ( glob_temp != NULL );

    /* Find union of negative concepts in query and doc1 */
    find_union ( query -> concepts, doc1 -> concepts, comp_atom, enum_primary );
    
    /* Find union of concepts in query and doc2 */
    find_union ( query -> concepts, doc2 -> concepts, comp_atom, enum_primary );
    
    if ( count_list ( glob_temp ) > 0 ) {
      /* Print preference */
      printf ( "%c\t%d\t%d\t%d\n", type, q, d1, d2 );
    }
    
    destroy_list ( &glob_temp );

    /* Running count */
    i ++;
    fprintf ( counter, "%d\r", i );
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
  if ( ( argc == 3 ) && ( *argv [2] == 'Q' ) ) {
    counter = fopen ( "/dev/null", "r" );
    assert ( counter != NULL );
  }
  else {
    counter = stderr;
  }
  
  /* Parameter count */
  if ( argc < 2 ) {
    fprintf ( stderr, USAGE );
    return ( 1 );
  }
  
  /* Read list of atomic concepts */
  fprintf ( stderr, "Reading atomic concepts.\n" );
  f = open_file ( argv [1] );
  read_concepts ( f );
  fclose ( f );
  
  /* Read preferences */
  fprintf ( stderr, "Reading preferences.\n" );
  read_prefs ( stdin );
  fclose ( f );
  
  return ( 0 );
}
