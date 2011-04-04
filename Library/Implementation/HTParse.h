/*                                                   HTParse:  URI parsing in the WWW Library
                                         HTPARSE
                                             
   This module is a part of the CERN Common WWW Library. It contains code to parse URIs
   and various related things such as:
   
       Parse a URI relative to another URI
      
       Get the URI relative to another URI
      
       Remove redundant data from the URI (HTSimplify and HTCanon)
      
       Expand a local host name into a full domain name (HTExpand)
      
       Search a URI for illigal characters in order to prevent security holes
      
       Escape and Unescape a URI for reserved characters in URLs
      
   Implemented by HTParse.c.
   
 */

#ifndef HTPARSE_H
#define HTPARSE_H
#include "HTUtils.h"
/*

HTParse:  Parse a URI relative to another URI

   This returns those parts of a name which are given (and requested) substituting bits
   from the related name where necessary.
   
  CONTROL FLAGS FOR HTPARSE
  
   The following are flag bits which may be ORed together to form a number to give the
   'wanted' argument to HTParse.
   
 */

#define PARSE_ACCESS            16
#define PARSE_HOST               8
#define PARSE_PATH               4
#define PARSE_ANCHOR             2
#define PARSE_PUNCTUATION        1
#define PARSE_ALL               31
/*

  ON ENTRY
  
  aName                   A filename given
                         
  relatedName             A name relative to which aName is to be parsed
                         
  wanted                  A mask for the bits which are wanted.
                         
  ON EXIT,
  
  returns                 A pointer to a malloc'd string which MUST BE FREED
                         
 */

PUBLIC char * HTParse  PARAMS(( const char * aName,
                                const char * relatedName,
                                int wanted));
/*

HTStrip: Strip white space off a string

  ON EXIT
  
   Return value points to first non-white character, or to 0 if none.
   
   All trailing white space is OVERWRITTEN with zero.
   
 */
#ifdef __STDC__
extern char * HTStrip(char * s);
#else
extern char * HTStrip();
#endif
/*

HTSimplify: Simplify a UTL

   A URI is allowed to contain the seqeunce xxx/../ which may be replaced by "" , and the
   seqeunce "/./" which may be replaced by "/". Simplification helps us recognize
   duplicate URIs. Thus, the following transformations are done:
   
       /etc/junk/../fred
      becomes
      /etc/fred
      
       /etc/junk/./fred
      becomes
      /etc/junk/fred
      
   but we should NOT change
   
       http://fred.xxx.edu/../.. or
      
       ../../albert.html
      
   In the same manner, the following prefixed are preserved:
   
       ./
      
       //
      
   In order to avoid empty URIs the following URIs become:
   
       /fred/..
      
      becomes /fred/..
      
       /fred/././..
      
      becomes /fred/..
      
       /fred/.././junk/.././
      becomes /fred/..
      
   If more than one set of `://' is found (several proxies in cascade) then only the part
   after the last `://' is simplified.
   
 */

PUBLIC char *HTSimplify PARAMS((char *filename));
/*

HTRelative:  Make Relative (Partial) URI

   This function creates and returns a string which gives an expression of one address as
   related to another. Where there is no relation, an absolute address is retured.
   
  ON ENTRY,
  
   Both names must be absolute, fully qualified names of nodes (no anchor bits)
   
  ON EXIT,
  
   The return result points to a newly allocated name which, if parsed by HTParse relative
   to relatedName, will yield aName. The caller is responsible for freeing the resulting
   name later.
   
 */
#ifdef __STDC__
extern char * HTRelative(const char * aName, const char *relatedName);
#else
extern char * HTRelative();
#endif
/*

HTExpand: Expand a Local Host Name Into a Full Domain Name

   This function expands the host name of the URI from a local name to a full domain name,
   converts the host name to lower case and takes away `:80', `:70' and `:21'. The
   advantage by doing this is that we only have one entry in the host case and one entry
   in the document cache.
   
 */

PUBLIC char *HTCanon PARAMS ((  char ** filename,
                                char *  host));
/*

HTEscape: Encode unacceptable characters in string

   This funtion takes a string containing any sequence of ASCII characters, and returns a
   malloced string containing the same infromation but with all "unacceptable" characters
   represented in the form %xy where X and Y are two hex digits.
   
 */

PUBLIC char * HTEscape PARAMS((CONST char * str, unsigned char mask));
/*

   The following are valid mask values. The terms are the BNF names in the URI document.
   
 */

#define URL_XALPHAS     (unsigned char) 1
#define URL_XPALPHAS    (unsigned char) 2
#define URL_PATH        (unsigned char) 4
/*

HTUnEscape: Decode %xx escaped characters

   This function takes a pointer to a string in which character smay have been encoded in
   %xy form, where xy is the acsii hex code for character 16x+y. The string is converted
   in place, as it will never grow.
   
 */

extern char * HTUnEscape PARAMS(( char * str));
/*

Prevent Security Holes

   HTCleanTelnetString() makes sure that the given string doesn't contain characters that
   could cause security holes, such as newlines in ftp, gopher, news or telnet URLs; more
   specifically: allows everything between hexadesimal ASCII 20-7E, and also A0-FE,
   inclusive.
   
  str                     the string that is *modified* if necessary.  The string will be
                             truncated at the first illegal character that is encountered.
                         
  returns                 YES, if the string was modified.      NO, otherwise.
                         
 */

PUBLIC BOOL HTCleanTelnetString PARAMS((char * str));
/*

 */

#endif  /* HTPARSE_H */
/*

   End of HTParse Module  */
