/*                                                                      File access in libwww
                                       FILE ACCESS
                                             
   These are routines for local file access used by WWW browsers and servers. Implemented
   by HTFile.c. This module is a part of the CERN Common WWW Library.
   
   If the file is not a local file, then we pass it on to HTFTP in case it can be reached
   by FTP. However, as this is very time consuming when requesting a local file that
   actually doesn't exist, this redirection will be disabled in the next major release,
   www-bug@info.cern.chJune 1994.
   
   Note: All functions that deal with directory listings etc. have been moved to HTDirBrw
   Module.
   
 */

#ifndef HTFILE_H
#define HTFILE_H

#include "HTFormat.h"
#include "HTAccess.h"
#include "HTML.h"               /* SCW */
#include "HTDirBrw.h"

#ifdef SHORT_NAMES
#define HTGetCoD        HTGetContentDescription
#define HTSplFiN        HTSplitFilename
#endif /*SHORT_NAMES*/
/*

Multiformat Handling

  SPLIT FILENAME TO SUFFIXES
  
 */

PUBLIC int HTSplitFilename PARAMS((char *       s_str,
                                   char **      s_arr));

/*

  GET CONTENT DESCRIPTION ACCORDING TO SUFFIXES
  
 */

PUBLIC HTContentDescription * HTGetContentDescription PARAMS((char ** actual,
                                                              int         n));

#define MULTI_SUFFIX ".multi"   /* Extension for scanning formats */
#define MAX_SUFF 15             /* Maximum number of suffixes for a file */

/*

Convert filenames between local and WWW formats

 */
extern char * HTLocalName PARAMS((CONST char * name));


/*

Make a WWW name from a full local path name

 */
extern char * WWW_nameOfFile PARAMS((const char * name));


/*

Generate the name of a cache file

 */
extern char * HTCacheFileName PARAMS((CONST char * name));
/*

HTSetSuffix: Define the representation for a file suffix

   This defines a mapping between local file suffixes and file content types and
   encodings.
   
  ON ENTRY,
  
  suffix                  includes the "." if that is important (normally, yes!)
                         
  representation          is MIME-style content-type
                         
  encoding                is MIME-style content-transfer-encoding (8bit, 7bit, etc)
                         
  language               is MIME-style content-language
                         
  quality                 an a priori judgement of the quality of such files (0.0..1.0)
                         
 */

/*
** Example:  HTSetSuffix(".ps", "application/postscript", "8bit", NULL, 1.0);
*/

PUBLIC void HTSetSuffix PARAMS((CONST char *    suffix,
                               CONST char *     representation,
                               CONST char *     encoding,
                               CONST char *     language,
                               float            quality));

PUBLIC void HTAddType PARAMS((CONST char *      suffix,
                              CONST char *      representation,
                              CONST char *      encoding,
                              float             quality));

PUBLIC void HTAddEncoding PARAMS((CONST char *  suffix,
                                  CONST char *  encoding,
                                  float         quality));

PUBLIC void HTAddLanguage PARAMS((CONST char *  suffix,
                                  CONST char *  language,
                                  float         quality));


/*

HTFileFormat: Get Representation and Encoding from file name

  ON EXIT,
  
  return                  The represntation it imagines the file is in
                         
  *pEncoding              The encoding (binary, 7bit, etc). See HTSetSuffix .
                         
  *pLanguage              The language.
                         
 */
extern HTFormat HTFileFormat PARAMS((
                CONST char *    filename,
                HTAtom **       pEncoding,
                HTAtom **       pLanguage));


/*

Determine file value from file name

 */


extern float HTFileValue PARAMS((
                CONST char * filename));


/*

Determine write access to a file

  ON EXIT,
  
  return value            YES if file can be accessed and can be written to.
                         
 */

/*

  BUGS
  
   Isn't there a quicker way?
   
 */


extern BOOL HTEditable PARAMS((CONST char * filename));


/*

Determine a suitable suffix, given the representation

  ON ENTRY,
  
  rep                     is the atomized MIME style representation
                         
  ON EXIT,
  
  returns                 a pointer to a suitable suffix string if one has been found,
                         else NULL.
                         
 */
extern CONST char * HTFileSuffix PARAMS((
                HTAtom* rep));



/*

The Protocols

 */
GLOBALREF HTProtocol HTFTP, HTFile;

#endif /* HTFILE_H */

/*

   end of HTFile */
