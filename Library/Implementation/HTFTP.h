/*                                                               FTP access module for libwww
                                   FTP ACCESS FUNCTIONS
                                             
   This is the FTP load module that handles all communication with FTP-servers. The module
   is part of the CERN Common WWW Library.
   
   Authors
   
      Tim Berners-lee, timbl@info.cern.ch
      
      Denis DeLaRoca 310 825-4580, CSP1DWD@mvs.oac.ucla.edu
      
      Lou Montulli, montulli@ukanaix.cc.ukans.edu
      
      Foteos Macrides, macrides@sci.wfeb.edu
      
      Henrik Frystyk, frystyk@dxcern.cern.ch
      
 */
#ifndef HTFTP_H
#define HTFTP_H
#include "HTChunk.h"
/*

Public Functions

   Theese are the public functions...
   
  ACCESSING FTP-SERVER
  
 */

extern int  HTLoadFTP PARAMS((HTRequest * request));
/*

  ENABLE/DISABLE REUSE OF CONTROL CONNECTIONS ON CLIENT SIDE
  
   The next two functions are for enabling and disabling reuse og control connections on
   client side. Though, this is a temporary solution as the library is going to be
   multi-threaded and then the control of open connections changes. Reuse of control
   connections is mainly intended for use when loading several files from the same server
   in the same directory, but changing directory IS supported using FTP-commands CDUP and
   CWD.
   
 */

extern void HTFTP_enable_session NOPARAMS;
extern BOOL HTFTP_disable_session NOPARAMS;
/*

  VARIOUS FUNCTIONS TO PARSE INFORMATION
  
   Theese functions are necessary in order to keep the internal data structures hidden.
   
 */

PUBLIC HTChunk *HTFTPWelcomeMsg PARAMS ((HTNetInfo *data));
PUBLIC BOOL HTFTUseList PARAMS ((HTNetInfo *data));
/*

Flags for FTP connections

   Those are the flags for configuring the FTP client.
   
 */

extern BOOL HTFTPUserInfo;
extern long HTFTPTimeOut;
/*

   If HTFTPUserInfo = YES (as pr default) then the users login name and password is reused
   when conneting to the same host. It is, however, overwritten by any userid and passwd
   specified in the URL. This is only for the client side, as server forks itself on any
   request. If this flag is not set, then anonymous and username of the current proces is
   used.
   
   In addition, the following defines are available in the module:
   
  LISTEN                 This defines makes it possible to use PORT and hence do an
                         passive open for the data connection. Though, if defined, this is
                         only used AFTER an active open has been tried using PASV.
                         
  REPEAT_PORT            If LISTEN is defined, then when we have found a passive port,
                         then reuse it for the next time, else we ask the system to get a
                         new one.
                         
  POLL_PORTS             If the system doesn't support finding a new port, then let's try
                         it ourselves.
                         
 */

#endif
/*

   end of HTFTP Module  */
