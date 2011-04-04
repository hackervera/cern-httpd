/*                                   AUTHENTICATION MODULE
                                             
   This is the authentication module. By modifying the function HTAA_authenticate()it can
   be made to support external authentication methods.
   
 */
#ifndef HTAUTH_H
#define HTAUTH_H

#include "HTUtils.h"
#include "HTAAUtil.h"


#ifdef SHORT_NAMES
#define HTAAauth        HTAA_authenticate
#endif /* SHORT_NAMES */


/*
** Server's representation of a user (fields in authentication string)
*/
typedef struct {
    HTAAScheme  scheme;         /* Scheme used to authenticate this user */
    char *      username;
    char *      password;
    char *      inet_addr;
    char *      timestamp;
    char *      secret_key;
} HTAAUser;

extern HTAAUser * HTUser;

#include "HTAAProt.h"

/*

USER AUTHENTICATION

 */
/* SERVER PUBLIC                                        HTAA_authenticate()
**                      AUTHENTICATE USER
** ON ENTRY:
**      req             request.
**      req->scheme     used authentication scheme.
**      req->scheme_specifics
**                      the scheme specific parameters
**                      (authentication string for Basic and
**                      Pubkey schemes).
**      req->prot       is the protection information structure
**                      for the file.
**
** ON EXIT:
**      returns         YES, if authentication succeeds and
**                      req->user is set to point to the authenticated
**                      user.  NO, if authentication fails.
*/
PUBLIC BOOL HTAA_authenticate PARAMS((HTRequest * req));
/*

 */
#endif /* not HTAUTH_H */
/*

   End of file HTAuth.h. */
