/****************************************************************
*
*           S O F T W A R E   S O U R C E   F I L E
*
*****************************************************************
*
*   Name of file   : concepts.c
*   Author         : Guido Hoss
*   Project        : ETH Diploma Thesis (SS 1989)
*   Creation Date  : 30/04/89
*   Type of file   : C Language File
*
*   Description
*   -----------
*   Builds concept spaces based on signs, situations, and
*   abstractions. Writes a list of all atomic concepts which belong
*   to each sign to the standard output.
*
*   Call Format
*   -----------
*	build_concepts <signs> <situations> <abstractions>
*
*   where <signs>, <situations>, and <abstractions> are the names
*   of the corresponding files.
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

#define PROG	"Concept Space Generation (gh, 30/04/89)\n"
#define USAGE	"Usage: build_concepts <signs> <situations> <abstractions> [QUIET]\n"

#include <stdio.h>
#include <malloc.h>
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
    int  sign;     /* sign index */
    int  initatom; /* initial atomic concept */
    int  newinit;  /* fixup variable (used for synonyms) */
    LIST atoms;    /* list of associated atomic concepts */
  } SIGN_STRUCT;

typedef
  struct {
    int  atom;         /* >0 = atomic concept; <0 = reference (-atom) */
    SIGN_STRUCT *ref;  /* reference: pointer to sign entry */
  } ATOM_STRUCT;
  

LIST
  atom_list,     /* Temporary list of atoms for a sign */
  sign_list;     /* List of all signs */
int
  maxatom = 1;   /* Highest atomic concept so far; must be <> 0 */
  
SIGN_STRUCT
  *curr_sign;    /* used by 'enum_signs' */

/* The following stack structure is used to detect cyclic references 
   in 'enum_signs'. */

#define STACK_SIZE	100
SIGN_STRUCT
  *stack [ STACK_SIZE ];
int
  stackmax = 0;

FILE
  *counter;	/* Virtual file used to output running counts */


/****************************************************************
**  Forward declarations (compiler type checking)
****************************************************************/   

#ifndef BSDUNIX
int main ( int, char *[] );
void load_signs ( FILE * );
void handle_situations ( FILE * );
void handle_abstractions ( FILE * );
int comp_sign ( ELEMENT, ELEMENT );
int comp_atom ( ELEMENT, ELEMENT );
BOOL enum_atoms ( ELEMENT );
BOOL enum_signs ( ELEMENT );
BOOL build_atoms ( ELEMENT );
void add_atom ( LIST, int, SIGN_STRUCT * );
SIGN_STRUCT *find_sign ( int );
BOOL check_synonym ( SIGN_STRUCT * );
void pop_call ( void );
#else
int main ();
void load_signs ();
void handle_situations ();
void handle_abstractions ();
int comp_sign ();
int comp_atom ();
BOOL enum_atoms ();
BOOL enum_signs ();
BOOL build_atoms ();
void add_atom ();
SIGN_STRUCT *find_sign ();
BOOL check_synonym ();
void pop_call ();
#endif


/****************************************************************
**  add_atom
**
**  Adds an atomic concept to a specified list.
**
**  IN  : list = Handle to list with elements of type ATOM_STRUCT.
**        s    = Sign value (or undefined if this is a reference).
**        r    = Reference (or NULL if none).
**
**  Negative values: subtract 1 from the negative sign value
**  (because +0 and -0 are identical).
****************************************************************/   

int comp_atom 
      ( s1, s2 )
ELEMENT
  s1;
ELEMENT
  s2;
{
  ATOM_STRUCT
   *a1, *a2;
   
  /* Order of sign structures = numerical order of atomic concepts.
     References are 'smaller' than non-references */
  a1 = (ATOM_STRUCT *) s1;
  a2 = (ATOM_STRUCT *) s2;
  
  if ( ( ( a1 -> ref != NULL ) && ( a2 -> ref != NULL ) ) ||
       ( ( a1 -> ref == NULL ) && ( a2 -> ref == NULL ) ) ) {
    /* Two sign entries or two references */
    return ( ((ATOM_STRUCT *) s1) -> atom - ((ATOM_STRUCT *) s2) -> atom );
  }
  else if ( a1 -> ref == NULL ) {
    return ( -1 );
  }
  else {
    /* a2 -> ref == NULL */
    return  ( 1 );
  }
}


void add_atom 
       ( list, s, r )
LIST
  list;
int
  s;
