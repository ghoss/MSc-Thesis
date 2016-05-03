/****************************************************************
*
*           S O F T W A R E   S O U R C E   F I L E
*
*****************************************************************
*
*   Name of file   : simplex.c
*   Author         : Guido Hoss
*   Creation Date  : 26/04/89
*   Type of file   : C Language File
*
*   Description
*   -----------
*   Simplex optimization algorithm. The implementation follows 
*   the program example in Schwarz: "Numerische Mathematik", Teubner
*   1986, p.67. Refer to this book for details.
*
*   The algorithm is used here to optimize the weights of a
*   domain algebra.
*
*   Call Format
*   -----------
*	optimize <eval-pref> <doc-descr> <concepts> <atom-docs>
*
*   where <eval-pref> is the file containing the satisfied and
*   unsatisfied preferences, <doc-descr> contains the weights of
*   each sign in each document, <concepts> is the list of atomic
*   concepts associated with each sign, and <atom-docs> is the
*   list of atomic concepts associated with each document.
*
*   The optimized weights are written to the standard output.
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

#define PROG	"Atomic Concept Weight Optimization (gh, 04/05/89)\n"
#define USAGE	"optimize <eval-pref> <doc-descr> <concepts> <atom-wgts> [QUIET]\n"

#ifdef MSDOS
#include <process.h>
#endif
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <malloc.h>
#include <string.h>

#include "boolean.h"
#include "list.h"
#include "util.h"

#define LINE_LENGTH	100


/* Low and high bounds of atomic concept weights */
#include "limits.h"

/* Precision of calculation */
#define EPSILON 0.00001

/* Natural logarithm of 2.0 */
#define log_2	log ( 2.0 )


typedef
  struct {
    int		sign;   /* number of this sign */
    LIST	atoms;  /* atomic concepts which belong to this sign */
  } SIGN_STRUCT;
  
typedef
  struct {
    int		index;		/* number of this document */
    LIST	docatoms;	/* weights of the atoms occuring in the document */
  } DOC_STRUCT;
  
typedef
  struct {
    int		atom;		/* number of an atomic concept */
    int		mat_index;	/* col index of the concept's weight in the matrix */
  } ATOM_STRUCT;
  
typedef
  struct {
    int		d_atom;		/* number of atom */
    float	weight;		/* weight of atom in document to which list belongs */
  } WGT_STRUCT;
  
typedef
  struct {
    int		u_atom;		/* Number of unused atom */
    float	idf;		/* idf of unused atom */
  } IDF_STRUCT;

int
    alloc_count = 0;	/* Used for memory overflow message */

LIST
  unused_wgts, /* list of unused weights */
  glob_list,   /* global list used by 'calc_rsv' */
  doc_list,    /* all documents which occur in the EVAL_PREF file */
  atom_list,   /* all atomic concepts and their matrix indices */
  sign_list;   /* signs used in documents in the EVAL_PREF file */

int
  serial,	/* used by 'make_docatoms' */
  num_prefs,	/* number of preferences in EVAL_PREF file */
  num_weights;	/* number of weights to optimize */

/* Optimization matrix */
float
  **translation,	/* ptr to array of translation equations */
  **min_weights,	/* ptr to array of lower bound equation vectors */
  **max_weights,	/* ptr to array of upper bound equation vectors */
  **rsv_eq;	/* ptr to array of RSV equation vectors */

float
  *cost;	/* pointer to vector of cost function */

/* Global variables used for RSV calculation */
float
  glob_const,	/* used by calc_rsv */
  glob_wgt,
  *glob_v;

BOOL
  glob_bool;	/* Used by 'calc_rsv' */
  
FILE
  *counter;	/* Virtual file used to output running counts */


/****************************************************************
**  Forward declarations (compiler type checking)
****************************************************************/ 

#ifndef BSDUNIX
int main ( int, char * [] );  
BOOL calc_rsv ( int, int, float [], float * );
BOOL simplex ( int, int, float [] );
float *matrix_ptr ( int, int );
void process_pref ( FILE * );
int comp_doc ( ELEMENT, ELEMENT );
int comp_sign ( ELEMENT, ELEMENT );
int comp_atom ( ELEMENT, ELEMENT );
int comp_wgt ( ELEMENT, ELEMENT );
void add_doc ( int );
void process_documents ( FILE * );
void process_concepts ( FILE * );
int matrix_index ( int );
void init_weights ( FILE *, float [] );
void calc_equations ( FILE *, float [] );
float *alloc_vector ( void );
int enum_pref ( FILE *, BOOL );
BOOL enum_results ( ELEMENT );
BOOL make_docatoms ( ELEMENT );
void print_results ( float [] );
void destroy_signlist ( void );
BOOL destroy_atoms ( ELEMENT );
BOOL union_proc ( ELEMENT, ELEMENT );
int comp_unused ( ELEMENT, ELEMENT );
void add_unused ( int, float );
void eliminate ( int, int, int *, int *, BOOL * );
void serialize_atoms ();
#else
int main ();  
BOOL calc_rsv ();
BOOL simplex ();
float *matrix_ptr ();
void process_pref ();
int comp_doc ();
int comp_sign ();
int comp_atom ();
int comp_wgt ();
void add_doc ();
void process_documents ();
void process_concepts ();
int matrix_index ();
void init_weights ();
void calc_equations ();
float *alloc_vector ();
int enum_pref ();
BOOL enum_results ();
BOOL make_docatoms ();
void print_results ();
void destroy_signlist ();
BOOL destroy_atoms ();
BOOL union_proc ();
int comp_unused ();
void add_unused ();
void eliminate ();
void serialize_atoms ();
#endif


