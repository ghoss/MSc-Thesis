/****************************************************************
*
*           S O F T W A R E   S O U R C E   F I L E
*
*****************************************************************
*
*   Name of file   : util.c
*   Author         : Guido Hoss
*   Project        : ETH Diploma Thesis (SS 1989)
*   Creation Date  : 29/04/89
*   Type of file   : C Language File
*
*   Description
*   -----------
*   Various C routines called by several programs.
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

#include <stdio.h>
#ifdef MSDOS
#include <process.h>
#endif
#include <assert.h>
#include <string.h>
#include <malloc.h>

#include "util.h"


/****************************************************************
**  duplicate
**
**  Creates a copy of a null-terminated character string.
**
**  IN  : s = string to be duplicated.
**
**  OUT : Function returns a pointer to a copy of s.
****************************************************************/

char *duplicate
        ( s )
char
  *s;
{
  char
    *s1;

  s1 = (char *) malloc ( strlen ( s ) + 1 );
  assert ( s1 != NULL );

  strcpy ( s1, s );
  return ( s1 );
}


/****************************************************************
**  open_file
**
**  Opens a file read-only, prints an error message if file
**  could not be opened, and aborts the program.
**
**  IN  : s = pointer to null-terminated file name.
**
**  OUT : Function returns a handle to the opened file. If not
**        successful, then function calls exit(1).
****************************************************************/

FILE *open_file
       ( s )
char
  *s;
{
  FILE
    *f;

  f = fopen ( s, "r" );
  if ( f == NULL ) {
    /* Could not open -> print error message and exit */
    perror ( s );
    exit ( 1 );
  }
  return ( f );
}
