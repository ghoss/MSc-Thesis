/****************************************************************
*
*           S O F T W A R E   S O U R C E   F I L E
*
*****************************************************************
*
*   Name of file   : list.c
*   Author         : Guido Hoss
*   Project        : ETH Diploma Thesis (SS 1989)
*   Creation Date  : 27/04/89
*   Type of file   : C Language File
*
*   Description
*   -----------
*   The abstract data type LIST is implemented as a sorted
*   dynamic array which is searched by a binary search algorithm.
*   As new elements are added, the list is extended automatically
*   by reallocating memory.
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

#include <stdio.h>
#ifdef MSDOS
#include <process.h>
#endif
#include <malloc.h>
#include <assert.h>

#include "boolean.h"


/****************************************************************
**  Forward declarations (compiler type checking)
****************************************************************/

#include "list.h"

#ifndef BSDUNIX
int binary_search ( ARRAY, int, ELEMENT, int (*) ( ELEMENT, ELEMENT ) );
ELEMENT add ( LIST, ELEMENT, int (*) ( ELEMENT, ELEMENT ), BOOL );
#else
int binary_search ();
ELEMENT add ();
#endif


/****************************************************************
**  create_list
**
**  Creates a new, empty list.
**
**  OUT : A handle to the new list, or NULL if the function did
**        not succeed.
****************************************************************/

LIST create_list
       ( )
{
  HEADER
    *header;

  /* Allocate header */
  header = (HEADER *) malloc ( sizeof ( HEADER ) );
  
  /* Allocate header and initialized contents for empty list */
  if ( header != NULL ) {
    header -> array = (ARRAY *) malloc ( sizeof ( ELEMENT ) );
    assert ( header -> array != NULL );
    header -> total = 1;
    header -> num = 0;
  }
  
  /* Return pointer to header */
  return ( header );
}


/****************************************************************
**  destroy_list
**
**  Deallocates all memory used by an existing list, thereby
**  destroying its contents. It must be possible to deallocate
**  the list elements with calls to free().
**
**  IN  : list = Pointer to handle of list to be destroyed.
**  OUT : '*list' is set to NULL.
****************************************************************/

void destroy_list
       ( list )
LIST
  *list;
{
  int
    i, max;
  ARRAY
    *arr;
    
  if ( *list == NULL ) return;

  /* Free all array elements */
  arr = (*list) -> array;
  max = (*list) -> num;
  for ( i = 0; i < max; i ++ ) {
    free ( (*arr) [i] );
  }
  
  /* Free header */
  free ( arr );
  free ( *list );
  *list = NULL;
}


/****************************************************************
**  binary_search
**
**  Does a binary search on a list.
**
**  IN  : 'base' = pointer to the first entry of the array.
**        'num'  = number of entries in the entry table.
**        'key'  = the element to be searched.
**        'comp' = the compare function (see add_list or
**                 lookup_list for a description).
**
**  OUT : If 'key' does NOT occur in the array, the 
**        index of the array element which must follow 'key'
**        if 'key' were inserted is returned. Otherwise, if
**        'key' was found, '-index - 1' (the negative index value
**        of the key minus one) is returned.
****************************************************************/   

int binary_search
      ( array, num, key, comp )
ARRAY
  array;
int
  num;
ELEMENT
  key;
#ifndef BSDUNIX
int
  (*comp) ( ELEMENT, ELEMENT );
#else
int
  (*comp) ();
#endif
{
  int
    compare,
    low, high, mid;

  low = 0;
  high = num - 1;
  
  while ( low <= high ) {
    mid = ( low + high ) / 2;
    compare = (*comp) ( key, array [ mid ] );

    if ( compare < 0 ) {
      high = mid - 1;
    }
    else if ( compare > 0 ) {
      low = mid + 1;
    }
    else {
      /* found at position 'mid' */
      return ( (-mid) - 1 );
    }
  }
  
  /* low = high + 1; insert between low and high, that is, in front
     of low */
  return ( low );
}


/****************************************************************
**  add
**
**  Function for add_list and lookup_list
**
**  IN  : 'list', 'elt', and 'comp' correspond to their 
**        counterparts in add_list and lookup_list.
**        enter = FALSE --> return NULL if element not found.
**                TRUE  --> add element if not found and return
**                          pointer to it.
**
**  OUT : See 'IN'.
****************************************************************/

ELEMENT add
          ( list, elt, comp, enter )
LIST
  list;
ELEMENT
  elt;
#ifndef BSDUNIX
int
  (*comp) ( ELEMENT, ELEMENT );
