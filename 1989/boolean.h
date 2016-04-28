/****************************************************************
*
*           S O F T W A R E   S O U R C E   F I L E
*
*****************************************************************
*
*   Name of file   : boolean.h
*   Author         : Guido Hoss
*   Creation Date  : 26/04/89
*   Type of file   : C Header File
*
*   Description    : Definitions for emulation of data type
*                   'BOOLEAN'.
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

#define FALSE 0
#define TRUE  1

/* BOOL variables should only take values 0 or 1 */
typedef
  unsigned char BOOL;