/****************************************************************
**  matrix_index
**
**  Returns the column number of an atomic concept in the 
**  optimization matrix.
**
**  IN  : a = atomic concept no.
**
**  OUT : The column number of concept 'a'.
**        If the concept does not occur in the optimization
**        problem, the function returns (-1).
****************************************************************/ 

int matrix_index
      ( a )
int
  a;
{
  ATOM_STRUCT
    *atm, temp;

  /* Search 'a' in the atomic concept list */
  temp.atom = a;
  atm = (ATOM_STRUCT *) lookup_list ( atom_list, (ELEMENT) &temp, comp_atom );
  
  if ( atm != NULL ) {
    return ( atm -> mat_index );
  }
  else {
    return ( -1 );
  }
}


/****************************************************************
**  process_concepts
**
**  Builds a list of signs and atomic concepts of each sign.
**
**  IN  : f = handle to the open CONCEPTS file.
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
    *atm;

  /* Create list of ALL atomic concepts */
  sign_list = create_list ();
  assert ( sign_list != NULL );

  /* Read file line by line */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    /* Read a number */
    n = sscanf ( line, " %d", &d );
    assert ( n == 1 );
    
    /* If number followed by a colon, then this is a sign index */
    if ( strchr ( line, ':' ) != NULL ) {
      /* Set sign entry in sign_list */
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
      /* Add atomic concept to concept list of current sign */
      atm = (ATOM_STRUCT *) malloc ( sizeof ( ATOM_STRUCT ) );
      assert ( atm != NULL );
      atm -> atom = d;
      atm = (ATOM_STRUCT *) add_list ( list, (ELEMENT) atm, comp_atom );
      assert ( atm != NULL );
    }
  }
  
  fprintf ( counter, "\n" );
}


/****************************************************************
**  process_documents
**
**  Looks at each document description in the DOC_DESCR file;
**  if the document occurs in a preference relation, then the
**  sign weights for this document are read. Otherwise, the
**  document and its signs are ignored.
**
**  IN  : f = handle to the open DOC_DESCR file.
****************************************************************/ 

int comp_wgt
      ( s1, s2 )
ELEMENT
  s1;
ELEMENT
  s2;
{
  return ( ((WGT_STRUCT *) s1) -> d_atom - ((WGT_STRUCT *) s2) -> d_atom );
}


BOOL make_docatoms
       ( e )
ELEMENT 
  e;
{
  WGT_STRUCT
    *a;

  /* duplicate atom and add to 'glob_list' */
  a = (WGT_STRUCT *) malloc ( sizeof ( WGT_STRUCT ) );
  assert ( a != NULL );
  a -> d_atom = ((ATOM_STRUCT *) e) -> atom;
  a -> weight = 0.0;
  a = (WGT_STRUCT *) add_list ( glob_list, (ELEMENT) a, comp_wgt );
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
  BOOL
    active = FALSE;
  SIGN_STRUCT
    t, *sgn;
  DOC_STRUCT
    t1, *doc;
  float
    w;
  int
    d, n;

  /* Read file line by line */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    /* Interpret as much fields as possible */
    n = sscanf ( line, " %d %f", &d, &w );
    switch ( n ) {
      
      case 1 :
        /* One field -> new document number; update running count */
        fprintf ( counter, "%d\r", d );
        
        /* Is document in doc_list? */
        t1.index = d;
        doc = (DOC_STRUCT *) lookup_list ( doc_list, (ELEMENT) &t1, comp_doc );
        if ( doc != NULL ) {
          active = TRUE;
          glob_list = doc -> docatoms;
        }
        else {
          active = FALSE;
        }
        break;
        
      case 2 :
        /* Two fields -> sign index and weight */
        if ( active ) {
          /* Find sign in sign_list */
          t.sign = d;
          sgn = (SIGN_STRUCT *) lookup_list ( sign_list, (ELEMENT) &t, comp_sign );
          assert ( sgn != NULL );
        
          /* Add atoms of sign to 'glob_list' */
          glob_wgt = w;
          enum_list ( sgn -> atoms, make_docatoms, ENUM_FORWARD );
        }
        break;
    }
  }
}


/****************************************************************
**  init_weights
**
**  Reads the file ATOM_DOCS and calculates the inverse document
**  frequency for each atomic concept. Stores this frequency
**  at the correct place in the solution vector.
**
**  IN  : f = handle to opened ATOM_DOCS file.
**        x = pointer to previously allocated array with as much
**            elements as there are atomic concepts.
**
**  OUT : x is filled with initial weights equal to the 
**        IDF of each atomic concept.
****************************************************************/ 

