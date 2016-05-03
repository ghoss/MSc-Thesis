/****************************************************************
*
*           S O F T W A R E   S O U R C E   F I L E
*
*****************************************************************
*
*   Name of file   : list.h
*   Author         : Guido Hoss
*   Project        : ETH Diploma Thesis (SS 1989)
*   Creation Date  : 27/04/89
*   Type of file   : C Header File
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

/* A list element is a pointer to a user-defined entity, such as a
   string, a structure etc. */
typedef
  char *ELEMENT;

/* Lists are arrays of list elements, sorted in a user-defined order.
   An ordering function must be specified when adding or searching an
   element in a list. */
typedef
  ELEMENT ARRAY [];

/* A list is identified by a list header which contains a pointer to
   the associated array and the number of elements contained in it. */
typedef
  struct {
    ARRAY *array;  /* Pointer to base of array */
    int   num;     /* Number of array entries used */
    int   total;   /* Number of total array entries */
  } HEADER;

typedef
  HEADER *LIST;


/* Direction constants for 'enum_list' */
#define ENUM_FORWARD	1
#define ENUM_BACKWARD	-1


/* Functions defined on lists */
#ifndef BSDUNIX
LIST create_list ( void );
void destroy_list ( LIST * );
ELEMENT add_list ( LIST, ELEMENT, int (*) ( ELEMENT, ELEMENT ) );
void delete_list ( LIST, ELEMENT, int (*) ( ELEMENT, ELEMENT ) );
ELEMENT lookup_list ( LIST, ELEMENT, int (*) ( ELEMENT, ELEMENT ) );
BOOL enum_list ( LIST, BOOL (*) ( ELEMENT ), int );
int count_list ( LIST );
BOOL find_union ( LIST, LIST, int (*) ( ELEMENT, ELEMENT ), 
                  BOOL (*) ( ELEMENT, ELEMENT ) );
BOOL merge_lists ( LIST, LIST, BOOL (*) ( ELEMENT ), int (*) ( ELEMENT, ELEMENT ) );
BOOL find_diff ( LIST, LIST, int (*) ( ELEMENT, ELEMENT ), 
                 BOOL (*) ( ELEMENT ) );
#else
LIST create_list ();
void destroy_list ();
void delete_list ();
ELEMENT add_list ();
ELEMENT lookup_list ();
BOOL enum_list ();
int count_list ();
BOOL find_union ();
BOOL merge_lists ();
BOOL find_diff ();
#endif
