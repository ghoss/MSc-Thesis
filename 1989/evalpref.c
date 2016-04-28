/****************************************************************
*
*           S O F T W A R E   S O U R C E   F I L E
*
*****************************************************************
*
*   Name of file   : evalpref.c
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
*	eval_prefs <relevant> <rsv> [QUIET]
*
*   where <relevant> is a list of queries and relevant documents
*   for each query, and <rsv> is a collection of retrieval status
*   values for queries and documents.
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
*   Git repository home: <https://github.com/ghoss/MSc-Thesis>
*
*****************************************************************
* Date        :
* Description :
****************************************************************/   

#define PROG	"Preference Evaluation (gh, 06/05/89)\n"
#define USAGE	"Usage: eval_prefs <relevant> <rsv> [QUIET]\n"

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



typedef
  struct {
    int  index;		/* index of a query */
    LIST relevant;	/* list of relevant documents for this query */
    BOOL handled : 1;	/* TRUE if query already seen (error!) */
  } QUERY_STRUCT;

typedef
  struct {
    int    doc;			/* index of a document */
    int    rlevel;		/* relevance level: 0=not relevant for curr qry */
    double rsv;			/* rsv between the query and this document */
  } DOC_STRUCT;

typedef
  struct {
    int		reldoc;		/* index of a relevant document */
    int		rellevel;	/* relevance level */
    BOOL	seen : 1;	/* relevant document has nonzero RSV */
  } RLV_STRUCT;

LIST
  queries,	/* List of queries and relevant documents */
  doc_list;	/* temporary ranking list for a query */

int
  req_query,
  useful_plus,
  useful_minus,
  total_plus,	/* Total number of preferences (satisfied/unsatisfied) */
  total_minus;
  
/* Global variables used by 'enum_query' */
int
  glob_level,	/* Relevance level of current query */
  glob_query;	/* Number of current query being processed */
DOC_STRUCT
  *glob_doc;
LIST 
  glob_ranking;
FILE
  *counter;	/* Virtual file used to output running counts */


/****************************************************************
**  Forward declarations (compiler type checking)
****************************************************************/  

#ifndef BSDUNIX
void load_rsv ( FILE * );
void load_relevant ( FILE * );
int comp_doc ( ELEMENT, ELEMENT );
int comp_query ( ELEMENT, ELEMENT );
int comp_relevant ( ELEMENT, ELEMENT );
BOOL enum_doc1 ( ELEMENT );
BOOL enum_doc2 ( ELEMENT );
void enum_query ( QUERY_STRUCT * );
void add_zerorsv ( LIST, LIST );
int main ( int, char * [] );
#else
void load_rsv ();
void load_relevant ();
int comp_doc ();
int comp_query ();
int comp_relevant ();
BOOL enum_doc1 ();
BOOL enum_doc2 ();
void enum_query ();
void add_zerorsv ();
int main ();
#endif
 
/****************************************************************
**  load_relevant
**
**  Loads a list of relevant documents for each query into
**  memory.
**
**  IN  : f = handle to open RELEVANT file.
****************************************************************/   

int comp_query
      ( q1, q2 )
ELEMENT
  q1;
ELEMENT
  q2;
{
  return ( ((QUERY_STRUCT *) q1) -> index - ((QUERY_STRUCT *) q2) -> index );
}


int comp_relevant
      ( d1, d2 )
ELEMENT
  d1;
ELEMENT
  d2;
{
  return ( ((RLV_STRUCT *) d1) -> reldoc - ((RLV_STRUCT *) d2) -> reldoc );
}


void load_relevant
       ( f )
