/***************************************************************

  Copyright (C) 2002, 2004 by Mantara Software
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

static char *default_keys_file =
"# Tickertape keys file - $HOME/.ticker/keys\n"
"#\n"
"# empty lines and those beginning with a hash are ignored\n"
"#\n"
"# Each line has the following format:\n"
"# \n"
"#	<key name>:<key type>:<key format>:<key data>\n"
"#\n"
"# key name    an arbitrary unique label for the key\n"
"# key type\n"
"#     public       somebody else's (prime) key\n"
"#     private      your (raw) key\n"
"# key format  where and how to load the key\n"
"#     hex-inline   hexadecimal data at the end of the line\n"
"#     hex-file     hexadecimal data in the named file\n"
"#     binary-file  binary data in the named file\n"
"# key data    the key contents, or the file name (depending on the key\n"
"#             format field)\n"
"#\n"
"# Some examples\n"
"\n"
"# Your own private key (replace this with secret data before uncommenting it!)\n"
"#%u@%d:private:hex-inline:01 23 45 67 89 ab cd ef\n"
"\n"
"# Alternatively your key can be stored in a separate file to allow this file\n"
"# to be readable by others without giving away your secret key\n"
"#%u@%d:private:hex-file:%h/.ticker/keys/%u@%d\n"
"\n"
"# Somebody else's public key\n"
"ilister@dstc.com:public:hex-inline:c03db3b5c9a8e4c7c8c63698189b3ef82f2b9753\n";
