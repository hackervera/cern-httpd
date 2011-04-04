/*                                            HTFormat: The format manager in the WWW Library
                            MANAGE DIFFERENT DOCUMENT FORMATS
                                             
   Here we describe the functions of the HTFormat module which handles conversion between
   different data representations.  (In MIME parlance, a representation is known as a
   content- type. In WWW  the term "format" is often used as it is shorter).
   
   This module is implemented by HTFormat.c. The module is a part of the WWW library.
   
Preamble

 */
#ifndef HTFORMAT_H
#define HTFORMAT_H

#include "HTUtils.h"
#include "HTStream.h"
#include "HTAtom.h"
#include "HTList.h"

#ifdef SHORT_NAMES
#define HTOutputSource HTOuSour
#define HTOutputBinary HTOuBina
#endif


typedef struct _HTContentDescription {
    char *      filename;
    HTAtom *    content_type;
    HTAtom *    content_language;
    HTAtom *    content_encoding;
    int         content_length;
    float       quality;
} HTContentDescription;

PUBLIC void HTAcceptEncoding PARAMS((HTList *   list,
                                     char *     enc,
                                     float      quality));

PUBLIC void HTAcceptLanguage PARAMS((HTList *   list,
                                     char *     lang,
                                     float      quality));

PUBLIC BOOL HTRank PARAMS((HTList * possibilities,
                           HTList * accepted_content_types,
                           HTList * accepted_content_languages,
                           HTList * accepted_content_encodings));


/*

HT Input Socket: Buffering for network in

   This routines provide simple character input from sockets. These are used for parsing
   input in arbitrary IP protocols (Gopher, NNTP, FTP).
   
 */
#define INPUT_BUFFER_SIZE 4096          /* Tradeoff spped vs memory*/
typedef struct _socket_buffer {
        char input_buffer[INPUT_BUFFER_SIZE];
        char * input_pointer;
        char * input_limit;
        int input_file_number;
        BOOL    s_do_buffering;
        char *  s_buffer;
        int     s_buffer_size;
        char *  s_buffer_cur;
} HTInputSocket;

/*

  CREATE INPUT BUFFER AND SET FILE NUMBER
  
 */
extern HTInputSocket* HTInputSocket_new PARAMS((int file_number));

/*

  GET NEXT CHARACTER FROM BUFFER
  
 */
extern int HTInputSocket_getCharacter PARAMS((HTInputSocket* isoc));

/*

  READ BLOCK FROM INPUT SOCKET
  
   Read *len characters and return a buffer (don't free) containing *len characters ( *len
   may have changed). Buffer is not NULL-terminated.
   
 */
extern char * HTInputSocket_getBlock PARAMS((HTInputSocket * isoc,
                                                  int *           len));

/*

  FREE INPUT SOCKET BUFFER
  
 */
extern void HTInputSocket_free PARAMS((HTInputSocket * isoc));


PUBLIC char * HTInputSocket_getLine PARAMS((HTInputSocket * isoc));
PUBLIC char * HTInputSocket_getUnfoldedLine PARAMS((HTInputSocket * isoc));
PUBLIC char * HTInputSocket_getStatusLine PARAMS((HTInputSocket * isoc));
PUBLIC BOOL   HTInputSocket_seemsBinary PARAMS((HTInputSocket * isoc));

/*

  SECURITY BUFFERING
  
   When it's necessary to get e.g. the header section, or part of it, exactly as it came
   from the client to calculate the message digest, these functions turn buffering on and
   off.  All the material returned by HTInputSocket_getStatusLine(),
   HTInputSocket_getUnfoldedLine() and HTInputSocket_getLine() gets buffered after a call
   to HTInputSocket_startBuffering() until either HTInputSocket_stopBuffering() is called,
   or an empty line is returned by any of these functions (end of body section).
   HTInputSocket_getBuffer() returns the number of characters buffered, and sets the given
   buffer pointer to point to internal buffer.  This buffer exists until
   HTInputSocketobject is freed.
   
 */


PUBLIC void HTInputSocket_startBuffering PARAMS((HTInputSocket * isoc));
PUBLIC void HTInputSocket_stopBuffering PARAMS((HTInputSocket * isoc));
PUBLIC int HTInputSocket_getBuffer PARAMS((HTInputSocket * isoc,
                                           char ** buffer_ptr));
/*

The HTFormat type

   We use the HTAtom object for holding representations. This allows faster manipulation
   (comparison and copying) that if we stayed with strings.
   
   The following have to be defined in advance of the other include files because of
   circular references.
   
 */
