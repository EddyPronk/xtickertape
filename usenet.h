/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1995.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, Gehrmann Labs
     University of Queensland
     St Lucia, 4072
     Australia
     Tel: +61 7 3365 4310
     Fax: +61 7 3365 4311
     Email: enquiries@dstc.edu.au

  This software is being provided "AS IS" without warranty of
  any kind.  In no event shall DSTC Pty Ltd be liable for
  damage of any kind arising out of or in connection with
  the use or performance of this software.

****************************************************************/

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

