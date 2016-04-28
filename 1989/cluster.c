/****************************************************************
*
*           S O F T W A R E   S O U R C E   F I L E
*
*****************************************************************
*
*   Name of file   : cluster.c
*   Author         : Guido Hoss
*   Creation Date  : 12/06/89
*   Type of file   : C Language File
*
*   Description
*   -----------
*   Partioning of the set of atomic concepts into similar clusters
*   using Ward's method.
*
*   Call Format
*   -----------
*	cluster <atom-docs>
*
*   where <atom-docs> is a file containing a list of atomic concepts
*   and the documents each occurs in. Each cluster and its members
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
*   Git repository home: <https://github.com/ghoss/MSc-Thesis>
*
*****************************************************************
* Date        :
* Description :
****************************************************************/   

#define PROG	"Cluster Partitioning (gh, 12/06/89)\n"
#define USAGE	"cluster <atom-docs> [QUIET]\n"

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

#define MAX_CLUSTERSIZE	30	/* Max. no. of concepts in a cluster */


/* A vector is represented by a list which contains all non-zero elements
   of the vector. Element type VECTOR_ELT */
typedef
  LIST VECTOR;

typedef
  struct C_NODE {
    int		key;		/* Sort key for cluster list */
    int		num;		/* Number of objects in tree at this node */
    VECTOR		centroid;	/* Non-zero centroid vector elements */
    struct C_NODE	*son_left;	/* Left and right descendants of node in */
    struct C_NODE	*son_right;	/* hierarchical cluster tree */
  } CLUSTER_NODE;

typedef
  struct {
    int		doc;	/* Index of document */
    double	freq;	/* Frequency of atomic concept in document */
  } VECTOR_ELT;


LIST
  cluster_list;		/* List of all clusters */
CLUSTER_NODE
  *top_node,		/* root node of cluster tree */
  *curr_cluster,
  *closest_a,		/* Used by 'build_tree' */
  *closest_b;
double
  closest_dist,		/* Used by 'build_tree' */
  glob_sum,		/* Used by 'calc_distance */
  glob_multiplier,	/* Used by 'calc_centroid */
  glob_div,
  glob_n1,
  glob_n2;
VECTOR
  glob_centroid;	/* Used by 'calc_centroid */
int
  serial_num;		/* Serial number for each cluster */


FILE
  *counter;	/* virtual file for display of running counts */
  

/****************************************************************
**  Compiler type declarations
****************************************************************/   

#ifndef BSDUNIX
void read_concepts ( FILE * );
double calc_distance ( CLUSTER_NODE *, CLUSTER_NODE * );
VECTOR calc_centroid ( CLUSTER_NODE *, CLUSTER_NODE * );
int comp_node ( ELEMENT, ELEMENT );
int comp_vector ( ELEMENT, ELEMENT );
CLUSTER_NODE *add_node ( int, VECTOR );
BOOL enum_primary ( ELEMENT );
BOOL enum_secondary ( ELEMENT );
BOOL enum_common ( ELEMENT, ELEMENT );
BOOL enum_noncommon ( ELEMENT );
BOOL enum_common1 ( ELEMENT, ELEMENT );
BOOL enum_noncommon1 ( ELEMENT );
void add_elt ( VECTOR, int, double );
void build_tree ( void );
void output_tree ( CLUSTER_NODE * );
void traverse_tree ( CLUSTER_NODE * );
int main ( int, char * [] );
#else
void read_concepts ();
double calc_distance ();
VECTOR calc_centroid ();
int comp_node ();
int comp_vector ();
CLUSTER_NODE *add_node ();
BOOL enum_primary ();
BOOL enum_secondary ();
BOOL enum_common ();
BOOL enum_noncommon ();
BOOL enum_common1 ();
BOOL enum_noncommon1 ();
void add_elt ();
void build_tree ();
void output_tree ();
void traverse_tree ();
int main ();
#endif


