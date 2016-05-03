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

main () {
  int i;
  for ( i = 30000; i < 30100; i ++ ) {
    if ( (i/2)*2==i ) printf ("%d\tIA_%d\n", i, (i-30000)/2 );
    else printf ("%d\tIO_%d\n", i, (i-1-30000)/2 );
  }
}