FILE
  *f;
{
  char
    line [ LINE_LENGTH ];
  int
    level, n, res;
  QUERY_STRUCT
    *curr_query, *q;
  RLV_STRUCT
    *d;

  /* Create global 'queries' list */
  queries = create_list ();
  assert ( queries != NULL );
  
  /* Read file one line at a time */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
    
    /* Line contains number, optionally followed by colon */
    res = sscanf ( line, " %d %d", &n, &level );
    assert ( res >= 1 );
    
    /* If number followed by a colon, then this is a query index;
       create new list entry for this query */
    if ( strchr ( line, ':' ) != NULL ) {
      if ( n >= 0 ) {
        /* Query numbers must be made negative */
        n = -n;
      }
      q = (QUERY_STRUCT *) malloc ( sizeof ( QUERY_STRUCT ) );
      assert ( q != NULL );
      q -> index = n;
      q -> relevant = NULL;
      q -> handled = FALSE;
      q = (QUERY_STRUCT *) add_list ( queries, (ELEMENT) q, comp_query );
      assert ( q != NULL );
      if ( q -> relevant == NULL ) {
        /* Make new list of relevant documents for this query */
        q -> relevant = create_list ();
        assert ( q -> relevant != NULL );
      }
      curr_query = q;
      
      /* Running count */
      fprintf ( counter, "%d\r", n );
    }
    else {
      /* document relevant to 'curr_query' */
      d = (RLV_STRUCT *) malloc ( sizeof ( RLV_STRUCT ) );
      assert ( d != NULL );
      d -> reldoc = n;
      if ( res == 2 ) {
        /* Relevance level specified */
        d -> rellevel = level;
      }
      else {
        /* Default level = 1 */
        d -> rellevel = 1;
      }
      d -> seen = FALSE;
      d = (RLV_STRUCT *) add_list ( curr_query -> relevant, (ELEMENT) d,
                                    comp_relevant );
      assert ( d != NULL );
    }
  }
  
  fprintf ( counter, "\n" );
}


/****************************************************************
**  add_zerorsv
**
**  Adds zero RSV values of relevant documents. These are not
**  automatically generated by the RSV calculation program.
**
**  IN  : relevant = List of relevant documents.
**        ranking  = Ranking list.
**
**  All relevant documents which do not occur in the ranking
**  list are added with RSV = 0.
****************************************************************/   

BOOL  enum_zerorel
        ( d )
ELEMENT
  d;
{
  RLV_STRUCT
    *rdoc;
  DOC_STRUCT
    *doc;
  
  rdoc = (RLV_STRUCT *) d;
  if ( rdoc -> seen ) return ( TRUE );
  
  doc = (DOC_STRUCT *) malloc ( sizeof ( DOC_STRUCT ) );
  assert ( doc != NULL );
  
  doc -> doc = ((RLV_STRUCT *) d ) -> reldoc;
  doc -> rlevel = ((RLV_STRUCT *) d ) -> rellevel;
  doc -> rsv = 0.0;
  
  doc = (DOC_STRUCT *) add_list ( glob_ranking, (ELEMENT) doc, comp_doc );
  assert ( doc != NULL );
  
  return ( TRUE );
}


void add_zerorsv
       ( relevant, ranking )
LIST
  relevant;
LIST
  ranking;
{
  glob_ranking = ranking;
  enum_list ( relevant, enum_zerorel, ENUM_FORWARD );
}


/****************************************************************
**  load_rsv
**
**  Loads RSV values between queries and documents.
**
**  IN  : f = handle to open RSV file.
****************************************************************/   

int comp_doc
      ( d1, d2 )
ELEMENT
  d1;
ELEMENT
  d2;
{
  DOC_STRUCT
    *doc1, *doc2;
  double
    delta;

  /* Rank documents by decreasing RSV value */
  doc1 = (DOC_STRUCT *) d1;
  doc2 = (DOC_STRUCT *) d2;
  delta = ( doc2 -> rsv - doc1 -> rsv );

  if ( delta > 0.0 ) {
    return ( 1 );
  }
  else if ( delta < 0.0 ) {
    return ( -1 );
  }
  else {
    /* document index decides in case of equal RSV */
    return ( doc2 -> doc - doc1 -> doc );
  }
}


