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

static char *default_groups_file =
"# Tickertape groups file - $HOME/.ticker/groups\n"
"#\n"
"# empty lines and those beginning with a hash are ignored\n"
"#\n"
"# Each line has the following format:\n"
"# \n"
"#	<group name>:<menu option>:<auto option>:<min time>:<max time>:<keys>\n"
"#\n"
"# group name	is the group of Tickertape events the line relates to\n"
"# menu option	whether the group appears on the 'Send' menu.  One of\n"
"#		'menu' or 'no menu'\n"
"# auto option	indicates MIME attachments should be automatically\n"
"#		viewed.  One of 'auto' or 'manual'\n"
"# min/max time	sets limits on the duration events are displayed - in mins\n"
"# keys		comma-separated list of key names to use (from the keys file)\n"
"#\n"
"# The order that the groups appear in this file determines the order\n"
"# they appear in the Group menu when sending events.  With multiple\n"
"# lines for the same group, all but the first are ignored\n"
"#\n"
"# Some examples\n"
"\n"
"# A misc chat group\n"
"Chat:menu:manual:1:60\n"
"\n"
"# Keep track of the coffee-pigs\n"
"Coffee:no menu:manual:1:30\n"
"\n"
"# Room booking events\n"
"Rooms:no menu:manual:1:60\n"
"\n"
"# All the gossip\n"
"level7:menu:manual:1:60\n"
"\n"
"# System messages about what was rebooted in the previous hour\n"
"rwall:no menu:manual:1:60\n"
"\n"
"# Your own group so that you can talk to yourself\n"
"%u@%d:menu:manual:1:10\n";

