/***********************************************************************

  Copyright (C) 1997-2009 by Mantara Software (ABN 17 105 665 594).
  All Rights Reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   * Redistributions of source code must retain the above
     copyright notice, this list of conditions and the following
     disclaimer.

   * Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials
     provided with the distribution.

   * Neither the name of the Mantara Software nor the names
     of its contributors may be used to endorse or promote
     products derived from this software without specific prior
     written permission. 

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.

***********************************************************************/

static char *defaultUsenetFile =
"# $HOME/.ticker/usenet -- Usenet subscription for Tickertape\n"
"#\n"
"# empty lines and those beginning with a hash are ignored\n"
"#\n"
"# each field after the group match is of the form:\n"
"#\n"
"#       <field> <op> <pattern>\n"
"#\n"
"# and seperated by slashes, `/'\n"
"#\n"
"# field is one of:\n"
"#       from \n"
"#       email\n"
"#       subject\n"
"#       keywords\n"
"#       x-posts (cross posts)\n"
"#\n"
"# op is one of:\n"
"#  matches -- regex\n"
"#      not -- does not match regex\n"
"#        = -- equals\n"
"#       != -- not equals\n"
"#        < -- less than (Cross posts only)\n"
"#        > -- greater than (Cross posts only)\n"
"#       <= -- less or equal (Cross posts only)\n"
"#       >= -- greater or equal (Cross posts only)\n"
"#\n"
"# pattern can be a number, a words (or words) or a regex depending on \n"
"# the operator\n"
"#\n"
"# The spaces between the <field>, <op> and the <pattern> ARE important\n"
"#\n"
"# Regex matches a NOT case sensitive\n"
"#\n"
"# Each expression on a single line is AND'ed together.  Different\n"
"# lines are the equivalent of OR'ing\n"
"#\n"
"# A few examples:\n"
"#\n"
"# All articles from Linus Torvalds\n"
"#.*/ from matches Linus\n"
"\n"
"# All articles from people from DSTC but not the ones in local groups\n"
"#not dstc.* / email matches .*dstc.edu.au\n"
"\n"
"# All articles in the DSTC announcement group\n"
"#dstc.announce\n"
"\n"
"# Articles in the computer groups about PGP, but not the ones heavily\n"
"# cross posted\n"
"#comp.* / subject matches PGP / x-posts < 5\n"
"\n"
"# All articles in forsale groups selling SPARCs posted by people in Oz\n"
"#.*forsale.* / subject matches SPARC / email matches .*au\n"
"\n"
"# Articles about Mitnick and from one of his friends\n"
"#.* / subject matches mitnick\n"
"#.* / email matches lewiz\n"
"#.* / from matches lewiz\n";

