/****************************************************************
*
*           S O F T W A R E   S O U R C E   F I L E
*
*****************************************************************
*
*   Name of file   : calc_pr.c
*   Author         : Guido Hoss
*   Project        : ETH Diploma Thesis (SS 1989)
*   Creation Date  : 06/05/89
*   Type of file   : C Language File
*
*   Description
*   -----------
*   Reads a list of RSV values for queries and documents and
*   generates a precision/recall table.
*
*   Call Format
*   -----------
*	calc_pr <relevant> <rsv>
*
*   where <relevant> is a list of queries and relevant documents
*   for each query, and <rsv> is a collection of retrieval status
*   values for queries and documents.
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

#define PROG	"Precision/Recall Calculation (gh, 10/05/89)\n"
#define USAGE	"Usage: calc_pr <relevant> <rsv> [QUIET]\n"

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


typedef
  struct {
    int  index;		/* index of a query */
    LIST relevant;	/* list of relevant documents for this query */
    BOOL handled : 1;	/* TRUE if query already seen (error!) */
  } QUERY_STRUCT;

typedef
  struct {
    int    doc;		    /* index of a document */
    double rsv;		    /* rsv between the query and this document */
    BOOL   relevant : 1;    /* TRUE if this document is relevant */
  } DOC_STRUCT;

typedef
  struct {
    int  reldoc;	/* index of a relevant document */
    BOOL ranked : 1;	/* TRUE if document occurs in ranking list */
  } RLV_STRUCT;

LIST
  queries,	/* List of queries and relevant documents */
  doc_list;	/* Temporary list for building PR table */

int
  total_retr,	/* total retrieved */
  total_rel,	/* total relevant */
  item_rel;	/* items retrieved so far and relevant */
  
/* Histogram array for precision/recall graph */

#define MAX_ARRAY	20	/* 20 * 0.05 = 1 */
struct ARRAY_STRUCT {
  double prec;
  double limit;
} array [ MAX_ARRAY ];

struct ARRAY_STRUCT
  sum_array [ MAX_ARRAY ];

double
  prec, recall;

int
  rank_count,
  worst_count,
  first_nrpos,	/* position of first non-rel document */
  first_nonrel,	/* number of first non-relevant doc in ranking list */
  num_queries;
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
void enum_query ( QUERY_STRUCT * );
BOOL enum_doc ( ELEMENT );
BOOL enum_worst ( ELEMENT );
BOOL enum_zerorsv ( ELEMENT );
int main ( int, char * [] );
#else
void load_rsv ();
void load_relevant ();
int comp_doc ();
int comp_query ();
int comp_relevant ();
BOOL enum_doc ();
void enum_query ();
BOOL enum_worst ();
BOOL enum_zerorsv ();
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
  return ( abs ( ((QUERY_STRUCT *) q1) -> index ) - 
           abs ( ((QUERY_STRUCT *) q2) -> index ) );
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
    n, res;
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
    res = sscanf ( line, " %d", &n );
    assert ( res == 1 );
    
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
      d -> ranked = FALSE;
      d = (RLV_STRUCT *) add_list ( curr_query -> relevant, (ELEMENT) d,
                                    comp_relevant );
      assert ( d != NULL );
    }
  }
  
  fprintf ( counter, "\n" );
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
  double
    delta;
  DOC_STRUCT
    *doc1, *doc2;
  
  doc1 = (DOC_STRUCT *) d1;
  doc2 = (DOC_STRUCT *) d2;
  
  /* Order documents by decreasing rsv value */
  delta = ( doc2 -> rsv - doc1 -> rsv );
  if ( delta > 0.0 ) {
    return ( 1 );
  }
  else if ( delta < 0.0 ) {
    return ( -1 );
  }
  else {
    /* if delta 0.0, sort by document index */
    return ( doc1 -> doc - doc2 -> doc );
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
    *rd, t1;
  float
    rsv;

  i = 0;
  curr_query = 0;
  doc_list = NULL;
  q = NULL;

  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    /* We expect 3 arguments: query, document, rsv */
    res = sscanf ( line, " %d %d %f", &query, &doc, &rsv );
    assert ( res == 3 );
    
    /* find query index in 'queries'; queries are assumed to be
       grouped!*/
    if ( query != curr_query ) {
      if ( q != NULL ) {
	/* Generate PR-graph */
	enum_query ( q );
      }
      t.index = query;
      q = (QUERY_STRUCT *) lookup_list ( queries, (ELEMENT) &t, comp_query );
      assert ( q != NULL );
      assert ( q -> handled == FALSE );
      if ( doc_list != NULL ) {
	destroy_list ( &doc_list );
      }
      doc_list = create_list ();
      assert ( doc_list != NULL );
      curr_query = query;
      q -> handled = TRUE;
    }

    /* add document to ranking list */
    d = (DOC_STRUCT *) malloc ( sizeof ( DOC_STRUCT ) );
    assert ( d != NULL );
    
    /* look if document is relevant */
    t1.reldoc = doc;
    rd = (RLV_STRUCT *) lookup_list ( q -> relevant, (ELEMENT) &t1, 
                                    comp_relevant );
    d -> relevant = ( rd != NULL );
    d -> rsv = (double) rsv;
    d -> doc = doc;
    if ( d -> relevant ) {
      rd -> ranked = TRUE;
    }
    
    d = (DOC_STRUCT *) add_list ( doc_list, (ELEMENT) d, comp_doc );
    assert ( d != NULL );
    
    /* running count */
    i ++;
    fprintf ( counter, "%d\r", i );
  }
  
  /* Process pending query */
  enum_query ( q );

  fprintf ( counter, "\n" );
}


