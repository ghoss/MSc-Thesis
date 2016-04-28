/****************************************************************
*
*           S O F T W A R E   S O U R C E   F I L E
*
*****************************************************************
*
*   Name of file   : calc_rsv.c
*   Author         : Guido Hoss
*   Creation Date  : 26/04/89
*   Type of file   : C Language File
*
*   Description
*   -----------
*   Calculation of RSV values for documents and queries.
*
*   Call Format
*   -----------
*	calc_rsv <doc-descr> <concepts> <atom-wgts>
*
*   where <doc-descr> is the file containing the weights of each
*   sign in each document, <concepts> is the list of the atomic
*   concepts and their associations with signs, and <atom-wgts>
*   contains the weight of each atomic concept. The latter file
*   is initialized with the IDF of each atom before the first
*   optimization step.
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

#define PROG	"RSV Calculation (gh, 05/05/89)\n"
#define USAGE	"calc_rsv <doc-descr> <concepts> <atom-wgts> [QUIET]\n"

#ifdef MSDOS
#include <process.h>
#endif
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>

#include "boolean.h"
#include "list.h"
#include "util.h"

#define LINE_LENGTH	100


typedef
  struct {
    int  sign;   /* number of this sign */
    LIST atoms;  /* atomic concepts which belong to this sign */
  } SIGN_STRUCT;
  
typedef
  struct {
    int  index;       /* number of this document */
    LIST docatoms;    /* list of atoms occuring in the document */
  } DOC_STRUCT;
  
typedef
  struct {
    int    atom;        /* number of an atomic concept */
    double weight;	/* atomic concept's weight */
  } ATOM_STRUCT;
  

LIST
  glob_list,   /* global list used by 'calc_rsv' */
  doc_list,    /* all documents which occur in the EVAL_PREF file */
  atom_list,   /* all atomic concepts and their matrix indices */
  sign_list;   /* signs used in documents */

/* Global variables used for RSV calculation */
double
  glob_wgt,
  glob_rsv;

DOC_STRUCT
  *glob_query;
FILE
  *counter;	/* Virtual file used to output running counts */


/****************************************************************
**  Forward declarations (compiler type checking)
****************************************************************/ 

#ifndef BSDUNIX
int main ( int, char * [] );  
double calc_rsv ( DOC_STRUCT *, DOC_STRUCT * );
int comp_doc ( ELEMENT, ELEMENT );
int comp_sign ( ELEMENT, ELEMENT );
int comp_atom ( ELEMENT, ELEMENT );
void process_documents ( FILE * );
void process_concepts ( FILE * );
void load_weights ( FILE * );
BOOL make_docatoms ( ELEMENT );
BOOL enum_query ( ELEMENT );
BOOL enum_doc ( ELEMENT );
BOOL union_proc ( ELEMENT, ELEMENT );
#else
int main ();  
double calc_rsv ();
int comp_doc ();
int comp_sign ();
int comp_atom ();
void process_documents ();
void process_concepts ();
void load_weights ();
BOOL make_docatoms ();
BOOL enum_query ();
BOOL enum_doc ();
BOOL union_proc ();
#endif


/****************************************************************
**  process_concepts
**
**  Looks at each sign in the CONCEPTS file; if the sign is in
**  sign_list (the list of signs which occur in some evaluated
**  document)
**  if the document occurs in a preference relation, then the
**  sign weights for this document are read. Otherwise, the
**  document and its signs are ignored.
**
**  IN  : f = handle to the open DOCU_DESC file.
****************************************************************/ 

int comp_sign
      ( s1, s2 )
ELEMENT
  s1;
ELEMENT
  s2;
{
  return ( ((SIGN_STRUCT *) s1) -> sign - ((SIGN_STRUCT *) s2) -> sign );
}


int comp_atom
      ( s1, s2 )
ELEMENT
  s1;
ELEMENT
  s2;
{
  return ( ((ATOM_STRUCT *) s1) -> atom - ((ATOM_STRUCT *) s2) -> atom );
}


void process_concepts
       ( f )