#else
int
  (*comp) ();
#endif
BOOL
  enter;
{
  ARRAY
    *arr;
  int
    newsize, i,
    n, tot, result;

  assert ( list != NULL );
  assert ( elt != NULL );

  /* It doesn't care if we call binary_search with a NULL array pointer */
  arr = list -> array;
  n = list -> num;
  tot = list -> total;
  result = binary_search ( *arr, n, elt, comp );
  
  if ( result < 0 ) {
    /* element occurs in list; binary_search returns negative index - 1 */
    if ( enter ) free ( elt );
    return ( (*arr) [ -result - 1 ] );
  }
  else {
    /* element is not in list */

    if ( enter ) {
      /* Test if still enough memory */
      if ( n >= tot ) {
        /* reallocate memory; create 10 new entries */
        newsize = tot + 10;
        arr = (ARRAY *) realloc ( arr, newsize * sizeof ( ELEMENT ) );
        if ( arr == NULL ) {
          return ( NULL );   /* no can do */
        }
        else {
          list -> total = newsize;
          list -> array = arr;   /* may be changed by realloc */
        }
      }

      /* Make a hole at entry arr[result] */
      for ( i = n; i > result; i -- ) {
        (*arr) [i] = (*arr) [ i - 1 ];
      }

      /* Fill in new entry */
      (*arr) [ result ] = elt;
      list -> num ++;
      return ( elt );
    }
    else {
      /* not in list, don't enter -> return NULL */
      return ( NULL );
    }
  }
}

         
/****************************************************************
**  add_list
**
**  Inserts an element in a list.
**
**  IN  : list = handle to list created with create_list.
**        elt  = pointer to new element. The pointer must have
**               been obtained by a call to malloc or calloc!
**        comp = compare function for 2 elements e1 and e2.
**               Must return < 0 if e1 < e2,
**                           = 0 if e1 = e2,
**                           > 0 if e1 > e2.
**
**  OUT : If the element already is in the list, then a pointer
**        is returned to the previously stored element. In this
**        case, 'elt' is automatically deallocated.
**        Otherwise, the element is inserted and 'elt' is returned.
**
**        If the function fails, NULL is returned and the element
**        is not inserted.
****************************************************************/

ELEMENT add_list
	  ( list, elt, comp )
LIST
  list;
ELEMENT
  elt;
#ifndef BSDUNIX
int
  (*comp) ( ELEMENT, ELEMENT );
#else
int
  (*comp) ();
#endif
{
  return ( add ( list, elt, comp, TRUE ) );
}


/****************************************************************
**  delete_list
**
**  Deletes an element from a list.
**
**  IN  : list = handle to list created with create_list.
**        elt  = pointer to an element. The element MUST be in
**               the list.
**               been obtained by a call to malloc or calloc!
**        comp = compare function for 2 elements e1 and e2.
**               Must return < 0 if e1 < e2,
**                           = 0 if e1 = e2,
**                           > 0 if e1 > e2.
**
**  OUT : The element is deleted from the list. Memory allocated
**        for the element is NOT freed.
****************************************************************/

void delete_list
       ( list, elt, comp )
LIST
  list;
ELEMENT
  elt;
#ifndef BSDUNIX
int
  (*comp) ( ELEMENT, ELEMENT );
#else
int
  (*comp) ();
#endif
{
  ARRAY
    *arr;
  int
    result,
    n, i;

  assert ( list != NULL );
  assert ( elt != NULL );

  /* It doesn't care if we call binary_search with a NULL array pointer */
  arr = list -> array;
  n = list -> num;
  result = binary_search ( *arr, n, elt, comp );
  assert ( result < 0 );
  
  /* binary_search returns negative index - 1 */
  for ( i = ( -result - 1 ); i < n; i ++ ) {
    (*arr) [i] = (*arr) [ i + 1 ];
  }
  
  /* Decrease amount of elements */
  list -> num --;
  
  /* Theoretically, list array memory should be freed at this point.
     Not implemented. */
}


/****************************************************************
**  lookup_list
**
**  Tests if a list contains a specified element.
**
**  IN  : list = handle to list created with create_list.
**        elt  = pointer to element to be searched.
**        comp = compare function for 2 elements e1 and e2.
**               Must return < 0 if e1 < e2,
**                           = 0 if e1 = e2,
**                           > 0 if e1 > e2.
**
**  OUT : If the element is in the list, a pointer to the stored
**        element (not to 'elt') is returned. Otherwise, the 
**        function returns NULL.
****************************************************************/