SIGN_STRUCT
  *r;
{
  ATOM_STRUCT
    *atm;

  atm = (ATOM_STRUCT *) malloc ( sizeof ( ATOM_STRUCT ) );
  assert ( atm != NULL );
  atm -> atom = s;
  atm -> ref = r;
  atm = (ATOM_STRUCT *) add_list ( list, (ELEMENT) atm, comp_atom );
  assert ( atm != NULL );
}


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
  /* Order of sign structures = numerical order of signs */
  return ( ((SIGN_STRUCT *) s1) -> sign - ((SIGN_STRUCT *) s2) -> sign );
}


void load_signs 
       ( f )
FILE
  *f;
{
  int
    initatom,
    res;
  char
    line [ LINE_LENGTH ];
  SIGN_STRUCT
    *elt;

  /* Create sign list */
  sign_list = create_list ();
  assert ( sign_list != NULL );
  
  /* Read sign file one line at a time */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    /* Create new list element for this sign */
    elt = (SIGN_STRUCT *) malloc ( sizeof ( SIGN_STRUCT ) );
    assert ( elt != NULL );

    /* Extract sign index */
    res = sscanf ( line, " %d", &( elt -> sign ) );
    assert ( res == 1 );
    
    /* Create empty list of atomic concepts */
    elt -> atoms = create_list ();
    assert ( elt -> atoms != NULL );
    
    /* Add sign to list */
    elt = (SIGN_STRUCT *) add_list ( sign_list, (ELEMENT) elt, comp_sign );
    assert ( elt != NULL );
    
    /* Associate initial atomic concept with sign */
    initatom = maxatom;
    maxatom ++;
    
    /* If sign is negative (stopword), then initial atom must also be
       negative */
    if ( elt -> sign < 0 ) {
      initatom = -initatom;
    }
    
    /* Add atomic concept (either new or atom of synonym) to list for
       current sign */
    add_atom ( elt -> atoms, initatom, NULL );
    elt -> initatom = elt -> newinit = initatom;

    /* Running count */
    fprintf ( counter, "%d\r", elt -> sign );
  }
  fprintf ( counter, "\n" );
}


/****************************************************************
**  handle_situations
**
**  Reads situations from a file and adds common atomic concepts
**  to the iag and iob of the situations. Also constructs induced
**  abstractions by designating the iag (resp. iob) as a subset of
**  agn (resp. obj).
****************************************************************/   

SIGN_STRUCT *find_sign
               ( s )
int 
  s;
{
  SIGN_STRUCT
    *res, temp;

  temp.sign = s;
  res = (SIGN_STRUCT *) lookup_list ( sign_list, (ELEMENT) &temp, comp_sign );
  
  if ( res == NULL ) {
    /* Maybe 's' is a stopword (negative index) */
    temp.sign = -s;
    res = (SIGN_STRUCT *) lookup_list ( sign_list, (ELEMENT) &temp, comp_sign );
  }
  
  /* The searched sign MUST be in the list in any case */
  assert ( res != NULL );
  return ( res );
}


void handle_situations
       ( f )
FILE
  *f;
{
  char
    line [ LINE_LENGTH ];
  int
    ag, ob, ia, io, res, n;
  SIGN_STRUCT
    *agn, *obj, *iag, *iob;

  /* Read one situation at a time */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    /* Get situation no. and agn,obj,iag,iob values */
    res = sscanf ( line, "%d %d %d %d %d", &n, &ag, &ob, &ia, &io );
    assert ( res == 5 );
    
    /* Find corresponding list entries */
    agn = find_sign ( ag );
    obj = find_sign ( ob );
    iag = find_sign ( ia );
    iob = find_sign ( io );
    
    /* c(iag) is a subset of c(agn); add reference to agn */
    add_atom ( agn -> atoms, ia, iag );
    
    /* c(iob) is a subset of c(obj); add reference to obj */
    add_atom ( obj -> atoms, io, iob );
    
    /* c(iag) and c(iob) have a common atomic concept; this concept
       has a negative index */
    add_atom ( iag -> atoms, -maxatom, NULL );
    add_atom ( iob -> atoms, -maxatom, NULL );
    maxatom ++;
    
    /* running count */
    fprintf ( counter, "%d\r", n );
  }
  fprintf ( counter, "\n" );
}


/****************************************************************
**  handle_abstractions
**
**  Loads abstractions from file into memory.
**
**  IN  : f = handle to open abstraction file.
****************************************************************/ 

void handle_abstractions
       ( f )
