/*                                                  HTGuess: Guess content-type from a stream
                                   CONTENT-TYPE GUESSER
                                             
   This stream is a one that reads first a chunk of stuff, tries to figure out the format,
   and calls HTStreamStack().  This is a kind of lazy-evaluation of HTStreamStack().
   
   This could be extended arbitrarily to recognize all the possible file formats in the
   world, if someone only had time to do it.
   
   Part of libwww. Implemented by HTGuess.c.
   
 */

#ifndef HTGUESS_H
#define HTGUESS_H

#include "HTStream.h"
#include <stdio.h>
#include "HTFormat.h"

#ifdef SHORT_NAMES
#define HTGuess_new     HTGuessN
#endif

PUBLIC HTStream * HTGuess_new PARAMS((HTRequest * req));


#endif  /* !HTGUESS_H */

/*

   End of file HTGuess.h.  */
