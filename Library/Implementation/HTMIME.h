/*                                                 HTMIME:   Parser of MIME format for libwww
                                       MIME PARSER
                                             
   The MIME parser stream presents a MIME document. It recursively invokes the format
   manager to handle embedded formats.
   
   As well as stripping off and parsing the headers, the MIME parser has to parse any
   weirld MIME encodings it may meet within the body parts of messages, and must deal with
   multipart messages.
   
   This module is implemented to the level necessary for operation with WWW, but is not
   currently complete for any arbitrary MIME message.
   
   Check the source for latest additions to functionality.
   
   The MIME parser is complicated by the fact that WWW allows real binary to be sent, not
   ASCII encoded.  Therefore the netascii decoding is included in this module. One cannot
   layer it by converting first from Net to local text, then decoding it. Of course, for
   local files, the net ascii decoding is not needed.  There are therefore two creation
   routines.
   
 */
#ifndef HTMIME_H
#define HTMIME_H

#include "HTStream.h"
#include "HTAnchor.h"
/*

  CONVERTERS IN THIS MODULE
  
 */

#ifndef pyramid
extern HTConverter HTMIMEConvert, HTNetMIME;
#endif

#endif
/*

   End of HTMIME  */