ELEMENT lookup_list
	  ( list, elt, comp )
LIST
  list;
ELEMENT
  elt;
#ifndef BSDUNIX
int
  (*comp) ( ELEMENT, ELEMENT );
#else
int
  (*comp) ();
#endif
{
  return ( add ( list, elt, comp, FALSE ) );
}


/****************************************************************
**  enum_list
**
**  Returns the contents of a list one by one.
**
**  IN  : list = handle to list created with create_list.
**        enum = function which is called for each list element.
**               If the function returns FALSE, enum_list aborts.
**        direction = either ENUM_FORWARD or ENUM_BACKWARD.
**
**  OUT : The function returns the result of the last call to
**        'enum'.
****************************************************************/

BOOL enum_list
       ( list, enum_proc, direction )
LIST
  list;
#ifndef BSDUNIX
BOOL
  (*enum_proc) ( ELEMENT );
#else
BOOL
  (*enum_proc) ();
#endif
int
  direction;
{
  int
    i, max;
  ARRAY
    *arr;
  BOOL
    result;
  
  assert ( ( direction == ENUM_FORWARD ) || ( direction == ENUM_BACKWARD ) );
  
  max = list -> num;
  arr = list -> array;
  result = TRUE;
  
  if ( direction == ENUM_FORWARD ) {
    for ( i = 0; i < max; i ++ ) {
      result = (*enum_proc) ( (*arr) [i] );
      if ( ! result ) break;
    }
  }
  else {
    for ( i = max - 1; i >= 0; i -- ) {
      result = (*enum_proc) ( (*arr) [i] );
      if ( ! result ) break;
    }
  }

  return ( result );
}


/****************************************************************
**  count_list
**
**  Returns the number of elements in a list.
**
**  IN  : list = handle to list created with create_list.
**
**  OUT : The function returns the number of elements in 'list'
**        or 0 if the list is empty.
****************************************************************/

int count_list
      ( list )
LIST
  list;
{
  if ( list == NULL ) {
    return ( 0 );
  }
  else {
    return ( list -> num );
  }
}


/****************************************************************
**  find_union
**
**  Calls a user-specified procedure for each element that occurs
**  simultaneously in two lists.
**
**  IN  : list1, list2 = handles to two lists with elements of
**                       the same type.
**
**        compare = a function which accepts pointers to two
**                  elements e1, e2 and returns -1 if e1 "<" e2,
**                  0 if e1 "=" e2, and +1 if e1 ">" e2.
**
**        call = a function to be called with e1 and e2 if
**               e1 "=" e2. If the function returns FALSE,
**               find_union returns immediately.
**               
**  OUT : The function returns the result of the last call to
**        "call".
****************************************************************/

BOOL find_union
       ( list1, list2, compare, call )
LIST
  list1;
LIST
  list2;
#ifndef BSDUNIX
int 
  (*compare) ( ELEMENT, ELEMENT );
BOOL 
  (*call) ( ELEMENT, ELEMENT );
#else
int 
  (*compare) ();
BOOL 
  (*call) ();
#endif
{
  int
    max1, max2,
    i, j, res;
  ARRAY
    *arr1, *arr2;
  ELEMENT
    e1, e2;
  BOOL
    ok;

  assert ( list1 != NULL );
  assert ( list2 != NULL );
  
  max1 = list1 -> num;
  arr1 = list1 -> array;
  max2 = list2 -> num;
  arr2 = list2 -> array;
  
  i = j = 0;
  ok = TRUE;
  while ( ( i < max1 ) && ( j < max2 ) && ok ) {
  
    /* Remember that lists are sorted in increasing order */
    e1 = (*arr1) [i];
    e2 = (*arr2) [j];
    res = (*compare) ( e1, e2 );
    
    if ( res < 0 ) {
      /* e1 is smaller than e2 */
      i ++;
    }
    else if ( res > 0 ) {
      /* e2 is smaller than e1 */
      j ++;
    }
    else {
      /* both are equal */
      ok = (*call) ( e1, e2 );
      i ++;
      j ++;
    }
  }
  return ( ok );
}


/****************************************************************
**  find_diff
**
**  Computes the symmetric set difference for two lists; that
**  is, calls a user-specified procedure for each element that
**  occurs in the first list but not in the second.
**
**  IN  : list1, list2 = handles to two lists with elements of
**                       the same type.
**
**        compare = a function which accepts pointers to two
**                  elements e1, e2 and returns -1 if e1 "<" e2,
**                  0 if e1 "=" e2, and +1 if e1 ">" e2.
**
**        call = a function to be called with e if e in list1 and
**               not e in list2. If the function returns FALSE,
**               find_union returns immediately.
**               
**  OUT : The function returns the result of the last call to
**        "call".
****************************************************************/

