/****************************************************************
*
*           S O F T W A R E   S O U R C E   F I L E
*
*****************************************************************
*
*   Name of file   : parse_itemcoll.c
*   Author         : Guido Hoss
*   Project        : ETH Diploma Thesis (SS 1989)
*   Creation Date  : 27/04/89
*   Type of file   : C Language File
*
*   Description
*   -----------
*   Creates a list of all terms per document from a collection.
*   Only information following lines beginning by .I, .T and .W
*   is extracted from the collection. The frequency of each term
*   written to the standard output. Test queries are treated as
*   part of the document collection although they are read from
*   a separate input file. For convenience, negative document
*   numbers are associated with queries.
*
*   Call Format
*   -----------
*
*      parse_itemcoll <docfile> <queryfile> <stoplist>
*
*   <docfile>   = Path name of document collection.
*   <queryfile> = Path name of file containing test queries.
*                 (Must have the same format as <docfile>)
*   <stoplist>  = Path name of stop list to be used.
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

#define PROG	"Document Collection Parser (gh, 29/04/89)\n"
#define USAGE	"Usage: parse_itemcoll <docfile> <queryfile> <stoplist> [QUIET]\n"

#include <ctype.h>
#include <stdio.h>
#ifdef MSDOS
#include <process.h>
#endif
#include <assert.h>
#include <malloc.h>
#include <string.h>

#include "boolean.h"
#include "list.h"
#include "util.h"
#include "wordstem.h"

#define MAX_WORDLEN   100    /* Max. length of a single or compound word */
#define LINE_LENGTH   100    /* Max. length of a line in the document file */


LIST
  doc_words = NULL,   /* list of words in document */
  stop_words;         /* list of stop words */

/* The list 'doc_words' contains elements of type TERMSTRUCT. This
   allows us to record the term frequency along with the terms. */
typedef
  struct {
    int freq;     /* frequency of term */
    char *term;   /* term spelling */
  } TERMSTRUCT;

FILE
  *counter;	/* Virtual file used to output running counts */


/****************************************************************
**  Forward declarations (compiler type checking)
****************************************************************/

#ifndef BSDUNIX
int comp_term ( ELEMENT, ELEMENT );
BOOL enum_proc ( ELEMENT );
void dump_list ( int );
void addword ( char * );
BOOL getword ( char *, char * );
BOOL stopword ( char * );
void analyze ( FILE *, BOOL );
int comp_str ( ELEMENT, ELEMENT );
void load_stoplist ( FILE * );
int main ( int, char *[] );
char *uppercase ( char * );
#else
int comp_term ();
BOOL enum_proc ();
void dump_list ();
void addword ();
BOOL getword ();
BOOL stopword ();
void analyze ();
int comp_str ();
void load_stoplist ();
int main ();
char *uppercase ();
#endif


/****************************************************************
**  uppercase
**
**  Converts a string to upper case.
**
**  IN  : s = pointer to string to be converted.
**
**  OUT : Function returns s.
****************************************************************/

char *uppercase
        ( s )
char
  *s;
{
  int
    i, len;
  char
    *s1;

  s1 = s;
  len = strlen ( s );

  for ( i = 0; i < len; i ++, s ++ ) {
    if ( islower ( *s ) ) {
      *s = toupper ( *s );
    }
  }
  return ( s1 );
}


/****************************************************************
**  comp_term
**
**  This procedure establishes an ordering of TERMSTRUCT records
**  by comparing the lexicographical spelling of the associated
**  terms. Used whenever a term needs to be added to 'doc_words'.
**
**  IN  : t1, t2 = pointers to TERMSTRUCT's to be compared.
**
**  OUT : -1 if t1 < t2, 0 if t1 = t2, +1 if t1 > t2.
****************************************************************/

int comp_term
      ( t1, t2 )
ELEMENT
  t1;
ELEMENT
  t2;
{
  return ( strcmp ( ((TERMSTRUCT *) t1) -> term, 
		    ((TERMSTRUCT *) t2) -> term ) );
}


/****************************************************************
**  dump_list
**
**  Writes a frequency table of the current document to the
**  standard output.
**
**  IN  : docnum = index of current document.
**        The contents of global list 'doc_words' are printed.
**        If doc_words is empty, then the function returns
**        immediately.
**
**  OUT : Global list 'doc_words' is destroyed.
****************************************************************/

BOOL enum_proc
       ( elt )
