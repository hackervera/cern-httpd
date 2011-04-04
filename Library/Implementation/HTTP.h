/*                                                                     HTTP module for libwww
                            HYPERTEXT TRANFER PROTOCOL MODULE
                                             
   This is the module that handles all communication with HTTP servers. The module is a
   part of the CERN Common WWW Library.
   
 */

#ifndef HTTP_H
#define HTTP_H
#include "HTAccess.h"
/*

Public Functions

   Theese are the public functions...
   
  ACCESSING HTTP-SERVER
  
 */

extern int HTLoadHTTP PARAMS((HTRequest * request));
/*

Controlling Flags

   The following variables can change the behaviour of the module.
   
  CACHE CONTROL FLAG
  
   Note: This variable is now replaced by the (char *) HTCacheDir in  HTAccess Module
   
   Turn this off if you don't want HTTP protocol fetches to leave cache files.  extern
   BOOL  HTCacheHTTP;
   
  REDIRECTIONS
  
   The maximum number of redirections is pr. default 10 and is set in the module. This
   prevents the library from going into an infinite loop.
   
 */

extern int HTMaxRedirections;
/*

  DISABLE/ENABLE USER IDENTIFICATION IN HTTP REQUEST
  
   If a client want the user's email address to be send in the HTTP request as a From
   field then turn this flag on. The default is off because it might cause security
   problems from within a firewall. When enabled, the format used is user@host.domain. The
   value can be changed, see HTTCP Module.
   
 */

extern BOOL HTEnableFrom;
/*

 */

#endif /* HTTP_H */
/*

   End of HTTP module definition.  */
