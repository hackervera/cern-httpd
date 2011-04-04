/*                                                              HTTee:  Tee stream for libwww
                                        TEE STREAM
                                             
   This is part of libwww .  The Tee stream just writes everything you put into it into
   two oter streams.
   
   One use (the only use?!) is for taking a cached copey on disk while loading the main
   copy, without having to wait for the disk copy to be finished and reread it.
   
 */
#include "HTStream.h"

extern HTStream * HTTee PARAMS((HTStream* s1, HTStream* s2));





/*

                                                                                    Tim BL
                                                                                          
    */
