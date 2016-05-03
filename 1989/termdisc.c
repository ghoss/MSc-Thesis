/****************************************************************
*
*           S O F T W A R E   S O U R C E   F I L E
*
*****************************************************************
*
*   Name of file   : termdisc.c
*   Author         : Guido Hoss
*   Project        : ETH Diploma Thesis (SS 1989)
*   Creation Date  : 15/08/89
*   Type of file   : C Language File
*
*   Description
*   -----------
*   Calculates average distance between documents and the
*   collection centroid.
*
*   Call Format
*   -----------
*
*      termdisc <doc-descr>
*
*   where <doc-descr> is the path of the term weight
*   file. The average distance is written to the standard error output.
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

#define PROG	"Centroid Distance Calculation (gh, 15/08/89)\n"
#define USAGE	"Usage: termdisc <doc-descr> [QUIET]\n"

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <assert.h>

#include "boolean.h"
#include "list.h"
#include "util.h"

#define MAX_WORDLEN	100
#define LINE_LENGTH	100

typedef
  struct {
    int  index;    /* number of this document */
    LIST signs;    /* weights of the signs occuring in the document */
  } DOC_STRUCT;
  
typedef
  struct {
    int   sign;    /* number of sign */
    float weight;  /* weight of sign in document to which this list belongs */
  } WGT_STRUCT;

LIST
  centroid,	/* Signs of centroid */
  doc_list;	/* List of documents and signs belonging to them */
float
  glob_dist,
  temp_dist,
  numdocs;	/* Number of documents */
  
  
/****************************************************************
**  read_wgts
** 
**  Reads the doc-descr file.
**
**  IN  : f = handle to open DOC-DESCR file.
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


int comp_wgt
      ( s1, s2 )
ELEMENT
  s1;
ELEMENT
  s2;
{
  return ( ((WGT_STRUCT *) s1) -> sign - ((WGT_STRUCT *) s2) -> sign );
}


void read_wgts 
       ( f )
FILE
  *f;
{
  char
    line [ LINE_LENGTH ];
  DOC_STRUCT
    *doc;
  WGT_STRUCT
    *wgt;
  LIST
    list;
  float 
    w;
  BOOL
    active;
  int
    d, n;

  /* Create sign and document list */
  doc_list = create_list ();
  assert ( doc_list != NULL );
  centroid = create_list ();
  assert ( centroid != NULL );

  numdocs = 0.0;
  
  /* Read file line by line */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    /* Interpret as much fields as possible */
    n = sscanf ( line, " %d %f", &d, &w );

    switch ( n ) {
      
      case 1 :
        /* One field -> new document number; update running count */
	active = ( d >= 0 );
	if ( ! active ) continue;

        doc = (DOC_STRUCT *) malloc ( sizeof ( DOC_STRUCT ) );
        assert ( doc != NULL );
        doc -> index = d;
        doc -> signs = NULL;
        doc = (DOC_STRUCT *) add_list ( doc_list, (ELEMENT) doc, comp_doc );
        assert ( doc != NULL );
        assert ( doc -> signs == NULL );
        doc -> signs = create_list ();
        list = doc -> signs;
        assert ( list != NULL );
        
        numdocs ++;
        break;
        
      case 2 :
        /* Two fields -> sign index and weight; add sign to list of doc */
        if ( ! active ) continue;

        /* Add sign and weight to list of current document */
        wgt = (WGT_STRUCT *) malloc ( sizeof ( WGT_STRUCT ) );
        assert ( wgt != NULL );
        wgt -> sign = d;
        wgt -> weight = w;
        wgt = (WGT_STRUCT *) add_list ( list, (ELEMENT) wgt, comp_wgt );
        assert ( wgt != NULL );
        
        /* Add sign and weight to centroid */
        wgt = (WGT_STRUCT *) malloc ( sizeof ( WGT_STRUCT ) );
        assert ( wgt != NULL );
        wgt -> sign = d;
        wgt -> weight = 0.0;
        
        /* Will be discarded automatically if already entered */
        wgt = (WGT_STRUCT *) add_list ( centroid, (ELEMENT) wgt, comp_wgt );
        assert ( wgt != NULL );
        wgt -> weight += w;
        
        break;
    }
  }
  
  fprintf ( stderr, "\n" );
}


/****************************************************************
**  calc_average
**
**  Calculates the average distance between documents and
**  centroid.
****************************************************************/

BOOL enum_signs1
       ( s1, s2 )
ELEMENT
  s1;
ELEMENT
  s2;
{
  float
    temp;
    
  /* Enumeration procedure for signs common to document and centroid.
     centroid sign is in s2 */
  temp = ((WGT_STRUCT *) s2) -> weight / numdocs - 
         ((WGT_STRUCT *) s1) -> weight;
  temp_dist += temp * temp;
  return ( TRUE );
}


BOOL enum_signs2
       ( s )
ELEMENT
  s;
{
  float
    temp;
    
  /* Enumeration procedure for unique signs in document */
  temp = ((WGT_STRUCT *) s) -> weight;
  temp_dist += temp * temp;
  return ( TRUE );
}


BOOL enum_signs3
       ( s )
ELEMENT
  s;
{
  float
    temp;
    
  /* Enumeration procedure for unique signs in centroid */
  temp = ((WGT_STRUCT *) s) -> weight / numdocs; 
  temp_dist += temp * temp;
  return ( TRUE );
}


float dist
        ( d )
DOC_STRUCT
  *d;
{
  temp_dist = 0.0;
  
  /* Subtract document vector d from centroid and square */
  
  /* Handle common signs */
  find_union ( d -> signs, centroid, comp_wgt, enum_signs1 );
  
  /* Handle signs only in centroid */
  find_diff ( centroid, d -> signs, comp_wgt, enum_signs3 );
  
  /* Handle signs only in d */
  find_diff ( d -> signs, centroid, comp_wgt, enum_signs2 );
  
  return ( sqrt ( temp_dist ) );
}


BOOL enum_docs
       ( e )
ELEMENT
  e;
{
  DOC_STRUCT
    *d;
  
  d = (DOC_STRUCT *) e;
  glob_dist += dist ( d );
  return ( TRUE );
}


void calc_average 
       ( )
{
  glob_dist = 0.0;
  
  /* Sum up for all documents */
  enum_list ( doc_list, enum_docs, ENUM_FORWARD );
   
  fprintf ( stderr, "Average distance: %f\n", glob_dist / numdocs );
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
  
  /* Check parameters */
  if ( argc < 2 ) {
    fprintf ( stderr, USAGE );
    return ( 1 );
  }
  
  /* Read sign weights into memory */
  fprintf ( stderr, "Reading sign weights.\n" );
  f = open_file ( argv [1] );
  read_wgts ( f );
  fclose ( f );
  
  /* Calculate average value */
  fprintf ( stderr, "Documents: %d\n", (int) numdocs );
  fprintf ( stderr, "Calculating average.\n" );
  calc_average ();
  
  return ( 0 );
}