void load_rsv
       ( f )
FILE
  *f;
{
  char
    line [ LINE_LENGTH ];
  int
    i, query, curr_query,
    doc, res;
  QUERY_STRUCT
    t, *q;
  DOC_STRUCT
    *d;
  RLV_STRUCT
    *rdoc,
    t1;
  float
    rsv;
  BOOL
    relevant;

  curr_query = 0;
  doc_list = NULL;
  q = NULL;

  /* Read RSV values line by line */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    /* We expect 3 arguments: query, document, rsv */
    res = sscanf ( line, " %d %d %f", &query, &doc, &rsv );
    assert ( res == 3 );
    
    /* find query index in 'queries'; queries are assumed to be
       grouped!*/
    if ( query != curr_query ) {
      if ( q != NULL ) {
	/* Add relevant documents with zero rsv to ranking list */
	add_zerorsv ( q -> relevant, doc_list );
	
	/* Generate preferences */
	enum_query ( q );
      }
      i = 0;
      t.index = query;
      q = (QUERY_STRUCT *) lookup_list ( queries, (ELEMENT) &t, comp_query );

      /* Query must be existing and not have occured before */
      assert ( q != NULL );
      assert ( q -> handled == FALSE );

      /* If this isn't the first query, destroy document lists of
         the previous query and recreate them */
      if ( doc_list != NULL ) {
	destroy_list ( &doc_list );
      }
      doc_list = create_list ();
      assert ( doc_list != NULL );
      curr_query = query;
      q -> handled = TRUE;
    }
    
    /* add current document to ranking list */
    d = (DOC_STRUCT *) malloc ( sizeof ( DOC_STRUCT ) );
    assert ( d != NULL );
    
    /* look if document is relevant */
    t1.reldoc = doc;
    rdoc = (RLV_STRUCT *) lookup_list ( q -> relevant, (ELEMENT) &t1, comp_relevant );
    relevant = ( rdoc != NULL );
    if ( relevant ) {
      rdoc -> seen = TRUE;
      d -> rlevel = rdoc -> rellevel;
    }
    else {
      d -> rlevel = 0;	/* document not relevant */
    }
    d -> rsv = (double) rsv;
    d -> doc = doc;
    
    d = (DOC_STRUCT *) add_list ( doc_list, (ELEMENT) d, comp_doc );
    assert ( d != NULL );
    
    /* running count */
    i ++;
    fprintf ( counter, "%d\r", i );
  }
  
  /* Process pending query */
  add_zerorsv ( q -> relevant, doc_list );
  enum_query ( q );

  fprintf ( counter, "\n" );
}


/****************************************************************
**  Document list enumeration procedures for unsatisfied
**  and satisfied preferences.
****************************************************************/ 

BOOL enum_doc2
       ( e )
