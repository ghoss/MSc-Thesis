/*
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
*/

#include <stdio.h>
#include <assert.h>

#define LINE_LENGTH	100


main 
  ( argc, argv )
int
  argc;
char
  *argv [];
{
  int
    a, o, ia, io,
    count, n, num, number;
  char
    line [ LINE_LENGTH ];
  FILE 
    *f;
    
  f = fopen ( argv [1], "r" );
  assert ( f != NULL );
  
  count = 30000;
  num = 0;
  
  while ( fgets ( line, LINE_LENGTH, f ) ) {
  
    n = sscanf ( line, " %d %d %d %d %d", &number, &a, &o, &ia, &io );
    assert ( n == 5 );
    
    if ( ia != count ) {
      printf ( "I-AGENT:  %s\n", line );
    }
    if ( io != count + 1 ) {
      printf ( "I-OBJECT:  %s\n", line );
    }
    if ( number != num ) {
      printf ( "LINE:  %s\n", line );
    }
    
    num ++;
    count += 2;
  }
    
  fclose ( f );
}