int comp_unused
      ( e1, e2 )
ELEMENT
  e1;
ELEMENT
  e2;
{
  /* Sort unused weights by atom index */
  return ( ((IDF_STRUCT *) e1) -> u_atom - ((IDF_STRUCT *) e2) -> u_atom );
}


void add_unused
       ( atom, idf )
int
  atom;
float 
  idf;
{
  IDF_STRUCT
    *unused;

  /* Add unused atom to internal list for rsv calculation */
  unused = (IDF_STRUCT *) malloc ( sizeof ( IDF_STRUCT ) );
  assert ( unused != NULL );
  unused -> u_atom = atom;
  unused -> idf = idf;
  unused = (IDF_STRUCT *) add_list ( unused_wgts, (ELEMENT) 
              unused, comp_unused );
  assert ( unused != NULL );
}


void init_weights
       ( f, x )
FILE
  *f;
float
  x [];
{
  char
    line [ LINE_LENGTH ];
  float
    *arr,
    idf;
  int
    matidx,
    curratom, n;

  /* Create list for unused weights which are needed in rsv calculation */
  unused_wgts = create_list ();
  assert ( unused_wgts != NULL );

  /* Create equation vector for translation equations */
  translation = (float **) calloc ( num_weights, sizeof ( float * ) );
  assert ( translation != NULL );
  
  /* Read file line by line */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    /* Read a number */
    n = sscanf ( line, " %d %f", &curratom, &idf );
    assert ( n == 2 );
    
    /* Check if we need this weight */
    matidx = matrix_index ( curratom );
    if ( matidx != -1 ) {
      /* This concept is used in one or more documents */
      x [ matidx ] = idf;

      /* Create translation equation */
      arr = alloc_vector ();
      arr [ matidx ] = 1.0;
      arr [ num_weights ] = idf;
      translation [ matidx ] = arr;
    }
    else {
      /* Unused concept, so write value directly to output */
      printf ( "%d\t%f\n", curratom, idf );
      add_unused ( curratom, idf );
    }	  
    fprintf ( counter, "%d\r", curratom );
  }
  
  fprintf ( counter, "\n" );
}


/****************************************************************
**  add_doc
**
**  Adds a document index to the global list 'doc_list'.
**
**  IN  : d = index of document to be added.
****************************************************************/ 

int comp_doc
      ( d1, d2 )
ELEMENT 
  d1;
ELEMENT
  d2;
{
  return ( ((DOC_STRUCT *) d1) -> index - ((DOC_STRUCT *) d2) -> index );
}


void add_doc 
       ( d )
int
  d;
{
  DOC_STRUCT
    *doc;
  
  doc = (DOC_STRUCT *) malloc ( sizeof ( DOC_STRUCT ) );
  assert ( doc != NULL );
  doc -> index = d;
  doc -> docatoms = NULL;
  
  doc = (DOC_STRUCT *) add_list ( doc_list, (ELEMENT) doc, comp_doc );
  assert ( doc != NULL );
  if ( doc -> docatoms == NULL ) {
    doc -> docatoms = create_list ();
    assert ( doc -> docatoms != NULL );
  }
}


/****************************************************************
**  process_pref
**
**  Processes evaluated preferences. Simply reads the document
**  numbers which occur in the preference file.
**
**  IN  : f = handle to open 'EVAL_PREF' file.
****************************************************************/ 

void process_pref
       ( f )
FILE
  *f;
{
  int
    d, d1, d2,
    res;
  char
    type,
    line [ LINE_LENGTH ];
    
  doc_list = create_list ();
  assert ( doc_list != NULL );
  
  /* Read one line at a time */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    /* Parse preference type and 3 document numbers */
    res = sscanf ( line, "%c %d %d %d", &type, &d, &d1, &d2 );
    assert ( res == 4 );
    
    /* Insert documents d, d1, d2 into document list */
    add_doc ( d );
    add_doc ( d1 );
    add_doc ( d2 );

    /* running count */
    num_prefs ++;
    fprintf ( counter, "%d\r", num_prefs );
  }
  fprintf ( counter, "\n" );
}


/****************************************************************
**  serialize_atoms
**
**  For each atom in atom_list: Associate a serial matrix index 
**  (beginning at 0) with each atom and replace atom_list in the end.
****************************************************************/ 

void add_atom
       ( atom )
int
  atom;
{
  ATOM_STRUCT
    *atm;
  
  atm = (ATOM_STRUCT *) malloc ( sizeof ( ATOM_STRUCT ) );
  assert ( atm != NULL );
  
  /* Add atom to 'atom_list' */
  atm -> atom = atom;
  atm -> mat_index = -1;
  atm = (ATOM_STRUCT *) add_list ( atom_list, (ELEMENT) atm, comp_atom );
  assert ( atm != NULL );
  
  if ( atm -> mat_index == -1 ) {
    atm -> mat_index = serial;
    serial ++;
  }
}



BOOL enum_primary
       ( a1, a2 )
ELEMENT
  a1;
ELEMENT
  a2;
{
  int
    atom;
    
  atom = ((WGT_STRUCT *) a1) -> d_atom;
  add_atom ( atom );
  return ( TRUE );
}



