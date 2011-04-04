/*                                                      HTAccess:  Access manager  for libwww
                                      ACCESS MANAGER
                                             
   This module keeps a list of valid protocol (naming scheme) specifiers with associated
   access code.  It allows documents to be loaded given various combinations of
   parameters. New access protocols may be registered at any time.
   
   Note: HTRequest defined and request parameter added to almost all calls 18 Nov 1993.
   
   This document is a part of the  libwww library. The code is implemented in  HTAcces.c
   
 */

#ifndef HTACCESS_H
#define HTACCESS_H
#include "HTList.h"
/*

   Short Names
   
 */

#ifdef SHORT_NAMES
#define HTClientHost            HTClHost
#define HTSearchAbsolute        HTSeAbso
#define HTOutputStream          HTOuStre
#define HTOutputFormat          HTOuForm
#endif
/*

Flags which may be set to control this module

 */

extern char * HTSaveLocallyDir;         /* Dir home for "save locally" files */
extern char * HTCacheDir;               /* Cache dir, default NULL: no cache */
extern char * HTClientHost;             /* Name or number of telnetting host */
extern FILE * HTlogfile;                /* File to output one-liners to */
extern BOOL HTSecure;                   /* Disable security holes? */
extern char * HTImServer;               /* If I'm cern_httpd */
extern BOOL HTImProxy;                  /* If I'm cern_httpd as a proxy */
extern BOOL HTForceReload;              /* Force reload from cache or net */
/*

Method Management

   Thesse are the valid methods, see HTTP Methods
   
 */

typedef enum {
        METHOD_INVALID  = 0,
        METHOD_GET      = 1,
        METHOD_HEAD,
        METHOD_POST,
        METHOD_PUT,
        METHOD_DELETE,
        METHOD_CHECKOUT,
        METHOD_CHECKIN,
        METHOD_SHOWMETHOD,
        METHOD_LINK,
        METHOD_UNLINK,
        MAX_METHODS
} HTMethod;
/*

  GET METHOD ENUMERATION
  
   Gives the enumeration value of the method as a function of the (char *) name.
   
 */

PUBLIC HTMethod HTMethod_enum PARAMS((char * name));
/*

  GET METHOD STRING
  
   The reverse of HTMethod_enum()
   
 */

PUBLIC char * HTMethod_name PARAMS((HTMethod method));
/*

  VALID METHODS
  
   This functions searches the list of valid methods for a given anchor, see HTAnchor
   module If the method is found it returns YES else NO.
   
 */

PUBLIC BOOL HTMethod_inList PARAMS((HTMethod    method,
                                    HTList *    list));
/*

   
   ___________________________________
   This section might be move to the Access Authentication Module
   
    Match Template Against Filename
    
 */

/* PUBLIC                                               HTAA_templateMatch()
**              STRING COMPARISON FUNCTION FOR FILE NAMES
**                 WITH ONE WILDCARD * IN THE TEMPLATE
** NOTE:
**      This is essentially the same code as in HTRules.c, but it
**      cannot be used because it is embedded in between other code.
**      (In fact, HTRules.c should use this routine, but then this
**       routine would have to be more sophisticated... why is life
**       sometimes so hard...)
**
** ON ENTRY:
**      template        is a template string to match the file name
**                      agaist, may contain a single wildcard
**                      character * which matches zero or more
**                      arbitrary characters.
**      filename        is the filename (or pathname) to be matched
**                      agaist the template.
**
** ON EXIT:
**      returns         YES, if filename matches the template.
**                      NO, otherwise.
*/
PUBLIC BOOL HTAA_templateMatch PARAMS((CONST char * template,
                                       CONST char * filename));
/*

   
   ___________________________________
   The following have to be defined in advance of the other include files because of
   circular references.
   
 */

typedef struct _HTRequest HTRequest;

/*
** Callback to call when username and password
** have been prompted.
*/
typedef int (*HTRetryCallbackType) PARAMS((HTRequest * req));


#include "HTAnchor.h"
#include "HTFormat.h"
#include "HTAAUtil.h"           /* HTAAScheme, HTAAFailReason */
#include "HTAABrow.h"           /* HTAASetup */
/*

Default WWW Addresses

   These control the home page selection. To mess with these for normal browses is asking
   for user confusion.
   
 */

#define LOGICAL_DEFAULT "WWW_HOME"            /* Defined to be the home page */

