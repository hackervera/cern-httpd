/*                                                                             Script Manager
                                      SCRIPT MANAGER
                                             
 */
/*
**      (c) COPYRIGHT MIT 1995.
**      Please first read the full copyright statement in the file COPYRIGH.
*/
/*

 */
#ifndef HTSCRIPT_H
#define HTSCRIPT_H

#include "HTUtils.h"

/* PUBLIC                                                       HTCallScript()
**              CALL A SCRIPT AND SEND RESULTS BACK TO CLIENT
** ON ENTRY:
**      req             the request.
**      req->script     script to call.
**      req->script_pathinfo
**                      path info after script name part in URL.
**      req->arg_keywords
**                      search keywords/form fields
**
**                              /htbin/foo/bar/x/y
**
**                      is called as:
**
**                          /foo /bar/x/y keywords...
**
**                      and:
**                              /htbin/foo
**
**                      is called as:
**
**                          /foo '' keywords...
**
** ON EXIT:
**      returns         HT_LOADED on success.
*/
PUBLIC int HTCallScript PARAMS((HTRequest *     req));
#endif
/*

   End of declaration module  */