/****************************************************************
**  add_node
**
**  Adds a node to the cluster list.
**
**  IN  : key = sort key of this node. Conventions are negative
**              numbers for non-atomic clusters and positive
**              numbers for clusters containing one atomic concept.
**              In the latter case, the sort key should be equal
**              to the index of the atomic concept.
**        vector = centroid vector of the cluster 
**
**  OUT : The function returns a pointer to the new created
**        cluster node.
****************************************************************/   

int comp_node 
      ( a, b )
ELEMENT
  a;
ELEMENT
  b;
{
  return ( ((CLUSTER_NODE *) a) -> key - ((CLUSTER_NODE *) b) -> key );
}


CLUSTER_NODE *add_node
                ( key, vector )
int
  key;
VECTOR
  vector;
{
  CLUSTER_NODE
    *node = NULL;
    
  /* Create a cluster node for the specified atomic concept */
  if ( count_list ( vector ) > 0 ) {
    /* Only needs to be done if atomic concept occurs in a document */
    node = (CLUSTER_NODE *) malloc ( sizeof ( CLUSTER_NODE ) );
    assert ( node != NULL );
    node -> centroid = vector;
    node -> num = 1;	/* Default value for initial clusters, will be overwritten */
    node -> key = key;
    node -> son_left = node -> son_right = NULL;
          
    node = (CLUSTER_NODE *) add_list ( cluster_list, (ELEMENT) node, comp_node );
    assert ( node != NULL );
  }
  return ( node );
}


/****************************************************************
**  add_elt
**
**  Adds a new vector component to a vector.
**
**  IN  : vector = vector to which component should be added.
**        index  = component index
**        value  = component value
****************************************************************/   

int comp_vector
      ( a, b )
ELEMENT
  a;
ELEMENT
  b;
{
  return ( ((VECTOR_ELT *) a) -> doc - ((VECTOR_ELT *) b) -> doc );
}


void add_elt 
       ( vector, index, value )
VECTOR
  vector;
int
  index;
double
  value;
{
  VECTOR_ELT
    *elt;
  
  elt = (VECTOR_ELT *) malloc ( sizeof ( VECTOR_ELT ) );
  assert ( elt != NULL );
  
  elt -> doc = index;
  elt -> freq = value;
  
  elt = (VECTOR_ELT *) add_list ( vector, (ELEMENT) elt, comp_vector );
  assert ( elt != NULL );
}


/****************************************************************
**  read_concepts
**
**  Reads the file ATOM_DOCS and builds a list of clusters.
**  Each cluster contains exactly one element, namely an atomic
**  concept. The centroid vector of such a cluster is the
**  representation of the atomic concept in the document space.
**
**  IN  : Handle to open 'ATOM_DOCS' file.
****************************************************************/   

void read_concepts
       ( f )
FILE
  *f;
{
  VECTOR
    vector;
  int
    n, d, curr_atom;
  char
    line [ LINE_LENGTH ];

  /* Create empty cluster list */
  cluster_list = create_list ();
  assert ( cluster_list != NULL );
  
  curr_atom = -1;
  
  /* Ignore first line of ATOM_DOCS */
  fgets ( line, LINE_LENGTH, f );
  
  /* Read ATOM_DOCS line by line */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
    /* Read a number */
    n = sscanf ( line, " %d", &d );
    assert ( n == 1 );
    
    /* If number followed by a colon, then this is an atomic concept */
    if ( strchr ( line, ':' ) != NULL ) {
    
      if ( curr_atom != -1 ) {
        /* Create cluster node and add to cluster list */
        add_node ( curr_atom, vector );
      }
      
      /* Create empty vector */
      vector = create_list ();
      assert ( vector != NULL );
      curr_atom = d;
      
      /* Running count */
      fprintf ( counter, "%d\r", d );
    }
    else {
      /* number is a document index; add to centroid vector */
      add_elt ( vector, (int) d, 1.0 );
    }
  }
  
  /* Process last concept */
  add_node ( curr_atom, vector );
  fprintf ( counter, "\n" );
}