#ifndef PERSONAL_DEFAULT
#define PERSONAL_DEFAULT "WWW/default.html"             /* in home directory */
#endif

#ifndef LOCAL_DEFAULT_FILE
#define LOCAL_DEFAULT_FILE "/usr/local/lib/WWW/default.html"
#endif

/* If one telnets to an access point it will look in this file for home page */
#ifndef REMOTE_POINTER
#define REMOTE_POINTER  "/etc/www-remote.url"               /* can't be file */
#endif

/* and if that fails it will use this. */
#ifndef REMOTE_ADDRESS
#define REMOTE_ADDRESS  "http://info.cern.ch/remote.html"   /* can't be file */
#endif

/* If run from telnet daemon and no -l specified, use this file: */
#ifndef DEFAULT_LOGFILE
#define DEFAULT_LOGFILE "/usr/adm/www-log/www-log"
#endif

/* If the home page isn't found, use this file: */
#ifndef LAST_RESORT
#define LAST_RESORT     "http://info.cern.ch/default.html"
#endif

/* This is the default cache directory: */
#ifndef CACHE_HOME_DIR
#define CACHE_HOME_DIR          "/tmp/"
#endif

/* The default directory for "save locally" and "save and execute" files: */
#ifndef SAVE_LOCALLY_HOME_DIR
#define SAVE_LOCALLY_HOME_DIR   "/tmp/"
#endif
/*

Protocol Specific Information

   This structure contains information about socket number, input buffer for reading from
   the network etc. The structure is used through out the protocol modules and is the
   refenrence point for introducing multi threaded execution into the library, see
   specifications on Multiple Threads.
   
 */

typedef struct _HTNetInfo {
    int                 sockfd;                         /* Socket descriptor */
    HTInputSocket *     isoc;                                /* Input buffer */
    int                 addressCount;        /* Attempts if multi-homed host */
    BOOL                CRLFdotCRLF;  /* Does the transmission end like this */
    struct _HTRequest * request;           /* Link back to request structure */
} HTNetInfo;
/*

   Note: The AddressCount varaible is used to count the number of attempt to connect to a
   multi-homed host so we know when to stop trying new IP-addresses.
   
The Request structure

   When a request is handled, all kinds of things about it need to be passed along.  These
   are all put into a HTRequest structure.
   
   Note 1: There is also a global list of converters
   
   Note 2: If you reuse the request structure for more than one request then make sure
   that the request is re-initialized, so that no `old' data is reused, see functions to
   manipulate HTRequest Structure. The library handles its own internal information from
   request to request but the information set by the caller is untouched.
   
 */
struct _HTRequest {

/*

   The elements of the request structure are as follows.
   
  SET BY THE CALLER OF HTACCESS:
  
    Conditions of the request itself:
    
 */
        HTMethod        method;

/*

   An atom for the name of the operation using HTTP method names .
   
 */
        HTList *        conversions ;
/*

   NULL, or a list of conversions which the format manager can do in order to fulfill the
   request.  This is set by the caller of HTAccess. Typically points to a list set up an
   initialisation time for example by HTInit.
   
 */
        HTList *        encodings;      /* allowed content-encodings      */

/*

   The list of encodings acceptablein the output stream.
   
 */
        HTList *        languages;      /* accepted content-languages     */

/*

   The list of (human) language values acceptable in the response
   
 */
        BOOL (* callback ) PARAMS((
                                struct _HTRequest* request,
                                void *param));

/*

   A function to be called back in the event that a file has been saved to disk by
   HTSaveAndCallBack for example.
   
 */
        void *          context;        /* caller data -- HTAccess unaware */

/*

   An arbitrary pointer passed to HTAccess and passed back as a parameter to the callback
   .
   
 */
        HTStream*       output_stream;

/*

   NULL in the case of display to the user, or if a specific output stream is required,
   the stream.
   
 */
        HTAtom *        output_format;

/*

   The output format required, or a generic format such as www/present (present to user).
   
    Information about the requester
    
 */
        char *          from;
/*

   Email format address of person responible for request, see From field in HTTP Protocol.
   The default address used is the current location, but that can be changed, see HTTCP
   module.
   
    The URI from which this request was obtained
    
 */
        HTParentAnchor *parentAnchor;
/*

   If this parameter is set then a `Referer: <parent address> is generated in the request
   to the server, see Referer field in a HTTP Request
   
  SET BY HTACCESS
  
   None of the bits below may be looked at by a client application except in the callback
   routine, when the anchor may be picked out.
   
 */
        HTParentAnchor* anchor;

/*

   The anchor for the object in question. Set immediately by HTAcesss.  Used by the
   protocol and parsing modules. Valid thoughout the access.
   
 */
        HTChildAnchor * childAnchor;    /* For element within the object  */

/*

   The anchor for the sub object if any.  The object builder should ensure that htis is
   selected, highlighted, etc when the object is loaded. NOTE: Set by HTAccess.
   
 */
        void *          using_cache;

/*

   pointer to cache element if cache hit
   
  ERROR DIAGNOSTICS
  
 */