FILE
  *f;
{
  char
    line [ LINE_LENGTH ];
  int
    i = 1,
    res,
    specific,
    general;
  SIGN_STRUCT
    *gen,
    *spc;
  
  /* Read one abstraction per line */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    /* Extract specific and general term */
    res = sscanf ( line, " %d %d", &specific, &general );
    assert ( res == 2 );
    
    /* Make a reference from general term to specific term */
    gen = find_sign ( general );
    spc = find_sign ( specific );
    add_atom ( gen -> atoms, specific, spc );
    
    /* running count */
    fprintf ( counter, "%d\r", i );
    i ++;
  }
  fprintf ( counter, "\n" );
}


/****************************************************************
**  enum_atoms, enum_signs, build_atoms
**
**  Print out the atomic concepts belonging to each sign.
****************************************************************/   

/****************************************************************
**  enum_atoms is called for the complete list of atomic concepts
**  for a sign. The list must not contain any references.
****************************************************************/ 
  
BOOL enum_atoms
       ( s )
ELEMENT
  s;
{
  printf ( "\t%d\n", ((ATOM_STRUCT *) s) -> atom );
  return ( TRUE );
}


/****************************************************************
**  check_synonym looks for cyclic references. If it found one,
**  all signs in the cycle are assigned the same initial concept
**  space.
****************************************************************/ 

BOOL check_synonym
       ( s )
SIGN_STRUCT
  *s;
{
  int
    i, j, common;

  /* check all elements in stack to find one that is equal to s */
  for ( i = stackmax - 1; i > 0; i -- ) {
    if ( stack [i] == s ) {
      /* we have a cycle of synonyms; assign a common concept space to all
         elements in cycle */
      common = stack [i] -> newinit;
      for ( j = i + 1; j < stackmax; j ++ ) {
        stack [j] -> newinit = common;
      }
      return ( TRUE );
    }
  }
  
  /* no synonym, so push s on stack */
  assert ( stackmax < STACK_SIZE );
  stack [ stackmax ] = s;
  stackmax ++;
  return ( FALSE );
}


void pop_call
       ( )
{
  /* remove one recursive call from the stack */
  assert ( stackmax > 0 );
  stackmax --;
}


/****************************************************************
**  build_atoms is called while resolving references in an
**  atom list.
****************************************************************/ 
  
BOOL build_atoms
       ( s )
ELEMENT
  s;
{
  SIGN_STRUCT
    *old_sign, *r;
  int
    atom;
    
  r = ((ATOM_STRUCT *) s) -> ref;
  if ( r != NULL ) {
    /* This is a reference to another list; enumerate that list first */
    if ( ! check_synonym ( r ) ) {
      old_sign = curr_sign;
      curr_sign = r;
      enum_list ( r -> atoms, build_atoms, ENUM_FORWARD );
      curr_sign = old_sign;
      pop_call ();
    }
  }
  else {
    /* This is a valid atom; add to atom_list */
    atom = ((ATOM_STRUCT *) s) -> atom;
    if ( atom == curr_sign -> initatom ) {
      /* Handle common concept spaces of synonyms */
      atom = curr_sign -> newinit;
    }
    add_atom ( atom_list, atom, NULL );
  }
  return ( TRUE );
}


BOOL enum_signs
       ( s )
ELEMENT
  s;
{
  /* Print number of sign */
  curr_sign = (SIGN_STRUCT *) s;
  printf ( "%d :\n", curr_sign -> sign );
  
  /* Create list which holds all atoms for this sign */
  atom_list = create_list ();
  assert ( atom_list != NULL );
  
  /* Resolve references */
  assert ( stackmax == 0 );      /* stack must be empty */
  check_synonym ( curr_sign );   /* push initial value on stack */
  enum_list ( curr_sign -> atoms, build_atoms, ENUM_FORWARD );
  pop_call ();
  
  /* Print all atomic concepts which belong to sign */
  enum_list ( atom_list, enum_atoms, ENUM_FORWARD );

  /* Destroy atom list */
  destroy_list ( &atom_list );
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

  /* Check arguments */
  if ( argc < 4 ) {
    fprintf ( stderr, USAGE );
    return ( 1 );
  }
  
  /* Open sign file */
  f = open_file ( argv [1] );
  
  /* Load signs into memory */
  fprintf ( stderr, "Loading signs.\n" );
  load_signs ( f );
  fclose ( f );
  
  /* Open situation file */
  f = open_file ( argv [2] );
  
  /* Process situations */
  fprintf ( stderr, "Processing situations.\n" );
  handle_situations ( f );
  fclose ( f );
  
  /* Open abstraction file */
  f = open_file ( argv [3] );
  
  /* Process explicit abstractions */
  fprintf ( stderr, "Processing abstractions.\n" );
  handle_abstractions ( f );
  fclose ( f );
  
  /* Print results */
  enum_list ( sign_list, enum_signs, ENUM_FORWARD );
  
  return ( 0 );
}