/****************************************************************
**  enum_query -> enum_doc, enum_worst, enum_zerorsv
**
**  Ranking list enumeration procedures.
****************************************************************/ 

BOOL enum_doc
       ( d )
ELEMENT
  d;
{
  DOC_STRUCT 
    *doc;
  double
    prec, recall;
  int
    i;

  doc = (DOC_STRUCT *) d;
  
  /* Update total retrieved and items retrieved and relevant */
  total_retr ++;
  if ( doc -> relevant ) {
    item_rel ++;
  }
  else {
    /* non-relevant document */
    if ( first_nonrel == 0 ) {
      /* save index of first non-relevant */
      first_nonrel = doc -> doc;
      first_nrpos = rank_count;
    }
     
    /* return because only precision changed, but recall is the same */
    return ( TRUE );
  }

  prec = ( (double) item_rel / (double) total_retr );	/* precision */
  recall = ( (double) item_rel / (double) total_rel );	/* recall */
  
  /* Update histogram */
  for ( i = 0; i < MAX_ARRAY; i ++ ) {
    if ( ( array [i].limit <= recall ) && ( array [i].prec < prec ) ) {
      array [i].prec = prec;
    }
  }

  rank_count ++;
  return ( item_rel < total_rel );
}


BOOL enum_bestnonrel
       ( d )
ELEMENT
  d;
{
  DOC_STRUCT 
    *doc;

  doc = (DOC_STRUCT *) d;
  
  if ( ! doc -> relevant ) {
    printf ( "\t%d. %d\n", rank_count, doc -> doc );
    worst_count --;
  }
  
  rank_count ++;
  return ( worst_count > 0 );
}


BOOL enum_worst
       ( d )
ELEMENT
  d;
{
  DOC_STRUCT 
    *doc;

  doc = (DOC_STRUCT *) d;
  
  if ( doc -> relevant ) {
    printf ( "\t%d. %d\n", rank_count, doc -> doc );
    worst_count --;
  }
  
  rank_count --;
  return ( worst_count > 0 );
}


BOOL enum_zerorsv
       ( d )
ELEMENT
  d;
{
  RLV_STRUCT 
    *doc;

  doc = (RLV_STRUCT *) d;
  
  if ( ! doc -> ranked ) {
    printf ( "%d  ", doc -> reldoc );
  }
  
  return ( TRUE );
}


void enum_query
       ( query )
QUERY_STRUCT
  *query;
{
  int
    i;

  /* Produce table for each query */
  total_rel = count_list ( query -> relevant );
  if ( total_rel == 0 ) {
    /* No relevant documents for this query */
    return;
  }
  total_retr = 0;
  item_rel = 0;
  num_queries ++;
  for ( i = 0; i < MAX_ARRAY; i ++ ) {
    array [i].limit = sum_array [i].limit;
    array [i].prec = 0.0;
  }
  
  first_nonrel = 0;
  rank_count = 1;
  enum_list ( doc_list, enum_doc, ENUM_FORWARD );

  printf ( "QUERY %d - total %d, relevant %d, 1st nonrel = %d. %d\n", 
           abs ( query -> index ), count_list ( doc_list ),
           count_list ( query -> relevant ), first_nrpos, first_nonrel );

  /* Find worst 5 documents in ranking list */
  worst_count = 5;
  rank_count = count_list ( doc_list );
  enum_list ( doc_list, enum_worst, ENUM_BACKWARD );
  printf ( "------- best non-relevant:\n" );
  
  /* Find 5 best non-relevant documents */
  rank_count = 1;
  worst_count = 5;
  enum_list ( doc_list, enum_bestnonrel, ENUM_FORWARD );
  printf ( "\n" );

  /* Find 5 relevant documents with zero RSV */
  printf ( "RSV zero:  " );
  enum_list ( query -> relevant, enum_zerorsv, ENUM_FORWARD );
  printf ( "\n\n" );
  
  /* Add sums to global histogram */
  for ( i = 0; i < MAX_ARRAY; i ++ ) {
    sum_array [i].prec += array [i].prec;
  }
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
  int
    i;
  double
    area;

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
  
  /* Argument count */
  if ( argc < 3 ) {
    fprintf ( stderr, USAGE );
    return ( 1 );
  }
  
  /* Load list of relevant documents */
  f = open_file ( argv [1] );
  fprintf ( stderr, "Loading relevant documents.\n" );
  load_relevant ( f );
  fclose ( f );
  
  /* Initialize precision/recall graph to trace values in 0.05 increments */
  num_queries = 0;
  for ( i = 0; i < MAX_ARRAY; i ++ ) {
    sum_array [i].prec = 0.0;
    sum_array [i].limit = (double) i * 0.05;
  }

  /* Produce RSV ranking */
  f = open_file ( argv [2] );
  fprintf ( stderr, "Loading RSV values.\n" );
  load_rsv ( f );
  fclose ( f );
  
  /* Normalize histogram */
  printf ( "-------\nGlobal average for %d queries\n-------\n", num_queries );
  printf ( " R \t P \n---\t---\n" );
  area = 0.0;
  for ( i = 0; i < MAX_ARRAY; i ++ ) {
    sum_array [i].prec /= (double) num_queries;
    area += sum_array [i].prec; 
    printf ( "%f\t%f\n", sum_array [i].limit, sum_array [i].prec );
  }

  printf ( "\nCurve sum = %f\n", area );
  
  return ( 0 );
}