ELEMENT
  elt;
{
  char
    *t;

  /* Print frequency tf(d,s) and alphanumeric term a(s) */
  t = ((TERMSTRUCT *) elt ) -> term;
  printf ( "\t%d\t%s\n", ((TERMSTRUCT *) elt) -> freq, t );

  /* Destroy term buffer (structure is freed by 'destroy_list') */
  free ( t );
  return ( TRUE );
}


void dump_list
       ( docnum )
int
  docnum;
{
  if ( doc_words == NULL ) return;

  if ( count_list ( doc_words ) > 0 ) {
    /* Standard version: Ignore empty documents */
    printf ( "%d\n", docnum );
    enum_list ( doc_words, enum_proc, ENUM_FORWARD );
  }
  destroy_list ( &doc_words );
}


/****************************************************************
**  addword
**
**  Adds a word to the term list of the current document.
**
**  IN  : word = pointer to null-terminated word to be added.
**               A duplicate of the word is generated by this
**               procedure.
****************************************************************/

void addword
       ( word )
char
  *word;
{
  TERMSTRUCT
    *elt,
    *res;
    
  /* Build initial term structure and add to list */
  elt = (TERMSTRUCT *) malloc ( sizeof ( TERMSTRUCT ) );
  assert ( elt != NULL );
  elt -> freq = 0;
  elt -> term = duplicate ( word );
  res = (TERMSTRUCT *) add_list ( doc_words, (ELEMENT) elt, comp_term );
  
  /* if res == elt, then term was not in the list before call to add_list;
     in that case, frequency is 0 (as initialized above). In any case,
     increment term frequency by 1. */
  assert ( res != NULL );
  res -> freq ++;
}


/****************************************************************
**  getword
**
**  Gets the next word from an input line and converts it to
**  uppercase. Subsequent calls to 'getword' with the same line
**  will yield the words in the line one by one.
**
**  IN  : buf  = pointer to input line.
**        word = pointer to buffer large enough to hold the next
**               word.
**
**  OUT : The contents of 'buf' are modified.
**        'word' contains the next word found. If no word could
**        be found, the function returns FALSE and 'word' is
**        empty. Otherwise, the function returns TRUE.
****************************************************************/

/* A word consists of letters, which are either uppercase characters
   or underscores */

#define isletter(c)	( isalpha ( c ) || ( (c) == '_' ) )


BOOL getword
       ( buf, word )
char
  *buf;
char
  *word;
{
  char
    *w,
    *start, 
    *end;
    
  /* Scan for first letter of word */
  start = buf;
  while ( ( *start != '\0' ) && ( ! isletter ( *start ) ) ) {
    start ++;
  }
  
  if ( *start != '\0' ) {
    /* Search for end of word */
    end = start;
    w = word;
    while ( ( *end != '\0' ) && ( isletter ( *end ) ) ) {
      *w = *end;
      w ++;
      end ++;
    }
    
    /* Delete everything up to end of word from 'buf' */
    strcpy ( buf, end );
    
    /* Null-terminate word and convert to uppercase */
    *w = '\0';
    uppercase ( word );
    return ( TRUE );
  }
  
  else {
    /* could not find any word */
    *word = '\0';
    return ( FALSE );
  }
}


/****************************************************************
**  stopword
**
**  Tests if a given word is in the stop list.
**
**  IN  : word = pointer to null-terminated word to be checked.
**
**  OUT : The function returns TRUE if the word is in the
**        stoplist.
****************************************************************/

BOOL stopword
       ( word )
char
  *word;
{
  /* Check list stop_words using comp_term */
  return ( lookup_list ( stop_words, (ELEMENT) word, comp_str ) != NULL );
}


/****************************************************************
**  analyze
**
**  Reads a text collection, searching for text after .T and .W
**  lines. Document numbers are taken from .I lines.
**  Builds compound terms of max. 2 words if words are not
**  separated by stop words. Stop list must have been loaded
**  previously. Document list must have been created.
**
**  IN  : f     = file handle of opened document file.
**        query = TRUE  --> This is a query file;
**                FALSE --> This is a document file.
**                (Distinction important because negative
**                document numbers are assigned to queries)
****************************************************************/

#define getline()   fgets ( line, LINE_LENGTH, f );

void analyze
       ( f, query )
FILE
  *f;