FILE 
  *f;
{
  char
    line [ LINE_LENGTH ];
  SIGN_STRUCT
    *sgn;
  LIST
    list;
  int
    d, n;
  ATOM_STRUCT
    *atm, *atm2;

  /* Create list of ALL atomic concepts */
  atom_list = create_list ();
  sign_list = create_list ();
  
  assert ( atom_list != NULL );
  assert ( sign_list != NULL );

  /* Read file line by line */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    /* Read a number */
    n = sscanf ( line, " %d", &d );
    assert ( n == 1 );
    
    /* If number followed by a colon, then this is a sign index */
    if ( strchr ( line, ':' ) != NULL ) {
      /* Add sign entry to sign_list */
      sgn = (SIGN_STRUCT *) malloc ( sizeof ( SIGN_STRUCT ) );
      assert ( sgn != NULL );
      sgn -> sign = d;
      sgn -> atoms = NULL;
      sgn = (SIGN_STRUCT *) add_list ( sign_list, (ELEMENT) sgn, comp_sign );
      assert ( sgn != NULL );
      if ( sgn -> atoms == NULL ) {
        sgn -> atoms = create_list ();
        assert ( sgn -> atoms != NULL );
      }
      list = sgn -> atoms;
      
      /* running count */
      fprintf ( counter, "%d\r", d );
    }
    
    else {
      /* no colon -> this is an atomic concept; add atom to atom_list,
         which contains ALL atomic concepts */
      atm = (ATOM_STRUCT *) malloc ( sizeof ( ATOM_STRUCT ) );
      assert ( atm != NULL );
      atm -> atom = d;
      atm -> weight = -999.9;  /* error value */
      atm = (ATOM_STRUCT *) add_list ( atom_list, (ELEMENT) atm, comp_atom );
      assert ( atm != NULL );

      /* Add atomic concept to concept list of current sign */
      atm2 = (ATOM_STRUCT *) malloc ( sizeof ( ATOM_STRUCT ) );
      assert ( atm2 != NULL );
      atm2 -> atom = d;
      atm2 = (ATOM_STRUCT *) add_list ( list, (ELEMENT) atm2, comp_atom );
    }
  }
  
  fprintf ( counter, "\n" );
}


/****************************************************************
**  process_documents
**
**  Looks at each document description in the DOCU-DESC file;
**  if the document occurs in a preference relation, then the
**  sign weights for this document are read. Otherwise, the
**  document and its signs are ignored.
**
**  IN  : f = handle to the open DOCU_DESC file.
****************************************************************/ 

BOOL make_docatoms
       ( e )
ELEMENT 
  e;
{
  ATOM_STRUCT
    *a;

  /* duplicate atom and add to 'glob_list' */
  a = (ATOM_STRUCT *) malloc ( sizeof ( ATOM_STRUCT ) );
  assert ( a != NULL );
  a -> atom = ((ATOM_STRUCT *) e) -> atom;
  a -> weight = 0.0;
  a = (ATOM_STRUCT *) add_list ( glob_list, (ELEMENT) a, comp_atom );
  assert ( a != NULL );
  
  /* atom might already have been entered previously, so ADD weight */
  a -> weight += glob_wgt;
  return ( TRUE );
}


void process_documents
       ( f )
FILE 
  *f;
{
  char
    line [ LINE_LENGTH ];
  SIGN_STRUCT
    t, *sgn;
  DOC_STRUCT
    *doc;
  double
    w;
  float 
    w1;
  int
    d, n;

  /* Create document list */
  doc_list = create_list ();
  assert ( doc_list != NULL );

  /* Read file line by line */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    /* Interpret as much fields as possible */
    n = sscanf ( line, " %d %f", &d, &w1 );
    w = (double) w1;
    switch ( n ) {
      
      case 1 :
        /* One field -> new document number; update running count */
        fprintf ( counter, "%d\r", d );
        
        doc = (DOC_STRUCT *) malloc ( sizeof ( DOC_STRUCT ) );
        assert ( doc != NULL );
        doc -> index = d;
        doc -> docatoms = NULL;
        doc = (DOC_STRUCT *) add_list ( doc_list, (ELEMENT) doc, comp_doc );
        assert ( doc != NULL );
        assert ( doc -> docatoms == NULL );
        doc -> docatoms = create_list ();
        glob_list = doc -> docatoms;
        assert ( glob_list != NULL );
        break;
        
      case 2 :
        /* Two fields -> sign index and weight; add all atoms of sign 
           to list of doc. */
           
        /* Find sign in sign_list */
        t.sign = d;
        sgn = (SIGN_STRUCT *) lookup_list ( sign_list, (ELEMENT) &t, comp_sign );
        assert ( sgn != NULL );
        
        /* Add atoms of sign to 'glob_list' */
        glob_wgt = w;
        enum_list ( sgn -> atoms, make_docatoms, ENUM_FORWARD );
        break;
    }
  }
  
  fprintf ( counter, "\n" );
}


int comp_doc
      ( d1, d2 )
ELEMENT 
  d1;
ELEMENT
  d2;
{
  return ( ((DOC_STRUCT *) d1) -> index - ((DOC_STRUCT *) d2) -> index );
}


/****************************************************************
**  calc_rsv
**
**  IN  : 
**
**  OUT : The vector v is filled with the appropriate values.
****************************************************************/