        BOOL            error_block;    /* YES if stream has been used    */
        HTList *        error_stack;    /* List of errors                 */

/*

  LIBRARY SIDE
  
 */

        HTNetInfo *     net_info;       /* Information about socket etc. */
        int             redirections;   /* Number of redirections */
/*

  SERVER SIDE
  
 */

        HTAtom *        content_type;   /* Content-Type:                  */
        HTAtom *        content_language;/* Language                      */
        HTAtom *        content_encoding;/* Encoding                      */
        int             content_length; /* Content-Length:                */
        HTInputSocket * isoc;           /* InputSocket object for reading */
        char *          authorization;  /* Authorization: field           */
        HTAAScheme      scheme;         /* Authentication scheme used     */
/*

  CLIENT SIDE
  
 */

        HTList *        valid_schemes;  /* Valid auth.schemes             */
        HTAssocList **  scheme_specifics;/* Scheme-specific parameters    */
        char *          prot_template;  /* WWW-Protection-Template: field */
        HTAASetup *     setup;          /* Doc protection info            */
        HTAARealm *     realm;          /* Password realm                 */
        char *          dialog_msg;     /* Authentication prompt (client) */
        HTRetryCallbackType
                        retry_callback; /* Called when password entered   */
};

/*

Functions to Manipulate a HTRequest Structure

   Just to make things easier especially for clients, here are some functions to
   manipulate the request structure:
   
  CREATE BLANK REQUEST
  
   This request has defaults in -- in most cases it will need some information added
   before being passed to HTAccess, but it will work as is for a simple request.
   
 */

PUBLIC HTRequest * HTRequest_new NOPARAMS;
/*

  DELETE REQUEST STRUCTURE
  
   Frees also conversion list hanging from req->conversions.
   
 */

PUBLIC void HTRequest_delete PARAMS((HTRequest * req));
/*

  CLEAR A REQUEST STRUCTURE
  
   Clears a request structure so that it can be reused. The only thing that differs from
   using free/new is that the list of conversions is kept.
   
 */

extern void HTRequest_clear PARAMS((HTRequest * req));
/*

Load a document from relative name

  ON ENTRY,
  
  relative_name           The relative address of the file to be accessed.
                         
  here                    The anchor of the object being searched
                         
  request->anchor         not filled in yet
                         
  ON EXIT,
  
  returns    YES          Success in opening file
                         
  NO                      Failure
                         
 */
extern  BOOL HTLoadRelative PARAMS((
                CONST char *            relative_name,
                HTParentAnchor *        here,
                HTRequest *             request));


/*

Load a document from absolute name

  ON ENTRY,
  
  addr                    The absolute address of the document to be accessed.
                         
  filter                  if YES, treat document as HTML
                         
  request->anchor         not filled in yet
                         
 */

/*

  ON EXIT,
  
 */

/*

  returns YES             Success in opening document
                         
  NO                      Failure
                         
 */
extern BOOL HTLoadAbsolute PARAMS((CONST char * addr,
                HTRequest *             request));


/*

Load a document from absolute name to a stream

  ON ENTRY,
  
  addr                    The absolute address of the document to be accessed.
                         
  filter                  if YES, treat document as HTML
                         
  request->anchor         not filled in yet
                         
  ON EXIT,
  
  returns YES             Success in opening document
                         
  NO                      Failure
                         
   Note: This is equivalent to HTLoadDocument
   
 */
extern BOOL HTLoadToStream PARAMS((
                CONST char *            addr,
                BOOL                    filter,
                HTRequest *             request));


/*

Load if necessary, and select an anchor

   The anchor parameter may be a child anchor. The anchor in the request is set to the
   parent anchor. The recursive function keeps the error stack in  the request structure
   so that no information is lost having more than one call.
   
  ON ENTRY,
  
  anchor                  may be a child or parenet anchor or 0 in which case there is no
                         effect.
                         
  request->anchor            Not set yet.
                         
  ON EXIT,
  
 */

