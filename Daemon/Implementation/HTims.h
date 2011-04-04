/*                                                               Stream for If-Modified-Since
                               STREAM FOR IF-MODIFIED-SINCE
                                             
 */
/*
**      (c) COPYRIGHT MIT 1995.
**      Please first read the full copyright statement in the file COPYRIGH.
*/
/*

   Stream for making sure that If-Modified-Since request is handled correctly.
   
 */
#ifndef HT_IMS_H
#define HT_IMS_H

#include "HTUtils.h"
#include "tcp.h"
#include "HTStream.h"

PUBLIC HTStream * HTIfModSinceStream PARAMS((time_t     limit,
                                             HTStream * sink));

#endif /* HT_IMS_H */
/*

   End of declaration module  */