typedef HTAtom * HTFormat;

#include "HTAccess.h"   /* Required for HTRequest definition */
                
/*

   These macros (which used to be constants) define some basic internally referenced
   representations.
   
  INTERNAL ONES
  
   The www/xxx ones are of course not MIME standard.
   
   star/star is an output format which leaves the input untouched. It is useful for
   diagnostics, and for users who want to see the original, whatever it is.
   
 */

#define WWW_SOURCE      HTAtom_for("*/*")      /* Whatever it was originally */
/*

   www/present represents the user's perception of the document.  If you convert to
   www/present, you present the material to the user.
   
 */

#define WWW_PRESENT     HTAtom_for("www/present")   /* The user's perception */
/*

   The message/rfc822 format means a MIME message or a plain text message with no MIME
   header. This is what is returned by an HTTP server.
   
 */

#define WWW_MIME        HTAtom_for("www/mime")             /* A MIME message */
/*

   www/print is like www/present except it represents a printed copy.
   
 */

#define WWW_PRINT       HTAtom_for("www/print")            /* A printed copy */
/*

   www/unknown is a really unknown type.  Some default action is appropriate.
   
 */

#define WWW_UNKNOWN     HTAtom_for("www/unknown")
/*

  MIME ONES (A FEW)
  
   These are regular MIME types.  HTML is assumed to be added by the W3 code.
   application/octet-stream was mistakenly application/binary in earlier libwww versions
   (pre 2.11).
   
 */

#define WWW_PLAINTEXT   HTAtom_for("text/plain")
#define WWW_POSTSCRIPT  HTAtom_for("application/postscript")
#define WWW_RICHTEXT    HTAtom_for("application/rtf")
#define WWW_AUDIO       HTAtom_for("audio/basic")
#define WWW_HTML        HTAtom_for("text/html")
#define WWW_BINARY      HTAtom_for("application/octet-stream")
#define WWW_VIDEO       HTAtom_for("video/mpeg")
/*

   Extra types used in the library
   
 */

#define WWW_NEWSLIST    HTAtom_for("text/newslist")
/*

   We must include the following file after defining HTFormat, to which it makes
   reference.
   
The HTEncoding type

 */
typedef HTAtom* HTEncoding;

/*

   The following are values for the MIME types:
   
 */
#define WWW_ENC_7BIT            HTAtom_for("7bit")
#define WWW_ENC_8BIT            HTAtom_for("8bit")
#define WWW_ENC_BINARY          HTAtom_for("binary")

/*

   We also add
   
 */
#define WWW_ENC_COMPRESS        HTAtom_for("compress")

#include "HTAnchor.h"

/*

The HTPresentation and HTConverter types

   This HTPresentation structure represents a possible conversion algorithm from one
   format to annother.  It includes a pointer to a conversion routine.  The conversion
   routine returns a stream to which data should be fed.  See also HTStreamStack which
   scans the list of registered converters and calls one. See the initialisation module
   for a list of conversion routines.
   
 */

typedef struct _HTPresentation HTPresentation;

typedef HTStream * HTConverter PARAMS((
        HTRequest *             request,
        void *                  param,
        HTFormat                input_format,
        HTFormat                output_format,
        HTStream *              output_stream));
        
struct _HTPresentation {
        HTAtom* rep;            /* representation name atomized */
        HTAtom* rep_out;        /* resulting representation */
        HTConverter *converter; /* The routine to gen the stream stack */
        char *  command;        /* MIME-format string */
        float   quality;        /* Between 0 (bad) and 1 (good) */
        float   secs;
        float   secs_per_byte;
};
/*

   A global list of converters is kept by this module.  It is also scanned by modules
   which want to know the set of formats supported. for example.  Note there is also an
   additional list associated with each request.
   
 */
extern HTList * HTConversions ;


/*

HTSetPresentation: Register a system command to present a format

  ON ENTRY,
  
  rep                     is the MIME - style format name
                         
  command                 is the MAILCAP - style command template
                         
  quality                 A degradation faction 0..1
                         
  maxbytes                A limit on the length acceptable as input (0 infinite)
                         
  maxsecs                 A limit on the time user will wait (0 for infinity)
                         
 */
extern void HTSetPresentation PARAMS((
        HTList *        conversions,
        CONST char *    representation,
        CONST char *    command,
        float           quality,
        float           secs,
        float           secs_per_byte
));