void serialize_atoms
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
    *query, *doc1, *doc2, t;
    
  atom_list = create_list ();
  assert ( atom_list != NULL );
  
  /* Read preference file. Only optimize atoms which occur in query 
     and at least one document */
  
  serial = i = 0;
  
  /* Read EVAL_PREF file, line by line */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    n = sscanf ( line, " %c %d %d %d", &type, &q, &d1, &d2 );
    assert ( n == 4 );
    
    /* Don't optimize weights in satisfied preferences. */
    if ( ( type == '+' ) || ( type == 'C' ) ) continue;
   
    /* Get concept lists of query and both docs */
    t.index = q;
    query = (DOC_STRUCT *) lookup_list ( doc_list, (ELEMENT) &t, comp_doc );
    t.index = d1;
    doc1 = (DOC_STRUCT *) lookup_list ( doc_list, (ELEMENT) &t, comp_doc );
    t.index = d2;
    doc2 = (DOC_STRUCT *) lookup_list ( doc_list, (ELEMENT) &t, comp_doc );

    assert ( query != NULL );
    assert ( doc1 != NULL );
    assert ( doc2 != NULL );
    
    /* Find union of concepts in query and doc1, build glob_atoms list */
    find_union ( query -> docatoms, doc1 -> docatoms, comp_wgt, enum_primary );
    
    /* Find union of concepts in query and doc2 */
    find_union ( query -> docatoms, doc2 -> docatoms, comp_wgt, enum_primary );
    
    /* Running count */
    i ++;
    fprintf ( counter, "%d\r", i );
  }

  fprintf ( counter, "\n" );
}


/****************************************************************
**  matrix_ptr
**
**  Gets a pointer to the matrix element in (row, col). Used to
**  simplify the procedures 'elt' and 'set'.
**  The matrix is interpreted as follows:
**
**  row = 0 .. rsv_equations - 1 : 
**        RSV equations of satisfied and unsatisfied preferences
**        (4.4 and 4.5 in the thesis, p.78 ).
**
**  row = rsv_equations .. rsv_equations + num_weights - 1 :
**        mininum weight equations (4.2)
**
**  row = rsv_equations + num_weights .. rsv_equations + 2*num_weights - 1 :
**        maximum weight equations (4.3)
**
**  row = rsv_equations + 2*num_weights:
**        cost function (4.1, p.77)
**
**  IN  : row, col = row and column of the desired element.
**
**  OUT : A pointer to the element is returned.
****************************************************************/ 

float *matrix_ptr
          ( row, col )
int
  row;
int
  col;
{
  float
    *base;

  /* Decrement row and column to map 1..x to 0..x-1 (c array
     indexing convention */
  row --;
  col --;
  assert ( row >= 0 );
  assert ( ( col >= 0 ) && ( col <= num_weights ) );

  if ( row < num_weights ) {
    /* translation equations */
    base = translation [ row ];
  }
  else {
    row -= num_weights;
    if ( row < num_prefs ) {
      /* RSV equation */
      base = rsv_eq [ row ];
    }
    else {
      row -= num_prefs;
      if ( row < num_weights ) {
        /* mininum weight constraints */
        base = min_weights [ row ];
      }
      else {
        row -= num_weights;
        if ( row < num_weights ) {
          /* maximum weight constraints */
	  base = max_weights [ row ];
        }
        else {
          /* cost function */
  	  assert ( row == num_weights );
	  base = cost;
	}
      }
    }
  }
  
  return ( & ( base [ col ] ) );
}


/****************************************************************
**  elt
**
**  Returns the value of an element of the optimization matrix
**
**  IN  : row, col = row and column of the element. The first
**                   row/column index is 0.
**
**  OUT : The function returns matrix [ row, col ].
****************************************************************/ 

#define elt(row,col)	(*matrix_ptr((row),(col)))


/****************************************************************
**  set
**
**  Sets the value of an element of the optimization matrix
**
**  IN  : row, col = row and column of the element. The first
**                   row/column index is 0.
**        value    = new value of the matrix element.
**                   matrix [ row, col ] := value;
****************************************************************/ 

#define set(row,col,value)	{ *matrix_ptr((row),(col)) = (value); }


/****************************************************************
**  calc_rsv
**
**  IN  : d1, d2 = numbers of the two documents
**        const  = pointer to variable where constant factor 
**                 of RSV is to be stored.
**        v      = pointer to vector where factors of individual
**                 weights are to be stored.
**
**  OUT : The vector v is filled with the appropriate values.
**        The value of the constant factor is returned.
****************************************************************/

BOOL union_proc
       ( a1, a2 )
ELEMENT
  a1;
