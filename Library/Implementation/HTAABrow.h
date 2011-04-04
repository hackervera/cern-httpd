/*                          BROWSER SIDE ACCESS AUTHORIZATION MODULE
                                             
   This module is the browser side interface to Access Authorization (AA) package.  It
   contains code only for browser.
   
   Important to know about memory allocation:
   
   Routines in this module use dynamic allocation, but free automatically all the memory
   reserved by them.
   
   Therefore the caller never has to (and never should) free() any object returned by
   these functions.
   
   Therefore also all the strings returned by this package are only valid until the next
   call to the same function is made. This approach is selected, because of the nature of
   access authorization: no string returned by the package needs to be valid longer than
   until the next call.
   
   This also makes it easy to plug the AA package in: you don't have to ponder whether to
   free()something here or is it done somewhere else (because it is always done somewhere
   else).
   
   The strings that the package needs to store are copied so the original strings given as
   parameters to AA functions may be freed or modified with no side effects.
   
   Also note:The AA package does not free() anything else than what it has itself
   allocated.
   
 */

#ifndef HTAABROW_H
#define HTAABROW_H

#include "HTUtils.h"            /* BOOL, PARAMS, ARGS */
#include "HTAAUtil.h"           /* Common parts of AA */
#include "HTList.h"
#include "HTAssoc.h"

#ifdef SHORT_NAMES
#define HTAAcoAu        HTAA_composeAuth
#define HTAAsRWA        HTAA_shouldRetryWithAuth
#endif /*SHORT_NAMES*/


/*
** Local datatype definitions
**
** HTAAServer contains all the information about one server.
*/
typedef struct {

    char *      hostname;       /* Host's name                  */
    int         portnumber;     /* Port number                  */
    HTList *    setups;         /* List of protection setups    */
                                /* on this server; i.e. valid   */
                                /* authentication schemes and   */
                                /* templates when to use them.  */
                                /* This is actually a list of   */
                                /* HTAASetup objects.           */
    HTList *    realms;         /* Information about passwords  */
} HTAAServer;


/*
** HTAASetup contains information about one server's one
** protected tree of documents.
*/
typedef struct {
    HTAAServer *server;         /* Which server serves this tree             */
    char *      template;       /* Template for this tree                    */
    HTList *    valid_schemes;  /* Valid authentic.schemes                   */
    HTAssocList**scheme_specifics;/* Scheme specific params                  */
    BOOL        reprompt;       /* Failed last time -- reprompt (or whatever)*/
} HTAASetup;


/*
** Information about usernames and passwords in
** Basic and Pubkey authentication schemes;
*/
typedef struct {
    char *      realmname;      /* Password domain name         */
    char *      username;       /* Username in that domain      */
    char *      password;       /* Corresponding password       */
} HTAARealm;


#include "HTAccess.h"           /* HTRequest */

/*

Routines for Browser Side Recording of AA Info

   Most of the browser-side AA is done by the following two functions (which are called
   from file HTTP.c so the browsers using libwww only need to be linked with the new
   library and not be changed at all):
   
      HTAA_composeAuth() composes the Authorization: line contents, if the AA package
      thinks that the given document is protected. Otherwise this function returns NULL.
      This function also calls the functions HTPrompt(),HTPromptPassword() and HTConfirm()
      to get the username, password and some confirmation from the user.
      
      HTAA_shouldRetryWithAuth() determines whether to retry the request with AA or with a
      new AA (in case username or password was misspelled).
      
 */

/* BROWSER PUBLIC                                       HTAA_composeAuth()
**
**      COMPOSE Authorization: HEADER LINE CONTENTS
**      IF WE ALREADY KNOW THAT THE HOST REQUIRES AUTHENTICATION
**
** ON ENTRY:
**      req             request, which contains
**      req->argument   document, that we're trying to access.
**      req->setup      protection setup info on browser.
**      req->scheme     selected authentication scheme.
**      req->realm      for Basic scheme the username and password.
**
** ON EXIT:
**      returns NO, if no authorization seems to be needed, and
**              req->authorization is NULL.
**              YES, if it has composed Authorization field,
**              in which case the result is in req->authorization,
**              e.g.
**
**                 "Basic AkRDIhEF8sdEgs72F73bfaS=="
*/
PUBLIC BOOL HTAA_composeAuth PARAMS((HTRequest * req));


/* BROWSER PUBLIC                                       HTAA_retryWithAuth()
**
**              RETRY THE SERVER WITH AUTHORIZATION (OR IF
**              ALREADY RETRIED, WITH A DIFFERENT USERNAME
**              AND/OR PASSWORD (IF MISSPELLED)) OR CANCEL
** ON ENTRY:
**      req             request.
**      req->valid_schemes
**                      This function should only be called when
**                      server has replied with a 401 (Unauthorized)
**                      status code, and req structure has been filled
**                      up according to server reply, especially the
**                      req->valid_shemes list must have been set up
**                      according to WWW-Authenticate: headers.
**      callback        the function to call when username and
**                      password have been entered (HTLoadHTTP()).
** ON EXIT:
**      On GUI clients pops up a username/password dialog box
**      with "Ok" and "Cancel".
**      "Ok" button press should do a callback
**
**              HTLoadHTTP(req);
**
**      This actually done by function HTPasswordDialog(),
**      which GUI clients redefine.
**
**      returns         YES, if dialog box was popped up.
**                      NO, on failure.
*/
PUBLIC BOOL HTAA_retryWithAuth PARAMS((HTRequest *              req,
                                       HTRetryCallbackType      callback));


/* PUPLIC                                               HTAA_cleanup()
**
**      Free the memory used by the entries concerning Access Authorization
**      in the request structure and put all pointers to NULL
**      Henrik 14/03-94.
**
** ON ENTRY:
**      req             the request structure
**
** ON EXIT:
**      returns         nothing.
*/
PUBLIC void HTAACleanup PARAMS((HTRequest *req));
/*

Enabling Gateway httpds to Forward Authorization

   These functions should only be called from daemon code, and HTAAForwardAuth_reset()
   must be called before the next request is handled to make sure that authorization
   string isn't cached in daemon so that other people can access private files using
   somebody elses previous authorization information.
   
 */

PUBLIC void HTAAForwardAuth_set PARAMS((CONST char * scheme_name,
                                        CONST char * scheme_specifics));
PUBLIC void HTAAForwardAuth_reset NOPARAMS;
/*

 */

#endif  /* NOT HTAABROW_H */
/*

   End of file HTAABrow.h.  */
