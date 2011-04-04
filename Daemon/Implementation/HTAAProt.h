/*                                   PROTECTION SETUP FILE
                                             
 */
#ifndef HTAAPROT_H
#define HTAAPROT_H

#include "HTUtils.h"
#include "HTGroup.h"
#include "HTAssoc.h"


#ifdef SHORT_NAMES
#define HTAAgUid        HTAA_getUid
#define HTAAgGid        HTAA_getGid
#define HTAAgDPr        HTAA_setDefaultProtection
#define HTAAsCPr        HTAA_setCurrentProtection
#define HTAAgCPr        HTAA_getCurrentProtection
#define HTAAgDPr        HTAA_getDefaultProtection
#define HTAAclPr        HTAA_clearProtections
#endif /*SHORT_NAMES*/

typedef struct _HTUidGid {
    char *      uname;
    char *      gname;
    int         uid;
    int         gid;
} HTUidGid;

/*
 *      Allocate and free a new uid and gid structure;
 *      set the current ids to eventually run as (done by calls
 *      from the rule module).
 */
PUBLIC HTUidGid * HTUidGid_new PARAMS((char * u, char * g));
PUBLIC void HTUidGid_free PARAMS((HTUidGid * ug));
PUBLIC void HTSetCurrentIds PARAMS((HTUidGid * ids));
/*

SERVER'S REPRESENTATION OF DOCUMENT (TREE) PROTECTIONS

 */
typedef struct {
    char *        template;     /* Template for this protection         */
#ifdef NOT_NEEDED_NOR_SHOULD_BE
    char *        filename;     /* Current document file                */
#endif
    HTUidGid *    ids;          /* User and group ids to run as         */
    BOOL          acl_override; /* Do ACL's override Masks              */
    GroupDef *    get_mask;     /* Allowed users and IP addresses (GET) */
    GroupDef *    put_mask;     /*  - " - (PUT)                         */
    GroupDef *    post_mask;    /*  - " - (POST)                        */
    GroupDef *    delete_mask;  /*  - " - (DELETE)                      */
    GroupDef *    gen_mask;     /* General mask (used when needed but   */
                                /* other masks not set).                */
    HTList *      valid_schemes;/* Valid authentication schemes         */
    HTAssocList * values;       /* Association list for scheme specific */
                                /* parameters.                          */
} HTAAProt;

extern HTAAProt * HTProt;       /* Current protection setup */
extern HTAAProt * HTDefProt;    /* Default protection setup */


/*

 */
#include "HTAccess.h"   /* HTRequest structure */

/*

CALLBACKS FOR RULE SYSTEM

   The following three functioncs are called by the rule system:
   
      HTAA_clearProtections()when starting to translate a filename
      
      HTAA_setDefaultProtection()when "defprot" rule is matched
      
      HTAA_setCurrentProtection()when "protect" rule is matched
      
   Protection setup files are cached by these functions.
   
 */
/* PUBLIC                                       HTAA_setDefaultProtection()
**              SET THE DEFAULT PROTECTION MODE
**              (called by rule system when a
**              "defprot" rule is matched)
** ON ENTRY:
**      req             request.
**      prot_filename   is the protection setup file (second argument
**                      for "defprot" rule, optional)
**      eff_ids         contains user and group id information.
**
** ON EXIT:
**      returns         nothing.
**                      Sets the module-wide variable default_prot.
*/
PUBLIC void HTAA_setDefaultProtection PARAMS((HTRequest *       req,
                                              CONST char *      prot_filename,
                                              HTUidGid *        eff_ids));



/* PUBLIC                                       HTAA_setCurrentProtection()
**              SET THE CURRENT PROTECTION MODE
**              (called by rule system when a
**              "protect" rule is matched)
** ON ENTRY:
**      req             request.
**      prot_filename   is the protection setup file (second argument
**                      for "protect" rule, optional)
**      eff_ids         contains user and group id information.
**
** ON EXIT:
**      returns         nothing.
**                      Sets the module-wide variable current_prot.
*/
PUBLIC void HTAA_setCurrentProtection PARAMS((HTRequest *       req,
                                              CONST char *      prot_filename,
                                              HTUidGid *        eff_ids));