BOOL union_proc
       ( a1, a2 )
ELEMENT
  a1;
ELEMENT
  a2;
{
  ATOM_STRUCT
    *atm, *at1, *at2;
  
  at1 = (ATOM_STRUCT *) a1;
  at2 = (ATOM_STRUCT *) a2;
  assert ( at1 -> atom == at2 -> atom );
  
  /* For each atom which occurs in both query and document: find weight */
  atm = (ATOM_STRUCT *) lookup_list ( atom_list, a1, comp_atom );
  assert ( atm != NULL );
  
  /* Multiply weights with weight of atomic concept */
  glob_rsv += ( at1 -> weight * at2 -> weight * atm -> weight );
  return ( TRUE );
}


double calc_rsv
       ( q, d )
DOC_STRUCT
  *q, *d;
{
  glob_rsv = 0.0;
  find_union ( q -> docatoms, d -> docatoms, comp_atom, union_proc );
  return ( glob_rsv );
}


/****************************************************************
**  calc_equations
**
**  Calculates the constraint equations which must be satisfied
**  by the optimized weights, as well as the cost function.
**
**  IN  : x = vector containing the inverse document frequency
**            of all atomic concepts. This vector is used to
**            generate equations (4.2, 4.3) on p.78 of the thesis.
**
**  OUT : The global vectors min_weights, max_weights, rsv_eq,
**        and cost are initialized with the appropriate values.
**        These vectors constitute the optimization matrix.
****************************************************************/

BOOL enum_doc
       ( e )
ELEMENT
  e;
{
  double
    rsv;
  DOC_STRUCT
    *d;

  d = (DOC_STRUCT *) e;
  if ( d -> index >= 0 ) {
    rsv = calc_rsv ( glob_query, d );
    if ( rsv > 0.0 ) {
      /* don't print zero rsv values */
      printf ( "%d\t%d\t%f\n", glob_query -> index, d -> index, rsv );
    }

    /* Running count */
    fprintf ( counter, "%d %d\r", glob_query -> index, d -> index );
  }
  return ( TRUE );
}


BOOL enum_query
       ( e )
ELEMENT
  e;
{
  DOC_STRUCT
    *q;

  q = (DOC_STRUCT *) e;

  if ( q -> index < 0 ) {
    /* This is a query, calculate RSV with all documents */
    glob_query = q;
    enum_list ( doc_list, enum_doc, ENUM_FORWARD );
  }
  return ( TRUE );
}


/****************************************************************
**  load_weights
**
**  Loads the weight of each atomic concept into memory.
**
**  IN  : f = handle to open ATOM_WGTS file.
****************************************************************/

void load_weights
       ( f )
FILE
  *f;
{
  char
    line [ LINE_LENGTH ];
  int
    d, res;
  double
    wgt;
  float
    w;
  ATOM_STRUCT
    t, *atm;

  while ( fgets ( line, LINE_LENGTH, f ) ) {
     
    /* Get number of atomic concept and weight */
    res = sscanf ( line, " %d %f", &d, &w );
    assert ( res == 2 );
    wgt = (double) w;   /* Argument to sscanf MUST be float */

    /* Search atomic concept in list */
    t.atom = d;
    atm = (ATOM_STRUCT *) lookup_list ( atom_list, (ELEMENT) &t,
                                        comp_atom );
    assert ( atm != NULL );
    atm -> weight = wgt;

    /* running count */
    fprintf ( counter, "%d\r", d );
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
  if ( ( argc == 5 ) && ( *argv [4] == 'Q' ) ) {
    /* in case of quiet mode: redirect running counts to /dev/null */
    counter = fopen ( "/dev/null", "r" );
    assert ( counter != NULL );
  }
  else {
    /* verbose mode: redirect running counts to stderr */
    counter = stderr;
  }
  
  /* Check parameters */
  if ( argc < 4 ) {
    fprintf ( stderr, USAGE );
    return ( 1 );
  }
  
  /* Read atomic concepts of each sign */
  fprintf ( stderr, "Reading atomic concepts.\n" );
  f = open_file ( argv [2] );
  process_concepts ( f );
  fclose ( f );
  
  /* Load weights of atomic concepts */
  fprintf ( stderr, "Reading weights.\n" );
  f = open_file ( argv [3] );
  load_weights ( f );
  fclose ( f );
  
  /* Read document descriptions */
  fprintf ( stderr, "Reading document descriptions.\n" );
  f = open_file ( argv [1] );
  process_documents ( f );
  fclose ( f );
  
  /* Calculate RSV values */
  fprintf ( stderr, "Calculating RSV values.\n" );
  enum_list ( doc_list, enum_query, ENUM_FORWARD );

  return ( 0 );
}
