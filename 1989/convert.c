/****************************************************************
*
*           S O F T W A R E   S O U R C E   F I L E
*
*****************************************************************
*
*   Name of file   : convert.c
*   Author         : Guido Hoss
*   Project        : ETH Diploma Thesis (SS 1989)
*   Creation Date  : 12/05/89
*   Type of file   : C Language File
*
*   Description
*   -----------
*   This utility program converts the 'qrels.text' file for the
*   cacm/cisi collections into the format of the RELEVANT file.
*
*   Call Format
*   -----------
*	convert <qrels>
*
*   The converted file is written to the standard output.
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

#define PROG	"'Relevant documents' file converter (gh, 12/05/89)\n"
#define USAGE	"Usage: convert <qrels>\n"

#include <stdio.h>
#ifdef MSDOS
#include <process.h>
#endif
#include <assert.h>

#include "util.h"


#define LINE_LENGTH	100


/****************************************************************
**  Forward declarations (compiler type checking)
****************************************************************/ 

#ifndef BSDUNIX
int main ( int, char * [] );
#else
int main ();
#endif

  
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
  int
    res, 
    query, doc,
    curr_query;
  char
    line [ LINE_LENGTH ];
  FILE
    *f;
    
  /* Program title */
  fprintf ( stderr, PROG );
  if ( argc != 2 ) {
    fprintf ( stderr, USAGE );
    return ( 1 );
  }
  
  /* Open input file */
  f = open_file ( argv [1] );
  curr_query = -1;
  
  /* Read one line at a time */
  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    /* Original line must contain at least two numbers */
    res = sscanf ( line, " %d %d", &query, &doc );
    assert ( res == 2 );
    
    /* Don't print query number on every line */
    if ( query != curr_query ) {
      printf ( "%d :\n", query );
      curr_query = query;
      fprintf ( stderr, "%d\r", query );
    }
    printf ( "\t%d\n", doc );
  }
  
  fclose ( f );
  fprintf ( stderr, "\n" );
  return ( 0 );
}