BOOL
  query;
{
  int
    res,
    docnum;
  char
    *ok,
    line [ LINE_LENGTH ],
    prev_word [ MAX_WORDLEN ],
    curr_word [ MAX_WORDLEN ],
    new_word [ MAX_WORDLEN ],
    cmd;

  /* Read document file line by line */
  ok = getline ();
  while ( ok ) {

    if ( line [0] == '.' ) {
      /* Maybe .I, .T, or .W */
      cmd = line [1];

      if ( cmd == 'I' ) {
        /* This is a document id; dump frequency list of previous document */
        dump_list ( docnum );
        doc_words = create_list ();
        assert ( doc_words != NULL );

        res = sscanf ( line, ".I %d", &docnum );
        assert ( res == 1 );

        /* docnum contains number of current document */
        if ( query ) {
          /* Assign negative numbers to queries */
          docnum = - docnum;
        }

        /* update running count */
        fprintf ( counter, "%d\r", docnum );

        /* Read next line */
        ok = getline ();
      }

      else if ( ( cmd == 'T' ) || ( cmd == 'W' ) || ( cmd == 'A' ) ) {
        /* Read text up to next command */
        ok = getline ();
        prev_word [0] = '\0';
        
        while ( ok && ( line [0] != '.' ) ) {
          
          /* Extract words from current line */
          while ( getword ( line, curr_word ) ) {
          
            /* Ignore if this is a stop word or shorter than 3 chars */
            if ( stopword ( curr_word ) ) {
              prev_word [0] = '\0';
              continue;
            }
	    else if ( strlen ( curr_word ) < 3 ) {
	      /* example: OUR COMPUTER'S MEMORY --> S becomes a word;
		 however, we still want to be able to build multiword
		 terms */
              continue;
            }

	    /* Perform word stemming */
	    StemEnglishWord ( curr_word );
            
            /* Increase term frequency of current word */
            addword ( curr_word );
            if ( prev_word [0] != '\0' ) {
              /* Build multiword term <prev_word, curr_word> */
              strcpy ( new_word, prev_word );
              strcat ( new_word, " " );
              strcat ( new_word, curr_word );
              addword ( new_word );
            }
            
            /* Current word becomes previous word */
            strcpy ( prev_word, curr_word ); 
          }
          ok = getline ();
        }
      }

      else {
        /* some other command; ignore it */
        ok = getline ();
      }
    }
    else {
      /* some other line */
      ok = getline ();
    }
  }
  dump_list ( docnum );
  fprintf ( counter, "\n" );
}


/****************************************************************
**  load_stoplist
**
**  Loads the stoplist into a memory list for faster subsequent
**  access.
**
**  IN  : f = file handle of opened stop list.
****************************************************************/

int comp_str
      ( s1, s2 )
ELEMENT
  s1;
ELEMENT
  s2;
{
  return ( strcmp ( (char *) s1, (char *) s2 ) );
}


void load_stoplist
       ( f )
FILE
  *f;
{
  int
    count = 0;
  ELEMENT
    res;
  char
    *w, word [ MAX_WORDLEN ];

  stop_words = create_list ();

  /* Read stop words, one at a time */
  while ( fscanf ( f, " %s", word ) > 0 ) {

    /* Since the list contains pointers to the strings, we must make a copy */
    w = uppercase ( duplicate ( word ) );
    res = add_list ( stop_words, (ELEMENT) w, comp_str );
    assert ( res != NULL );

    /* Show running count */
    count ++;
    fprintf ( counter, "%d\r", count );
  }
  fprintf ( counter, "\n" );
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
    *docfile,
    *stoplist,
    *queryfile;

  /* Program title */
  fprintf ( stderr, PROG );

  /* Get verbose or quiet mode */
  if ( ( argc == 5 ) && ( *argv [4] == 'Q' ) ) {
    /* in case of quiet mode: redirect running counts to /dev/null */
    counter = fopen ( "/dev/null", "r" );
    assert ( counter != NULL );
  }
  else {
    /* verbose mode: redirect running counts to stderr */
    counter = stderr;
  }

  if ( argc < 4 ) {
    fprintf ( stderr, USAGE );
    return ( 1 );
  }

  /* Open document file, query file, and stop list */
  
  docfile = open_file ( argv [1] );
  queryfile = open_file ( argv [2] );
  stoplist = open_file ( argv [3] );

  /* Load the entire stop list into memory */
  fprintf ( stderr, "Loading stop list.\n" );
  load_stoplist ( stoplist );

  /* Read the document collection */
  fprintf ( stderr, "Reading documents.\n" );
  analyze ( docfile, FALSE );

  /* Read test queries */
  fprintf ( stderr, "Reading queries.\n" );
  analyze ( queryfile, TRUE );

  fclose ( docfile );
  fclose ( queryfile );
  fclose ( stoplist );
  destroy_list ( &stop_words );

  return ( 0 );
}