/*

  returns YES             Success
                         
  returns NO              Failure
                         
  request->anchor         The parenet anchor.
                         
 */

extern BOOL HTLoadAnchor PARAMS((HTAnchor  * a,
                                 HTRequest *            request));
extern BOOL HTLoadAnchorRecursive PARAMS((HTAnchor  * a,
                                          HTRequest *           request));
/*

Load a Document

   This is an internal routine, which has an address AND a matching anchor.  (The public
   routines are called with one OR the other.) This is recursively called from file load
   module to try ftp (though this will be obsolete in the next major release).
   
  ON ENTRY,
  
  request->
                         
   anchor
                         
                          a parent anchor with fully qualified
                         
                         
                         
                              hypertext reference as its address set
                         
   output_format
                          valid
                         
   output_stream
                          valid on NULL
                         
   keep_error_stack       If this is set to YES then the error (or info) stack is not
                         cleared from the previous call.
                         
  ON EXIT,
  
   returns
   
                           Error has occured.
                         
   HT_LOADED
                          Success
                         
   HT_NO_DATA
                          Success, but no document loaded.
                         
                         
                         
                         
                         (telnet sesssion started etc)
                         
 */

PUBLIC int HTLoad PARAMS((HTRequest * request, BOOL keep_error_stack));
/*

Bind an anchor to a request structure without loading

   The anchor parameter may be a child anchor. The anchor in the request is set to the
   parent anchor. This is useful in non-interactive mode if no home-anchor is known.
   Actually the same as HTLoadAnchor(), but without loading
   
  ON ENTRY,
  
  anchor                  may be a child or parenet anchor or 0 in which case there is no
                         effect.
                         
  request->anchor         Not set yet.
                         
  ON EXIT,
  
 */

/*

   returns YES   Success
   
   returns NO    Failure
   
   request->anchor       The parenet anchor.
   
 */

extern BOOL HTBindAnchor PARAMS((HTAnchor *anchor, HTRequest *request));


/*

Make a stream for Saving object back

  ON ENTRY,
  
  request->anchor         is valid anchor which has previously beeing loaded
                         
  ON EXIT,
  
  returns                 0 if error else a stream to save the object to.
                         
 */


extern HTStream * HTSaveStream PARAMS((HTRequest * request));


/*

Search

   Performs a search on word given by the user. Adds the search words to the end of the
   current address and attempts to open the new address.
   
  ON ENTRY,
  
  *keywords               space-separated keyword list or similar search list
                         
  here                    The anchor of the object being searched
                         
 */
extern BOOL HTSearch PARAMS((
                CONST char *            keywords,
                HTParentAnchor*         here,
                HTRequest *             request));


/*

Search Given Indexname

   Performs a keyword search on word given by the user. Adds the keyword to  the end of
   the current address and attempts to open the new address.
   
  ON ENTRY,
  
  *keywords               space-separated keyword list or similar search list
                         
  *indexname              is name of object search is to be done on.
                         
 */
extern BOOL HTSearchAbsolute PARAMS((
        CONST char *            keywords,
        CONST char *            indexname,
        HTRequest *             request));


/*

Register an access method

   An access method is defined by an HTProtocol structure which point to the routines for
   performing the various logical operations on an object: in HTTP terms,  GET, PUT, and
   POST.
   
   Each of these routine takes as a parameter a request structure containing details ofthe
   request.  When the protocol class routine is called, the anchor elemnt in the request
   is already valid (made valid by HTAccess).
   
 */
typedef struct _HTProtocol {
        char * name;
        
        int (*load)PARAMS((HTRequest *  request));
                
        HTStream* (*saveStream)PARAMS((HTRequest *      request));

        HTStream* (*postStream)PARAMS((
                                HTRequest *     request,
                                HTParentAnchor* postTo));

} HTProtocol;

extern BOOL HTRegisterProtocol PARAMS((HTProtocol * protocol));


/*

Generate the anchor for the home page

 */

/*

   As it involves file access, this should only be done once when the program first runs.
   This is a default algorithm -- browser don't HAVE to use this.
   
 */

extern HTParentAnchor * HTHomeAnchor NOPARAMS;
/*

 */

#endif /* HTACCESS_H */
/*

   end of HTAccess  */