/****************************************************************
**  calc_distance
**
**  Calculates the Euclidean distance between two centroid vectors.
**  The distance is calculated as the sum of the squares of all
**  components not contained in (a AND b), plus the sum of the
**  squared differences of all components in (a AND b).
**
**  IN  : a, b = clusters with element type CLUSTER_NODE.
**               Both clusters must have valid centroids.
**
**  OUT : The function returns the distance as a floating-point
**        value.
****************************************************************/   

BOOL enum_noncommon
       ( e )
ELEMENT
  e;
{
  double
    t;
  
  t = ((VECTOR_ELT *) e) -> freq;
  glob_sum += t * t;
  return ( TRUE );
}


BOOL enum_common
       ( e1, e2 )
ELEMENT
  e1;
ELEMENT
  e2;
{
  VECTOR_ELT
    *c1, *c2;
  double
    t;
    
  c1 = (VECTOR_ELT *) e1;
  c2 = (VECTOR_ELT *) e2;
  
  /* Subtract vector components */
  t = c1 -> freq - c2 -> freq;
  glob_sum += t * t;
  
  return ( TRUE );
}


double calc_distance
         ( a, b )
CLUSTER_NODE
  *a;
CLUSTER_NODE
  *b;
{
  glob_sum = 0.0;
    
  /* Get symmetric set difference A - B */
  find_diff ( a -> centroid, b -> centroid, comp_vector, enum_noncommon );
  
  /* Get symmetric set difference B - A */
  find_diff ( b -> centroid, a -> centroid, comp_vector, enum_noncommon );
  
  /* Get union A AND B */
  find_union ( a -> centroid, b -> centroid, comp_vector, enum_common );
  
  /* Return weighted distance measure */
  return ( ( (double) a -> num * (double) b -> num * glob_sum ) / 
           (double) ( a -> num + b -> num ) );
}


/****************************************************************
**  calc_centroid
**
**  Calculates the centroid of two clusters. Vector calculations
**  are done in a similar way as 'enum_distance' does.
**
**  IN  : a, b = Clusters with type CLUSTER_NODE.
**
**  OUT : The function returns a new centroid vector of the
**        merged cluster. It does not merge the clusters
**        itself.
****************************************************************/   

BOOL enum_noncommon1
       ( e )
ELEMENT
  e;
{
  VECTOR_ELT
    *elt;
  
  elt = (VECTOR_ELT *) e;
  
  /* Create new vector element */
  add_elt ( glob_centroid, elt -> doc, elt -> freq * glob_multiplier );
  return ( TRUE );
}


BOOL enum_common1
       ( e1, e2 )
ELEMENT
  e1;
ELEMENT
  e2;
{
  VECTOR_ELT
    *elt1, *elt2;
  
  elt1 = (VECTOR_ELT *) e1;
  elt2 = (VECTOR_ELT *) e2;
    
  /* Create new vector element */
  add_elt ( glob_centroid, elt1 -> doc, 
            ( elt1 -> freq * glob_n1 + elt2 -> freq * glob_n2 ) / glob_div );

  return ( TRUE );
}


VECTOR calc_centroid
        ( a, b )
CLUSTER_NODE
  *a;
CLUSTER_NODE
  *b;
{
  /* Add vector elements to centroid which do not occur in both vectors */
  glob_centroid = create_list ();
  assert ( glob_centroid != NULL );
  
  glob_div = (double) ( a -> num + b -> num );
  glob_n1 = (double) a -> num;
  glob_n2 = (double) b -> num;
  
  /* Get symmetric set difference A - B */
  glob_multiplier = glob_n1 / glob_div;
  find_diff ( a -> centroid, b -> centroid, comp_vector, enum_noncommon1 );
  
  /* Get symmetric set difference B - A */
  glob_multiplier = glob_n2 / glob_div;
  find_diff ( b -> centroid, a -> centroid, comp_vector, enum_noncommon1 );
  
  /* Get union A AND B */
  find_union ( a -> centroid, b -> centroid, comp_vector, enum_common1 );

  return ( glob_centroid );
}


/****************************************************************
**  build_tree
**
**  Builds the hierarchical cluster tree by repeatedly comparing
**  closest clusters and merging them until no clusters remain.
****************************************************************/   