ELEMENT
  a2;
{
  float
    p;
  WGT_STRUCT
    *at1, *at2;
  int
    matidx;
  IDF_STRUCT
    t, *unused;

  at1 = (WGT_STRUCT *) a1;
  at2 = (WGT_STRUCT *) a2;
  assert ( at1 -> d_atom == at2 -> d_atom );
  
  /* Multiply weights */
  p = ( at1 -> weight * at2 -> weight );
  
  /* Set vector element of this atomic concept. The atom is not
     necessarily used because there is a limit on the number of atoms
     that can be optimized. */
  matidx = matrix_index ( at1 -> d_atom );
  if ( matidx != -1 ) {
    glob_v [ matidx ] = p;
    if ( p != 0.0 ) {
      glob_bool = TRUE;
    }
  }
  else {
    /* unused atom; look up in unused_wgts and add to constant factor */
    t.u_atom = at1 -> d_atom;
    unused = (IDF_STRUCT *) lookup_list ( unused_wgts, 
               (ELEMENT) &t, comp_unused );
    assert ( unused != NULL );
    glob_const += ( p * unused -> idf );
  }

  return ( TRUE );
}


BOOL calc_rsv
         ( d1, d2, v, const )
int
  d1;
int
  d2;
float
  v [];
float
  *const;
{
  DOC_STRUCT
    t, *q, *d;

  glob_v = v;

  /* Get document descriptions of d1 and d2 and store into global vars */
  t.index = d1;
  q = (DOC_STRUCT *) lookup_list ( doc_list, (ELEMENT) &t, comp_doc );
  assert ( q != NULL );

  t.index = d2;
  d = (DOC_STRUCT *) lookup_list ( doc_list, (ELEMENT) &t, comp_doc );
  assert ( d != NULL );

  glob_const = 0.0;
  glob_bool = FALSE;
  
  find_union ( q -> docatoms, d -> docatoms, comp_wgt, union_proc );
  *const = glob_const;
  
  return ( glob_bool );
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
**  OUT : dryrun = FALSE:
**        The global vectors min_weights, max_weights, rsv_eq,
**        and cost are initialized with the appropriate values.
**        These vectors constitute the optimization matrix.
**
**        dryrun = TRUE:
**        Nothing is changed. The function returns the number
**        of actual matrix rows. The caller should then allocate
**        memory for the rsv_eq vector and call calc_equations
**        a second time with dryrun = FALSE.
****************************************************************/

float *alloc_vector
	  ( )
{
  float 
    *v;

  /* Note that calloc initializes the vector to zero */
  v = (float *) calloc ( num_weights + 1, sizeof ( float ) );
  if ( v == NULL ) {
    fprintf ( stderr, "Matrix mem alloc failed: vector %d\n", alloc_count );
    assert ( FALSE );
  }
  return ( v );
}


int enum_pref
       ( prefs, dryrun )
FILE
  *prefs;
BOOL
  dryrun;
{
  float
    c1, c2,
    *v1, *v2, 
    sign;
  int
    i, d, d1, d2,
    idx, res;
  char
    type,
    line [ LINE_LENGTH ];
  BOOL 
    nonzero1,
    nonzero2;
  int
    num_plus,
    num_minus;

  num_plus = num_minus = idx = 0;
  
  /* Read preference file again */
  while ( fgets ( line, LINE_LENGTH, prefs ) ) {
  
    /* Parse preference type and 3 document numbers */
    res = sscanf ( line, "%c %d %d %d", &type, &d, &d1, &d2 );
    assert ( res == 4 );

    /* Calculate RSV( d, d1 ) and RSV( d, d2 ) */
    v1 = alloc_vector ();
    v2 = alloc_vector ();
    nonzero1 = calc_rsv ( d, d1, v1, &c1 );
    nonzero2 = calc_rsv ( d, d2, v2, &c2 );

    /* check if any atomic concept occurs in one of the documents and the query */
    if ( ( ! nonzero1 ) && ( ! nonzero2 ) ) {
      /* No common concepts, so ignore preference */
      free ( v1 );
      free ( v2 );
      continue;
    }
    
    /* Update control counts */
    if ( type == '+' ) {
      num_plus ++;
    }
    else if ( type == '-' ) {
      num_minus ++;
    }
    
    if ( dryrun ) {
      /* Don't actually perform calculations; simply count number of
         preferences with common concepts */
      if ( type != 'C' ) idx ++;
      free ( v1 );
      free ( v2 );
      continue;
    }
    
    /* Get difference between both RSV values; negate vector elements if
       this preference is satisfied (equation 4.4) */
    if ( ( type == '-' ) || ( type == 'C' ) ) {
      /* unsatisfied */
      sign = 1.0;
    }
    else {
      /* satisfied */
      sign = -1.0;
    }
    
    /* Add result to matrix and throw away v1, v2 */
    for ( i = 0; i < num_weights; i ++ ) {
      v1 [i] = ( v1 [i] - v2 [i] ) * sign;
    }
    v1 [ num_weights ] = sign * ( EPSILON + ( c1 - c2 ) );
 
    if ( type != 'C' ) {
      rsv_eq [ idx ] = v1;
      alloc_count ++;
      idx ++;
    }
    free ( v2 );

    /* If this is an unsatisfied preference, add vector v1 to cost function */
    if ( ( type == '-' ) || ( type == 'C' ) ) {
      for ( i = 0; i <= num_weights; i ++ ) {
	/* We perform a subtraction because cost must be MINIMIZED */
        cost [i] -= v1 [i];
      }
      if ( type == 'C' ) {
        free ( v1 );
      }
    }
  }
  
  fprintf ( stderr, "Preferences: %d\n", idx );
  fprintf ( stderr, "Total + : %d, - : %d\n", num_plus, num_minus );
  assert ( num_plus + num_minus == idx );
  
  /* Return number of matrix rows */
  return ( idx );
}


void calc_equations
       ( prefs, x )
FILE
  *prefs;
float
  x [];
{
  int
    i;
  float
    *arr;

  /* Satisfied and unsatisfied preferences and cost function */
  cost = alloc_vector ();
  fprintf ( stderr, "   cost/rsv simulation\n" );
  num_prefs = enum_pref ( prefs, TRUE );
  
  /* Allocate sufficient memory for the matrix rows */
  rsv_eq = (float **) calloc ( num_prefs, sizeof ( float * ) );
  assert ( rsv_eq != NULL );

  /* Call enum_pref a second time */
  rewind ( prefs );
  fprintf ( stderr, "   cost/rsv calculation\n" );
  enum_pref ( prefs, FALSE );

  /* List of unused weights is no longer needed */
  destroy_list ( &unused_wgts );
  
  /* Allocate arrays of pointers to vectors */
  min_weights = (float **) calloc ( num_weights, sizeof ( float * ) );
  assert ( min_weights != NULL );
  max_weights = (float **) calloc ( num_weights, sizeof ( float * ) );
  assert ( max_weights != NULL );

  /* Low and high bounds of atomic weights */
  fprintf ( stderr, "   min/max\n" );
  for ( i = 0; i < num_weights; i ++ ) {

    /* Low bounds */
    arr = alloc_vector ();
    arr [i] = 1.0;
    arr [ num_weights ] = - C1 * x [i];
    min_weights [i] = arr;
   
    /* High bounds */
    arr = alloc_vector ();
    arr [i] = -1.0;
    arr [ num_weights ] = C2 * x [i];
    max_weights [i] = arr;
  }
}


/****************************************************************
**  print_results
**  
**  Prints the optimized weights of the atomic concepts in
**  the correct order.
**
**  IN  : x = vector which contains optimized weights.
****************************************************************/

BOOL enum_results
       ( a )
ELEMENT
  a;
{
  ATOM_STRUCT
    *atm;

  /* The number of an atom is usually not equal to its index in
     the weight vector */
  atm = (ATOM_STRUCT *) a;
  printf ( "%d\t%f\n", atm -> atom, glob_v [ atm -> mat_index ] );
  return ( TRUE );
}


void print_results
       ( x )
float
  x [];
{
  /* glob_v is accessed by enum_results */
  glob_v = x;
  enum_list ( atom_list, enum_results, ENUM_FORWARD );
}


/****************************************************************
**  eliminate 
**
**  Performs elimination of free variables (see book, pp.72)
**
**  IN  : n = Number of variables to be maximized.
**        m = Number of constraint equations to be satisfied.
**        a = Matrix with n+1 columns and m+1 rows. Rows 1..m
**            contain the m constraint equations y[i]. Row m+1
**            contains the equation z to be maximized. Columns
**            1..n contain the coefficients a[ik] (resp. b[k]
**            in row m+1). Column n+1 contains the constants
**            c[i] (resp. d in row m+1). The matrix is accessed
**            with the read procedure 'elt' and the write 
**            procedure 'set'.
**         ba,nb,trans = the corresponding arrays in the 'simplex'
**                       procedure.
****************************************************************/

void eliminate
       ( n, m, ba, nb, trans )
int
  n;
int
  m;
int
  *ba;
int
  *nb;
BOOL
  *trans;
{
  int
    h, i, k, p, q;
  float
    max,
    pivot,
    quot,
    temp;

  /* Eliminate all 'n' free variables; pivot column is q */
  for ( q = 1; q <= n; q ++ ) {

    p = 0;
    if ( elt ( m + 1, q ) > 0.0 ) { 
      max = -99999999999.9;  /* This should read 'minus infinity' */
      for ( i = 1; i <= m; i ++ ) {
	if ( ! trans [i] ) {
          temp = elt ( i, q );
          if ( temp < 0.0 ) {
            quot = elt ( i, n + 1 ) / temp;
	    if ( quot >= 0.0 ) {
	      fprintf ( stderr, "eliminate: inaccuracy, val = %f\n", elt
	      ( i, n + 1 ) );
	      set ( i, n + 1, 0.0 );
	      quot = 0.0;
            }
	    assert ( quot <= 0.0 );
            if ( quot > max ) {	
              p = i;
              max = quot;
	    }
	  }
        }
      }
    }
    else {
      max = 99999999999.9;  /* This should read 'infinity' */
      for ( i = 1; i <= m; i ++ ) {
	if ( ! trans [i] ) {
          temp = elt ( i, q );
          if ( temp > 0.0 ) {
            quot = elt ( i, n + 1 ) / temp;
	    if ( quot < 0.0 ) {
	      fprintf ( stderr, "eliminate: inaccuracy, val = %f\n", elt
	      ( i, n + 1 ) );
	      set ( i, n + 1, 0.0 );
	      quot = 0.0;
            }
	    assert ( quot >= 0.0 );
            if ( quot < max ) {	
              p = i;
              max = quot;
	    }
          }
        }
      }
    }

    /* If p is zero, the system does not have a solution! */
    assert ( p != 0 );

    /* Step 'AT' */
    h = nb [p];
    nb [p] = ba [q];
    ba [q] = h;
    trans [p] = TRUE;
    pivot = elt ( p, q );

    for ( k = 1; k <= n + 1; k ++ ) {
      if ( k != q ) {
        set ( p, k, - elt ( p, k ) / pivot );
        for ( i = 1; i <= m + 1; i ++ ) {
	  if ( i != p ) {
            set ( i, k, elt (i, k) + elt (i, q) * elt (p, k) );
	  }
        }
      }
    }

    for ( i = 1; i <= m + 1; i ++ ) {
      set ( i, q, elt ( i, q ) / pivot );
    }
    set ( p, q, 1.0 / pivot );

    /* Running count */
    fprintf ( counter, "%d\r", q );
  }

  fprintf ( counter, "\n" );
}


/****************************************************************
**  simplex
**
**  Optimization procedure. Processes a system of m constraint
**  equations with n variables.
**
**  The m constraint equations are of the form
**
**     y[i] = sum ( k=1..n, a[ik]*x[k] + c[i] ) >= 0. (i=1..m)
**
**  The maximum equation z has the form
**
**     z = sum ( k=1..n, b[k]*x[k] + d ) = max!
**
**  IN  : n = Number of variables to be maximized.
**        m = Number of constraint equations to be satisfied.
**        a = Matrix with n+1 columns and m+1 rows. Rows 1..m
**            contain the m constraint equations y[i]. Row m+1
**            contains the equation z to be maximized. Columns
**            1..n contain the coefficients a[ik] (resp. b[k]
**            in row m+1). Column n+1 contains the constants
**            c[i] (resp. d in row m+1). The matrix is accessed
**            with the read procedure 'elt' and the write 
**            procedure 'set'.
**        x = Pointer to a real vector of dimension n. This
**            vector must be initialized with a valid solution
**            which satisfies the constraint functions.
**            The solution (0, 0, ..) is valid if all c[i] >= 0.
**
**  OUT : If the system has a solution, then the vector x
**        contains the optimal values of the n variables x[1]..x[n]
**        and the function returns TRUE.
**        Otherwise, the function returns FALSE and the contents
**        of x are invalid.
****************************************************************/

BOOL simplex
       ( n, m, x )
int
  n;
int
  m;
float
  x [];
{
  int
    h, i, j, k, p, q, temp2,
    iterations,
    *ba,     /* dynamic array, dim n */
    *nb;     /* dynamic array, dim m */
  BOOL
    *trans;	/* dynamic, dim m */
  float
    max,
    pivot,
    quot,
    temp;

  /* Allocate ba and nb vectors; dimension n+1 so we can address
     from 1..n */
  ba = (int *) calloc ( n + 1, sizeof ( int ) );
  assert ( ba != NULL );
  nb = (int *) calloc ( m + 1, sizeof ( int ) );
  assert ( nb != NULL );

  /* Automatically initialized to FALSE */
  trans = (BOOL *) calloc ( m + 1, sizeof ( BOOL ) );
  assert ( trans != NULL );
  
  /* Initialize 'ba' vector elements 0..n-1 */
  for ( k = 1; k <= n; k ++ ) {
    ba [k] = k;
  }

  /* Initialize 'nb' vector elements 0..m-1 */
  for ( i = 1; i <= m; i ++ ) {
    nb [i] = -i;
  }

  /* Transform coordinate system by the amount of vector x; don't
     process transformation equations */
  for ( i = n + 1; i <= m + 1; i ++ ) {
    temp = 0.0;
    for ( k = 1; k <= n; k ++ ) {
      temp += elt ( i, k ) * x [k - 1];
    }
    set ( i, n + 1, elt ( i, n + 1 ) + temp );   /* a[in] is c[i] */
    if ( i <= m ) {
      if ( elt ( i, n + 1 ) < 0.0 ) {
	fprintf ( stderr, "Warning: inaccuracy, row %d, val %f\n", i, elt
	( i, n + 1 ) );
	set ( i, n + 1, 0.0 );
      }
    }
  }

  fprintf ( stderr, "Elimination.\n" );
  eliminate ( n, m, ba, nb, trans );

#ifndef NDEBUG
  /* Perform test to see if all c[i] elements positive (except those
     in the translation equations */
  for ( i = 1; i <= m; i ++ ) {
    if ( ! trans [i] ) {
      if ( elt ( i, n + 1 ) < 0.0 ) {
	fprintf ( stderr, "inaccurracy after elimination, val=%f\n", 
	  elt ( i, n + 1 ) );
        set ( i, n + 1, 0.0 );
      }
      assert ( elt ( i, n + 1 ) >= 0.0 );
    }
  }
#endif

  fprintf ( stderr, "Calculation loop start.\n" );
  iterations = 0;
  
  do {
    q = 0;
    max = 0.0;

    /* Step 'PIV' */
    for ( k = 1; k <= n; k ++ ) {
      temp = elt ( m + 1, k );
      if ( temp > max ) {
        q = k;
        max = temp;
      }
    }

    /* Exit while-loop if q is zero */
    if ( q == 0 ) break;

    p = 0;
    max = -99999999999.9;  /* This should read 'minus infinity' */
    for ( i = 1; i <= m; i ++ ) {
      /* Translation equations are not considered since they do not 
	 exist in the scheme anymore */
      if ( ! trans [i] ) {
        temp = elt ( i, q );
        if ( temp < 0.0 ) {
          quot = elt ( i, n + 1 ) / temp;
          if ( quot > max ) {	
            p = i;
            max = quot;
          }
        }
      }
    }

    /* If p is zero, the system does not have a solution! */
    if ( p == 0 ) {
      return ( FALSE );
    }

    /* Step 'AT' */
    h = nb [p];
    nb [p] = ba [q];
    ba [q] = h;
    pivot = elt ( p, q );

    for ( k = 1; k <= n + 1; k ++ ) {
      if ( k != q ) {
        set ( p, k, - elt ( p, k ) / pivot );
        for ( i = 1; i <= m + 1; i ++ ) {
          if ( i != p ) {
            set ( i, k, elt (i, k) + elt (i, q) * elt (p, k) );
          }
        }
      }
    }

    for ( i = 1; i <= m + 1; i ++ ) {
      set ( i, q, elt ( i, q ) / pivot );
    }

    set ( p, q, 1.0 / pivot );

    iterations ++;
  } while ( TRUE );   /* = endless loop */

  /* Step 'LOES' */
  for ( i = 1; i <= m; i ++ ) {
    temp2 = nb [i];
    j = abs ( temp2 );
    if ( temp2 > 0 ) {
      /* Implicit retransformation of coordinate system by adding the amount */
      x [ j - 1 ] += elt ( i, n + 1 );
    }
  }

  fprintf ( stderr, "Iterations: %d\n", iterations );
  return ( TRUE );
}


/****************************************************************
**  destroy_signlist
**
**  Removes the sign list and the atom lists attached to each
**  sign.
****************************************************************/

BOOL destroy_atoms
       ( e )
ELEMENT
  e;
{
  destroy_list ( &( ((SIGN_STRUCT *) e) -> atoms ) );
  return ( TRUE );
}


void destroy_signlist
       ( )
{
  enum_list ( sign_list, destroy_atoms, ENUM_FORWARD );
  destroy_list ( &sign_list );
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
  BOOL
    ok;
  FILE
    *prefs, *f;
  float
    *x;   /* Pointer to the (dynamic) solution vector */

  /* Program title */
  fprintf ( stderr, PROG );
  fprintf ( stderr, "Parameters: C1 = %f, C2 = %f\n", C1, C2 );

  /* Get verbose or quiet mode */
  if ( ( argc == 6 ) && ( *argv [5] == 'Q' ) ) {
    /* in case of quiet mode: redirect running counts to /dev/null */
    counter = fopen ( "/dev/null", "r" );
    assert ( counter != NULL );
  }
  else {
    /* verbose mode: redirect running counts to stderr */
    counter = stderr;
  }

  /* Check parameters */
  if ( argc < 5 ) {
    fprintf ( stderr, USAGE );
    return ( 1 );
  }
  
  /* Read preferences into memory */
  fprintf ( stderr, "Reading preferences.\n" );
  prefs = open_file ( argv [1] );
  process_pref ( prefs );
  fclose ( prefs );

  /* Read atomic concepts of each sign */
  fprintf ( stderr, "Reading atomic concepts.\n" );
  f = open_file ( argv [3] );
  process_concepts ( f );
  fclose ( f );
  
  /* Read document descriptions (only those which are necessary) */
  fprintf ( stderr, "Reading document descriptions.\n" );
  f = open_file ( argv [2] );
  process_documents ( f );
  fclose ( f );
  
  /* Destroy sign list to save memory */
  destroy_signlist ();
  
  /* Create ranking of atoms */
  fprintf ( stderr, "Serializing atoms.\n" );
  prefs = open_file ( argv [1] );
  serialize_atoms ( prefs );
  fclose ( prefs );
  num_weights = count_list ( atom_list );
  fprintf ( stderr, "Weights to optimize: %d\n", num_weights );
  
  /* Initialize solution vector with IDF of all atomic concepts.
     First, allocate memory for solution vector */
  fprintf ( stderr, "Initializing weights.\n" );
  x = alloc_vector ();
  f = open_file ( argv [4] );
  init_weights ( f, x );
  fclose ( f );
  
  /* Calculate coefficients for each RSV constraint */
  fprintf ( stderr, "Calculating RSV values.\n" );
  prefs = open_file ( argv [1] );
  calc_equations ( prefs, x );
  fclose ( prefs );
  
  /* Tackle the optimization problem */
  fprintf ( stderr, "Simplex algorithm.\n" );
  
  ok = simplex ( num_weights, num_weights * 3 + num_prefs, x );
  assert ( ok );

  /* Print results */
  fprintf ( stderr, "Printing results.\n" );
  print_results ( x );

  return ( 0 );
}