BOOL find_diff
       ( list1, list2, compare, call )
LIST
  list1;
LIST
  list2;
#ifndef BSDUNIX
int 
  (*compare) ( ELEMENT, ELEMENT );
BOOL 
  (*call) ( ELEMENT );
#else
int 
  (*compare) ();
BOOL 
  (*call) ();
#endif
{
  int
    i1, j1,
    max1, max2,
    i, j, res;
  ARRAY
    *arr1, *arr2;
  ELEMENT
    e1, e2;
  BOOL
    ok;

  assert ( list1 != NULL );
  assert ( list2 != NULL );
  
  max1 = list1 -> num;
  arr1 = list1 -> array;
  max2 = list2 -> num;
  arr2 = list2 -> array;
  
  i = j = 0;
  ok = TRUE;
  while ( ( ( i < max1 ) && ( j < max2 ) ) && ok ) {
  
    /* Remember that lists are sorted in increasing order */
    assert ( i < max1 );
    assert ( j < max2 );
    e1 = (*arr1) [i];
    e2 = (*arr2) [j];
    res = (*compare) ( e1, e2 );
    
    if ( res < 0 ) {
      /* e1 is smaller than e2, output e1 */
      ok = (*call) ( e1 );
      i ++;
    }
    else if ( res > 0 ) {
      /* e2 is smaller than e1 */
      j ++;
    }
    else {
      /* both are equal */
      i ++;
      j ++;
    }
  }

  /* Dump remainders of list1 */
  for ( i1 = i; ( i1 < max1 ) && ok; i1 ++ ) {
    assert ( i1 < max1 );
    ok = (*call) ( (*arr1) [i1] );
  }
  return ( ok );
}


/****************************************************************
**  merge_lists
**
**  Calls a user-specified procedure for each element that occurs
**  in both lists.
**
**  IN  : list1, list2 = handles to two lists with elements of
**                       the same type.
**
**        compare = a function which accepts pointers to two
**                  elements e1, e2 and returns -1 if e1 "<" e2,
**                  0 if e1 "=" e2, and +1 if e1 ">" e2.
**
**        call = a function to be called with an element of either
**               list. If the function returns FALSE,
**               find_union returns immediately.
**               
**  OUT : The function returns the result of the last call to
**        "call".
****************************************************************/

BOOL merge_lists
       ( list1, list2, call, compare )
LIST
  list1;
LIST
  list2;
#ifndef BSDUNIX
BOOL 
  (*call) ( ELEMENT );
int 
  (*compare) ( ELEMENT, ELEMENT );
#else
BOOL 
  (*call) ();
int 
  (*compare) ();
#endif
{
  int
    i1, j1,
    max1, max2,
    i, j, res;
  ARRAY
    *arr1, *arr2;
  ELEMENT
    e1, e2;
  BOOL
    ok;

  assert ( list1 != NULL );
  assert ( list2 != NULL );
  
  max1 = list1 -> num;
  arr1 = list1 -> array;
  max2 = list2 -> num;
  arr2 = list2 -> array;
  
  i = j = 0;
  ok = TRUE;
  while ( ( ( i < max1 ) && ( j < max2 ) ) && ok ) {
  
    /* Remember that lists are sorted in increasing order */
    assert ( i < max1 );
    assert ( j < max2 );
    e1 = (*arr1) [i];
    e2 = (*arr2) [j];
    res = (*compare) ( e1, e2 );
    
    if ( res < 0 ) {
      /* e1 is smaller than e2 */
      ok = (*call) ( e1 );
      i ++;
    }
    else if ( res > 0 ) {
      /* e2 is smaller than e1 */
      ok = (*call) ( e2 );
      j ++;
    }
    else {
      /* both are equal */
      ok = (*call) ( e1 );
      i ++;
      j ++;
    }
  }

  /* Dump remainders of one list */
  for ( i1 = i; ( i1 < max1 ) && ok; i1 ++ ) {
    assert ( i1 < max1 );
    ok = (*call) ( (*arr1) [i1] );
  }
  for ( j1 = j; ( j1 < max2 ) && ok; j1 ++ ) {
    assert ( j1 < max2 );
    ok = (*call) ( (*arr2) [j1] );
  }
  return ( ok );
}