BOOL enum_secondary
       ( c )
ELEMENT
  c;
{
  CLUSTER_NODE
    *cluster;
  double
    d;
    
  cluster = (CLUSTER_NODE *) c;
  if ( cluster == curr_cluster ) {
    /* For any cluster i, only check clusters 1..i-1 - symmetrical matrix */
    return ( FALSE );
  }
  
  d = calc_distance ( curr_cluster, cluster );
  if ( d < closest_dist ) {
    /* A new record! Save winning clusters */
    closest_dist = d;
    closest_a = curr_cluster;
    closest_b = cluster;
  }
  
  fprintf ( counter, "%d %d\r", curr_cluster -> key, cluster -> key );
  return ( TRUE );
}


BOOL enum_primary
       ( c )
ELEMENT
  c;
{
  curr_cluster = (CLUSTER_NODE *) c;
  enum_list ( cluster_list, enum_secondary, ENUM_FORWARD );
  return ( TRUE );
}


void build_tree
       ( )
{
  CLUSTER_NODE
    *new_cluster;
  VECTOR
    centroid;
  int
    serial_key;
  
  serial_key = -1;
  
  while ( count_list ( cluster_list ) > 1 ) {
  
    closest_a = closest_b = NULL;
    closest_dist = 999999999.0;		/* infinity */
    
    enum_list ( cluster_list, enum_primary, ENUM_FORWARD );
    
    /* The two closest clusters are now in closest_a and closest_b */
    assert ( closest_a != NULL );
    assert ( closest_b != NULL );
    
    delete_list ( cluster_list, (ELEMENT) closest_a, comp_node );
    delete_list ( cluster_list, (ELEMENT) closest_b, comp_node );
    
    centroid = calc_centroid ( closest_a, closest_b );
    new_cluster = add_node ( serial_key, centroid );
    serial_key --;
    
    new_cluster -> son_left = closest_a;
    new_cluster -> son_right = closest_b;
    new_cluster -> num = ( closest_a -> num + closest_b -> num );
    top_node = new_cluster;
    
    /* Running count */
    fprintf ( counter, "%d   \r", count_list ( cluster_list ) );
  }
  fprintf ( counter, "\n" );
}


/****************************************************************
**  output_tree
**
**  Prints all concepts which are in the tree with the specified
**  root.
**
**  IN  : root = root node of the cluster tree.
****************************************************************/   

void output_tree
       ( root )
CLUSTER_NODE
  *root;
{
  if ( root -> son_left != NULL ) {
    /* Either both sons are NULL, or both are not NULL */
    assert ( root -> son_right != NULL );
    output_tree ( root -> son_left );
    output_tree ( root -> son_right );
  }
  else {
    /* Finally reached leaf */
    assert ( root -> son_right == NULL );
    printf ( "\t%d\n", root -> key );
  }
}


/****************************************************************
**  traverse_tree
**
**  Traverses the tree with specified root at node and seeks to 
**  make clusters with size < MAX_CLUSTERSIZE.
**
**  IN  : root = root node of the cluster tree.
****************************************************************/   

void traverse_tree
       ( root )
CLUSTER_NODE
  *root;
{
  if ( root -> num <= MAX_CLUSTERSIZE ) {
    /* Tree is small enough to be output */
    printf ( "%d :\n", serial_num );
    output_tree ( root );
    serial_num ++;
  }
  else {
    /* Too large, so try sons */
    traverse_tree ( root -> son_left );
    traverse_tree ( root -> son_right );
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
  
  /* Open atom-docs file */
  f = open_file ( argv [1] );
  
  /* Read concepts and build initial clusters */
  fprintf ( stderr, "Reading concepts.\n" );
  read_concepts ( f );
  
  /* Build hierarchical cluster tree */
  fprintf ( stderr, "Building cluster tree.\n" );
  build_tree ();
  
  /* Traverse cluster tree and create cluster file */
  fprintf ( stderr, "Generating clusters.\n" );
  serial_num = 0;
  traverse_tree ( top_node );
  
  return ( 0 );
}
