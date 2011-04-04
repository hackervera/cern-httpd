/*                                                                            Icon Management
                                     ICON MANAGEMENT
                                             
 */

#ifndef HTICONS_H
#define HTICONS_H
#include "HTFormat.h"

#ifdef ISC3             /* Lauren */
typedef int mode_t;
#endif

/*

Icons

   Icons are bound to MIME content-types and encoding.  These functions bind icon URLs to
   given content-type or encoding templates.  Templates containing a slash are taken to be
   content-type templates, other are encoding templates.
   
   
   
Controlling globals

  SHOW BRACKETS AROUND ALTERNATIVE TEXT
  
   By default alternative text is bracketed by square brackets (the ALT tag to IMG
   element).  Setting the global HTDirShowBrackets to false will turn this feature off.
   
 */

typedef struct _HTIconNode {
    char *      icon_url;
    char *      icon_alt;
    char *      type_templ;
} HTIconNode;

/*
 * The list element definition to bind a CGI to a filetyp for special
 * presentation like looking in an archiv (AddHref /cgi-bin/unarch? .zip .tar)
 */
typedef struct _HTHrefNode {
    char *      href_url;
    char *      type_templ;
} HTHrefNode;


/*

 */

extern BOOL HTDirShowBrackets;
extern HTIconNode * icon_unknown;               /* Unknown file type */
extern HTIconNode * icon_blank;                 /* Blank icon in heading */
extern HTIconNode * icon_parent;                /* Parent directory icon */
extern HTIconNode * icon_dir;                   /* Directory icon */
/*

Public functions

   All of these functions take an absolute URL and alternate text to use.
   
 */

/* Generates the alt-tag */
PUBLIC char * HTIcon_alt_string PARAMS((char * alt,
                                        BOOL   brackets));

/*
 * General icon binding.  Use this icon if content-type or encoding
 * matches template.
 */
PUBLIC void HTAddIcon PARAMS((char *    url,
                              char *    alt,
                              char *    type_templ));


/*
 * Called from HTConfig.c to build the list of all the AddHref's
 */
PUBLIC void HTAddHref PARAMS((char *    url,
                              char *    type_templ));


/*
 * Icon for which no other icon can be used.
 */
PUBLIC void HTAddUnknownIcon PARAMS((char * url,
                                     char * alt));

/*
 * Invisible icon for the listing header field to make it aligned
 * with the rest of the listing (this doesn't have to be blank).
 */
PUBLIC void HTAddBlankIcon PARAMS((char * url,
                                   char * alt));

/*
 * Icon to use for parent directory.
 */
PUBLIC void HTAddParentIcon PARAMS((char * url,
                                    char * alt));

/*
 * Icon to use for a directory.
 */
PUBLIC void HTAddDirIcon PARAMS((char * url,
                                 char * alt));

/*                                                               HTGetIcon()
** returns the icon corresponding to content_type or content_encoding.
*/
PUBLIC HTIconNode * HTGetIcon PARAMS((mode_t    mode,
                                      HTFormat  content_type,
                                      HTFormat  content_encoding));

/*
 * returns the HrefNode to get the URL for presentation of a file (indexing)
 */
PUBLIC HTHrefNode * HTGetHref PARAMS(( char *  filename));


/*

    A Predifined Set of Icons
    
   The function HTStdIconInit(url_prefix) can be used to initialize a standard icon set:
   
       blank.xbm for the blank icon
      
       directory.xbm for directory icon
      
       back.xbm for parent directory
      
       unknown.xbm for unknown icon
      
       binary.xbm for binary files
      
       text.xbm for ascii files
      
       image.xbm for image files
      
       movie.xbm for video files
      
       sound.xbm for audio files
      
       tar.xbm for tar and gtar files
      
       compressed.xbm for compressed and gzipped files
      
 */

PUBLIC void HTStdIconInit PARAMS((CONST char * url_prefix));
/*

 */

#endif /* HTICONS */
/*

   end  */