ELEMENT
  e;
{
  int
    d1, d2;
  char
    ch;
  DOC_STRUCT
    *d;
  double
    delta;
  BOOL
    useful;
    
  d = ((DOC_STRUCT *) e );
  
  /* glob_doc is relevant. Only continue if d is less relevant */
  if ( ( d -> rlevel == glob_level - 1 ) || ( d -> rlevel == 0 ) ) {
  
    /* Compare RSV of d with RSV of glob_doc (in respect to "glob_query") */
    delta = ( d -> rsv - glob_doc -> rsv );
    
    /* Round small values */
    if ( fabs ( delta ) < EPSILON ) {
      delta = 0.0;
    }
  
    /* Since glob_doc is relevant and d less relevant, the preference reads
       "d1 <q d2" */
    d1 = d -> doc;
    d2 = glob_doc -> doc;
      
    /* Preference is unsatisfied if  delta >= 0 ( RSV(d) >= RSV(g_d) ) */
    if ( delta >= -EPSILON ) {	
      /* The usefulness of the preference depends on the values of
	 C1 and C2 */
      ch = '-';
      useful = ( C1 * d -> rsv - C2 * glob_doc -> rsv < -EPSILON );
      total_minus ++;
      if ( useful ) useful_minus ++;
    }
    else if ( delta < -EPSILON ) {
      /* Satisfied preference */
      ch = '+';
      useful = ( C2 * d -> rsv - C1 * glob_doc -> rsv >= -EPSILON );
      total_plus ++;
      if ( useful ) useful_plus ++;
    }
    else {
      /* No preference */
      return ( TRUE );
    }
    
    /* Print preference */
    if ( useful || ( ch == '-' ) ) {
      if ( ! useful ) {
        /* Unsatisfied preference, important for cost function */
        assert ( ch == '-' );
        ch = 'C';
      }
      if ( ( ( ch == '-' ) || ( ch == 'C' ) ) ) {
        if ( ( glob_query == req_query ) || ( req_query == 0 ) ) {
          printf ( "%c\t%d\t%d\t%d\t%f\n", ch, glob_query, d1, d2, delta );
        }
      }
      else if ( ch == '+' ) {
        printf ( "%c\t%d\t%d\t%d\t%f\n", ch, glob_query, d1, d2, delta );
      }
    }
  }
  
  return ( TRUE );
}



BOOL enum_doc1
       ( e )
ELEMENT
  e;
{
  /* For each RELEVANT doc: enumerate all non-relevant documents in 
     doc_list again and create preferences as required */
  glob_doc = ((DOC_STRUCT *) e );
  
  if ( glob_doc -> rlevel > 0 ) {
    /* Only create satisfied preferences for current relevance class + 1 */
    glob_level = glob_doc -> rlevel;
    enum_list ( doc_list, enum_doc2, ENUM_FORWARD );
  }
  
  return ( TRUE );
}


void enum_query
       ( q )
QUERY_STRUCT
  *q;
{
  /* Enumerate the first documents in doc_list */
  glob_query = q -> index;
  enum_list ( doc_list, enum_doc1, ENUM_FORWARD );
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
  char 
    *query;

  /* Program title */
  fprintf ( stderr, PROG );
  fprintf ( stderr, "Parameters: C1 = %f, C2 = %f\n", C1, C2 );

  /* Get verbose or quiet mode */
  if ( ( argc == 4 ) && ( *argv[3] == 'Q' ) ) {
    /* in case of quiet mode: redirect running counts to /dev/null */
    counter = fopen ( "/dev/null", "r" );
    assert ( counter != NULL );
  }
  else {
    /* verbose mode: redirect running counts to stderr */
    counter = stderr;
  }
  
  /* Argument count */
  if ( argc < 3 ) {
    fprintf ( stderr, USAGE );
    return ( 1 );
  }
  
  /* Get number of current query */
  query = (char *) getenv ( "QUERY" );
  if ( query == NULL ) {
    fprintf ( stderr, "(All queries)\n" );
    req_query = 0;
  }
  else {
    req_query = atoi ( query );
    assert ( req_query != 0 );
    if ( req_query > 0 ) {
      /* Make query number negative in any case */
      req_query = -req_query;
    }
    fprintf ( stderr, "(Query no. %d)\n", req_query );
  }

  /* Load list of relevant documents */
  f = open_file ( argv [1] );
  fprintf ( stderr, "Loading relevant documents.\n" );
  load_relevant ( f );
  fclose ( f );
  
  total_plus = total_minus = useful_plus = useful_minus = 0;
  
  /* Produce RSV ranking */
  f = open_file ( argv [2] );
  fprintf ( stderr, "Generating preferences.\n" );
  load_rsv ( f );
  fclose ( f );
  
  fprintf ( stderr, "Total + : %d, total - : %d\n", total_plus, total_minus );
  fprintf ( stderr, "Useful + : %d, useful - : %d\n", useful_plus, useful_minus );
  
  return ( 0 );
}
