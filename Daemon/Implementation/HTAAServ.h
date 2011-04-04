/*                          SERVER SIDE ACCESS AUTHORIZATION MODULE
                                             
   This module is the server side interface to Access Authorization (AA) package. It
   contains code only for server.
   
   _Important_ to know about memory allocation:
   
   Routines in this module use dynamic allocation, but free automatically all the memory
   reserved by them.
   
   Therefore the caller never has to (and never should) free()any object returned by these
   functions.
   
   Therefore also all the strings returned by this package are only valid until the next
   call to the same function is made. This approach is selected, because of the nature of
   access authorization: no string returned by the package needs to be valid longer than
   until the next call.
   
   This also makes it easy to plug the AA package in: you don't have to ponder whether to
   free()something here or is it done somewhere else (because it is always done somewhere
   else).
   
   The strings that the package needs to store are copied so the original strings given as
   parameters to AA functions may be freed or modified with no side effects.
   
   _Also note:_ The AA package does not free()anything else than what it has itself
   allocated.
   
 */
#ifndef HTAASERV_H
#define HTAASERV_H

#include <stdio.h>              /* FILE                 */
#include "HTUtils.h"            /* BOOL, PARAMS, ARGS   */
#include "HTRules.h"            /* This module interacts with rule system */
#include "HTAAUtil.h"           /* Common parts of AA   */
#include "HTAuth.h"             /* Authentication       */


#ifdef SHORT_NAMES
#define HTAAstMs        HTAA_statusMessage
#define HTAAchAu        HTAA_checkAuthorization
#define HTAAcoAH        HTAA_composeAuthHeaders
#define HTAAsLog        HTAA_startLogging
#endif /*SHORT_NAMES*/

/*

CHECK ACCESS AUTHORIZATION

   HTAA_checkAuthorization()is the main access authorization function.
   
 */
/* PUBLIC                                             HTAA_checkAuthorization()
**              CHECK IF USER IS AUTHORIZED TO ACCESS A FILE
** ON ENTRY:
**      req     data structure representing the request.
**
** ON EXIT:
**      returns status codes uniform with those of HTTP:
**        200 OK           if file access is ok.
**        401 Unauthorized if user is not authorized to
**                         access the file.
**        403 Forbidden    if there is no entry for the
**                         requested file in the ACL.
**
** NOTE:
**      This function does not check whether the file
**      exists or not -- so the status  404 Not found
**      must be returned from somewhere else (this is
**      to avoid unnecessary overhead of opening the
**      file twice).
**
*/
PUBLIC int HTAA_checkAuthorization PARAMS((HTRequest * req));
/*

COMPOSE STATUS LINE MESSAGE

 */
/* SERVER PUBLIC                                        HTAA_statusMessage()
**              RETURN A STRING EXPLAINING ACCESS
**              AUTHORIZATION FAILURE
**              (Can be used in server reply status line
**               with 401/403 replies.)
** ON ENTRY:
**      req     failing request.
** ON EXIT:
**      returns a string containing the error message
**              corresponding to internal HTAAFailReason.
*/
PUBLIC char *HTAA_statusMessage PARAMS((HTRequest * req));
/*

COMPOSE "AUTHENTICATE:" HEADER LINES FOR SERVER REPLY

 */
/* SERVER PUBLIC                                    HTAA_composeAuthHeaders()
**              COMPOSE WWW-Authenticate: HEADER LINES
**              INDICATING VALID AUTHENTICATION SCHEMES
**              FOR THE REQUESTED DOCUMENT
** ON ENTRY:
**      req     request.
**
** ON EXIT:
**      returns a buffer containing all the WWW-Authenticate:
**              fields including CRLFs (this buffer is auto-freed).
**              NULL, if authentication won't help in accessing
**              the requested document.
*/
PUBLIC char *HTAA_composeAuthHeaders PARAMS((HTRequest * req));
/*

START ACCESS AUTHORIZATION LOGGING

 */
/* PUBLIC                                               HTAA_startLogging()
**              START UP ACCESS AUTHORIZATION LOGGING
** ON ENTRY:
**      fp      is the open log file.
**
*/
PUBLIC void HTAA_startLogging PARAMS((FILE * fp));

PUBLIC void HTSetAttributes PARAMS((HTRequest *   req,
                                    struct stat * stat_info));

PUBLIC BOOL HTGetAttributes PARAMS((HTRequest * req,
                                    char *      path));

/*

 */
#endif  /* NOT HTAASERV_H */
/*

   End of file HTAAServ.h. */
