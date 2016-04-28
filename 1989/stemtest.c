/****************************************************************
*
*           S O F T W A R E   S O U R C E   F I L E
*
*****************************************************************
*
*   Name of file   : stemtest.c
*   Author         : Guido Hoss
*   Project        : ETH Diploma Thesis (SS 1989)
*   Creation Date  : 03/06/89
*   Type of file   : C Language File
*
*   Description
*   -----------
*   Allows verification of the stemming algorithm.
*
*   Call Format
*   -----------
*	stemtest [word]
*
*   If no word is specified on the command line, the program
*   prompts with 'word?'. The user may stop the program by
*   pressing Return.
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
#include <string.h>
#include <ctype.h>
#include "boolean.h"
#include "util.h"
#include "wordstem.h"

#ifndef BSDUNIX
void main ( int, char *[] );
#endif

#define MAX_WORDLEN	50


char *uppercase 
        ( w )
char
  *w;
{
  char
    *ch;

  for ( ch = w; *ch != '\0'; ch ++ ) {
    if ( islower ( *ch ) ) {
      *ch = toupper ( *ch );
    }
  }
  return ( w );
}


void main
       ( argc, argv )
int 
  argc;
char
  *argv [];
{
  char
    word [ MAX_WORDLEN ];

  if ( argc == 1 ) {
    /* endless loop */
    while ( TRUE ) {
      printf ( "word? [Return = end] " );
      gets ( word );
      if ( strlen ( word ) == 0 ) {
	break;
      }
      else {
	StemEnglishWord ( uppercase ( word ) );
	printf ( "--> %s\n\n", word );
      }
    }
  }
  else {
    /* command line mode */
    strcpy ( word, argv [1] );
    StemEnglishWord ( uppercase ( word ) );
    printf ( "%s\n", word );
  }
}