/*

HTSetConversion:   Register a converstion routine

  ON ENTRY,
  
  rep_in                  is the content-type input
                         
  rep_out                 is the resulting content-type
                         
  converter               is the routine to make the stream to do it
                         
 */

extern void HTSetConversion PARAMS((
        HTList *        conversions,
        CONST char *    rep_in,
        CONST char *    rep_out,
        HTConverter *   converter,
        float           quality,
        float           secs,
        float           secs_per_byte
));


/*

HTStreamStack:   Create a stack of streams

   This is the routine which actually sets up the conversion. It currently checks only for
   direct conversions, but multi-stage conversions are forseen. It takes a stream into
   which the output should be sent in the final format, builds the conversion stack, and
   returns a stream into which the data in the input format should be fed.  The anchor is
   passed because hypertxet objects load information into the anchor object which
   represents them.
   
   If guess is true and input format is www/unknown, try to guess the format by looking at
   the first few butes of the stream.
   
 */
extern HTStream * HTStreamStack PARAMS((
        HTFormat                format_in,
        HTRequest *             request,
        BOOL                    guess));

/*

HTStackValue: Find the cost of a filter stack

   Must return the cost of the same stack which HTStreamStack would set up.
   
  ON ENTRY,
  
  format_in               The fomat of the data to be converted
                         
  format_out              The format required
                         
  initial_value           The intrinsic "value" of the data before conversion on a scale
                         from 0 to 1
                         
  length                  The number of bytes expected in the input format
                         
 */
extern float HTStackValue PARAMS((
        HTList *                conversions,
        HTFormat                format_in,
        HTFormat                format_out,
        float                   initial_value,
        long int                length));

#define NO_VALUE_FOUND  -1e20           /* returned if none found */

/*

HTCopy:  Copy a socket to a stream

   This is used by the protocol engines to send data down a stream, typically one which
   has been generated by HTStreamStack. Returns the number of bytes transferred.
   
 */
extern int HTCopy PARAMS((
        int                     file_number,
        HTStream*               sink));

        
/*

HTFileCopy:  Copy a file to a stream

   This is used by the protocol engines to send data down a stream, typically one which
   has been generated by HTStreamStack. It is currently called by HTParseFile
   
 */
extern void HTFileCopy PARAMS((
        FILE*                   fp,
        HTStream*               sink));

        
/*

HTCopyNoCR: Copy a socket to a stream, stripping CR characters.

   It is slower than HTCopy .
   
 */

extern void HTCopyNoCR PARAMS((
        int                     file_number,
        HTStream*               sink));


/*

HTParseSocket: Parse a socket given its format

   This routine is called by protocol modules to load an object.  uses HTStreamStack and
   the copy routines above.  Returns HT_LOADED if succesful, <0 if not.
   
 */
extern int HTParseSocket PARAMS((
        HTFormat        format_in,
        int             file_number,
        HTRequest *     request));

/*

HTParseFile: Parse a File through a file pointer

   This routine is called by protocols modules to load an object. uses HTStreamStack and
   HTFileCopy .  Returns HT_LOADED if succesful, <0 if not.
   
 */
extern int HTParseFile PARAMS((
        HTFormat        format_in,
        FILE            *fp,
        HTRequest *     request));

/*

HTNetToText: Convert Net ASCII to local representation

   This is a filter stream suitable for taking text from a socket and passing it into a
   stream which expects text in the local C representation. It does ASCII and newline
   conversion. As usual, pass its output stream to it when creating it.
   
 */
extern HTStream *  HTNetToText PARAMS ((HTStream * sink));

/*

HTFormatInit: Set up default presentations and conversions

   These are defined in HTInit.c or HTSInit.c if these have been replaced. If you don't
   call this routine, and you don't define any presentations, then this routine will
   automatically be called the first time a conversion is needed. However, if you
   explicitly add some conversions (eg using HTLoadRules) then you may want also to
   explicitly call this to get the defaults as well.
   
 */

extern void HTFormatInit PARAMS((HTList * conversions));

/*

HTFormatInitNIM: Set up default presentations and conversions

   This is a slightly different version of HTFormatInit, but without any conversions that
   might use third party programs. This is intended for Non Interactive Mode.
   
 */
extern void HTFormatInitNIM PARAMS((HTList * conversions));

/*

HTFormatDelete: Remove presentations and conversions

   Deletes the list from HTFormatInit or HTFormatInitNIM
   
 */
extern void HTFormatDelete PARAMS((HTList * conversions));

/*

Epilogue

 */
extern BOOL HTOutputSource;     /* Flag: shortcut parser */
#endif

/*

   end */