/* SERVER INTERNAL                                      HTAA_clearProtections()
**              CLEAR DOCUMENT PROTECTION MODE
**              (ALSO DEFAULT PROTECTION)
**              (called by the rule system)
** ON ENTRY:
**      No arguments.
**
** ON EXIT:
**      returns nothing.
**              Frees the memory used by protection information.
*/
PUBLIC void HTAA_clearProtections NOPARAMS;
/*

GETTING PROTECTION SETTINGS

      HTAA_getCurrentProtection()returns the current protection mode (if there was a
      "protect" rule). NULL, if no "protect" rule has been matched.
      
      HTAA_getDefaultProtection()sets the current protection mode to what it was set to by
      "defprot" rule and also returns it (therefore after this call also
      HTAA_getCurrentProtection()returns the same structure.
      
 */
/* PUBLIC                                       HTAA_getCurrentProtection()
**              GET CURRENT PROTECTION SETUP STRUCTURE
**              (this is set up by callbacks made from
**               the rule system when matching "protect"
**               (and "defprot") rules)
** ON ENTRY:
**      HTTranslate() must have been called before calling
**      this function.
**
** ON EXIT:
**      returns a HTAAProt structure representing the
**              protection setup of the HTTranslate()'d file.
**              This must not be free()'d.
*/
PUBLIC HTAAProt *HTAA_getCurrentProtection NOPARAMS;



/* PUBLIC                                       HTAA_getDefaultProtection()
**              GET DEFAULT PROTECTION SETUP STRUCTURE
**              (this is set up by callbacks made from
**               the rule system when matching "defprot"
**               rules)
** ON ENTRY:
**      HTTranslate() must have been called before calling
**      this function.
**
** ON EXIT:
**      returns a HTAAProt structure representing the
**              default protection setup of the HTTranslate()'d
**              file (if HTAA_getCurrentProtection() returned
**              NULL, i.e. if there is no "protect" rule
**              but ACL exists, and we need to know default
**              protection settings).
**              This must not be free()'d.
*/
PUBLIC HTAAProt *HTAA_getDefaultProtection NOPARAMS;
/*

GET USER AND GROUP IDS TO WHICH SET TO

 */
#ifndef VMS
/* PUBLIC                                                       HTAA_getUid()
**              GET THE USER ID TO CHANGE THE PROCESS UID TO
** ON ENTRY:
**      req     request.
**
** ON EXIT:
**      returns the uid number to give to setuid() system call.
**              Default is 65534 (nobody).
*/
PUBLIC int HTAA_getUid NOPARAMS;


/* PUBLIC                                                       HTAA_getGid()
**              GET THE GROUP ID TO CHANGE THE PROCESS GID TO
** ON ENTRY:
**      req     request.
**
** ON EXIT:
**      returns the uid number to give to setgid() system call.
**              Default is 65534 (nogroup).
*/
PUBLIC int HTAA_getGid NOPARAMS;
#endif /* not VMS */
/*

   For VMS:
   
 */
#ifdef VMS
/* PUBLIC                                                       HTAA_getUidName()
**              GET THE USER ID NAME (VMS ONLY)
** ON ENTRY:
**      No arguments.
**
** ON EXIT:
**      returns the user name
**              Default is "" (nobody).
*/
PUBLIC char * HTAA_getUidName NOPARAMS;

/* PUBLIC                                                       HTAA_getFileName
**              GET THE FILENAME (VMS ONLY)
** ON ENTRY:
**      No arguments.
**
** ON EXIT:
**      returns the filename
*/
PUBLIC char * HTAA_getFileName NOPARAMS;
#endif /* VMS */
/*

 */
PUBLIC HTAAProt * HTAAProt_parseInlined PARAMS((FILE * fp));

#endif /* not HTAAPROT_H */
/*

   End of file HTAAProt.h. */
