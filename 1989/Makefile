#################################################################
#
#           S O F T W A R E   S O U R C E   F I L E
#
#################################################################
#
#   Name of file   : Makefile (sources) 
#   Author         : Guido Hoss
#   Project        : ETH Diploma Thesis (SS 1989)
#   Creation Date  : 29/04/89
#   Type of file   : UNIX 'make' File
#
#   Call Format
#   -----------
#
#	make
#	make parse_itemcoll
#	make generate_ida
#	make calc_docdescr
#	make build_concepts
#	make calc_atomdocs
#	make init_atomwgts
#	make calc_rsv
#	make eval_prefs
#	make optimize
#	make calc_pr
#	make select
#	make cluster
#	make stemtest
#       make termfreq
#	make termdisc
#
#################################################################
#
#   COPYRIGHT (C) 1989, 2016 BY GUIDO HOSS.
#
#   This program is free software: you can redistribute it and/or 
#   modify it under the terms of the GNU General Public License
#   as published by the Free Software Foundation, either version 3
#   of the License, or (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public
#   License along with this program.  If not, see
#   <http://www.gnu.org/licenses/>.
#
#   Git repository home: <https://github.com/ghoss/Thesis>
#
#################################################################
# Date        :
# Description :
#################################################################

#
#  These programs have been compiled on several systems:
#
#  MSDOS   : Microsoft C 5.0 compiler, full type checking
#  XENIX   : XENIX System V Unix cc, limited type checking
#  BSDUNIX : SUN cc, no type checking
#

#
#  Command file names
#

#CC = cc -AL -c
#LD = cc
CC = cc -O -DP50 -DBSDUNIX -c
LD = cc

#  Default make action
all :	parse_itemcoll  generate_ida  calc_docdescr \
	build_concepts  calc_atomdocs init_atomwgts \
	optimize	calc_rsv      eval_prefs \
	calc_pr		convert	      stemtest \
	cluster	select	termfreq      termdisc

#
#  Utility functions
#
util.o :	util.c util.h
	$(CC) util.c

#
#  Binary list manager (object module used by other programs)
#
list.o :	list.c list.h
	$(CC) list.c


#
#  Precedence selection
#
select.o :	select.c util.h list.h
	$(CC) select.c

select :	select.o util.o list.o
	$(LD) select.o util.o list.o -o select


#
#  Stemming test program
#
stemtest.o :	stemtest.c wordstem.h
	$(CC) stemtest.c

stemtest :	stemtest.o wordstem.o
	$(LD) stemtest.o wordstem.o -o stemtest

wordstem.o :	wordstem.c
	$(CC) wordstem.c


#
#  Cluster partitioning
#
cluster.o :	cluster.c list.h util.h
	$(CC) cluster.c

cluster :	cluster.o list.o util.o
	$(LD) cluster.o list.o util.o -o cluster


#
#  Document collection parser
#
parse.o :	parse.c wordstem.h util.h list.h
	$(CC) parse.c

parse_itemcoll :	parse.o wordstem.o list.o util.o
	$(LD) parse.o wordstem.o list.o util.o -o parse_itemcoll


#
#  Initial domain algebra generator
#
generate.o :	generate.c util.h list.h
	$(CC) generate.c

generate_ida :	generate.o util.o list.o
	$(LD) generate.o list.o util.o -o generate_ida


#
#  Calculate sign weights
#
calcswgt.o :	calcswgt.c util.h list.h
	$(CC) calcswgt.c

calc_docdescr :	calcswgt.o list.o util.o
	$(LD) calcswgt.o list.o util.o -lm -o calc_docdescr

#
#  Concept space generation
#
concepts.o :	concepts.c util.h list.h
	$(CC) concepts.c

build_concepts :	concepts.o util.o list.o
	$(LD) concepts.o util.o list.o -o build_concepts

#
#  Atomic concepts <-> document list calculation
#
atomdocs.o :	atomdocs.c util.h list.h
	$(CC) atomdocs.c

calc_atomdocs :	atomdocs.o util.o list.o
	$(LD) atomdocs.o util.o list.o -o calc_atomdocs

#
#  Initialization of atomic weights
#
initatom.o :	initatom.c util.h
	$(CC) initatom.c

init_atomwgts :	initatom.o util.o
	$(LD) initatom.o util.o -lm -o init_atomwgts

#
#  Simplex optimization
#
simplex.o :	simplex.c util.h list.h limits.h
	$(CC) simplex.c

optimize :	simplex.o util.o list.o
	$(LD) simplex.o util.o list.o -lm -o optimize

#
#  Calculation of RSV values
#
calc_rsv.o :	calc_rsv.c util.h list.h
	$(CC) calc_rsv.c

calc_rsv :	calc_rsv.o util.o list.o
	$(LD) calc_rsv.o util.o list.o -lm -o calc_rsv

#
#  Evaluate results of RSV calculation and relevance assessments
#
evalpref.o :	evalpref.c util.h list.h limits.h
	$(CC) evalpref.c

eval_prefs :	evalpref.o util.o list.o
	$(LD) evalpref.o util.o list.o -lm -o eval_prefs

#
#  Recall/precision calculation
#
calc_pr.o :	calc_pr.c util.h list.h
	$(CC) calc_pr.c

calc_pr :	calc_pr.o util.o list.o
	$(LD) calc_pr.o util.o list.o -lm -o calc_pr

#
#  File conversion utility for qrels.text
#

convert.o :	convert.c  util.h
	$(CC) convert.c

convert :	convert.o  util.o
	$(LD) convert.o util.o -o convert

#
#  Term frequency calculation
#

termfreq.o :	termfreq.c  util.h  list.h
	$(CC) termfreq.c

termfreq :	termfreq.o  util.o  list.o
	$(LD) termfreq.o util.o list.o -o termfreq

#
#  Centroid calculation
#

termdisc.o :	termdisc.c  util.h  list.h
	$(CC) termdisc.c

termdisc :	termdisc.o  util.o  list.o
	$(LD) termdisc.o util.o list.o -lm -o termdisc
