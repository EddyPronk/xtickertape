/***************************************************************

  Copyright (C) 1999, 2001-2002, 2004 by Mantara Software
  (ABN 17 105 665 594). All Rights Reserved.

  This software is the property of the Mantara Software. All use
  of this software is permitted only under the terms of a
  license agreement with Mantara Software. If you do not have
  such a license, then you have no rights to use this software
  in any manner and should contact Mantara at the address below
  to determine an appropriate licensing arrangement.
  
     Mantara Software
     PO Box 1820
     Toowong QLD 4066
     Australia
     Tel: +61 7 3876 8844
     Fax: +61 7 3876 8843
     Email: licensing@mantara.com
  
     Web: http://www.mantara.com
  
  This software is being provided "AS IS" without warranty of
  any kind. In no event shall Mantara Software be liable for
  damage of any kind arising out of or in connection with the
  use or performance of this software.

****************************************************************/

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

