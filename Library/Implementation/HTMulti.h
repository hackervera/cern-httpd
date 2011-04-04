/*                                                             Multiformat handling in libwww
                                   MULTIFORMAT HANDLING
                                             
 */

#ifndef HTMULTI_H
#define HTMULTI_H

#include "HTFile.h"

/*
**      Set default file name for welcome page on each directory.
*/
PUBLIC void HTAddWelcome PARAMS((char * welcome_name));

/*
**      Perform multiformat handling
*/
PUBLIC char * HTMulti PARAMS((HTRequest *       req,
                              char *            path,
                              struct stat *     stat_info));

#endif /* HTMULTI_H */

/*

   End of HTMulti.  */
