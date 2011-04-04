/*                                                          Initialization routines in libwww
                                  INITIALIZATION MODULE
                                             
   This module resisters all the plug & play software modules which will be used in the
   program on client side (The server has it's own initialization module). The module is a
   part of the CERN Common WWW Library.
   
   The initialization consists of two phases: Setting up the MIME type conversions and
   defining the relation between a basic set of file suffixes and MIME types.  To override
   this, just copy it and link in your version before you link with the library.
   
   Implemented by HTInit.c by default.
   
 */

#include "HTUtils.h"
#include "HTList.h"
/*

MIME Type Conversions

   Two default functions can be used to do the MIME type initialization. The first
   contains all MIME types the client can handle plus the types that can be handled using
   save and execute calls, e.g. to start a PostScript viewer. The second function only
   contains MIME type conversions that can be handled internally in the client. As an
   example, the latter is used when  Line Mode Browser is started in Non-Interactive mode.
   
 */

PUBLIC void HTFormatInit PARAMS((HTList * conversions));
PUBLIC void HTFormatInitNIM PARAMS((HTList * conversions));
/*

File Suffix Setup

   This functions defines a basic set of file suffixes and the corresponding MIME types.
   
 */

PUBLIC void HTFileInit NOPARAMS;
/*

   End of HTInit Module.  */
