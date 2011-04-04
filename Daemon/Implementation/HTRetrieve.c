/*		Handle a Retrieve request from a WWW client	HTRetrieve.c
**		===========================================
**
** Authors
**	CTB	Carl Barker, Brunel
**	DMX	Daniel Martin
**	TBL	Tim Berners-Lee, CERN, Geneva	timbl@info.cern.ch
**
** History:
**	29 Apr 91 (TBL)	Split from daemon.c
**	5 Sept 91 (DMX)	Added path simplification to prevent '..'ing to an 
**			uncorrect directory.
**			Added '\r' as space for telneting to socket.
**	10 Sep 91 (TBL)	Reject request and log if fails authorisation
**	26 Jan 92 (TBL) Added some of CTB's code for directory read.
**	23 Apr 93 (TBL) keyword untangling passed to lower level
**	31 Oct 93 (AL)	Added /htbin stuff.
**	 7 Jul 94 (FM)	Commented out dead extern declaration.
*/

/* (c) CERN WorldWideWeb project 1990,91. See Copyright.html for details */

#include <time.h>

#define USE_PLAINTEXT	/* Makes retrieval of postscript easier for now */
			/* but not good sgml */

#define BUFFER_SIZE 4096	/* Arbitrary size for efficiency */
#define INFINITY 512		/* file name length @@ FIXME */

#include "HTUtils.h"
#include "HTFormat.h"
#include "tcp.h"

#include "HTRules.h"
#include "HTParse.h"

#include "HTFile.h"
#include "HTDaemon.h"		/* calls back to HTTP daemon */
#include "HTMLGen.h"		/* For HTML generator */
#include "HTWriter.h"		/* For making streams to net and disk */

#include "HTAccess.h"
#include "HTScript.h"		/* /htbin script calls */
#include "HTAAUtil.h"		/* FREE() macro */
#include "HTRules.h"		/* HTSearchScript */
#include "HTAAServ.h"

extern int WWW_TraceFlag;	/* Control diagnostic output */
#if 0
extern FILE * logfile;			/* Log file output */
extern char HTClientHost[16];		/* Client name to be output */
#endif
extern int HTWriteASCII PARAMS((int soc, char * s));	/* In HTDaemon.c */

/* PUBLIC FILE * logfile = 0;	*/ /* Log file if any */
extern char *HTClientHost;	/* Peer internet address */





/*	Override HTML presentation method
**	---------------------------------
**
**	The "presentation" of HTML in the case of a server is
**	the generation of HTML markup.   The presence of this
**	routine prevents any of the client-oriented presentation code
**	from being picked up from the library libwww.
*/
PUBLIC HTStructured* HTML_new ARGS5(HTRequest *,	req,
				    void *,		param,
				    HTFormat,		input_format,
				    HTFormat,		output_format,
				    HTStream *,		output_stream)
{
    HTStream * markup = HTStreamStack(WWW_HTML, req, NO);
    if (!markup) return NULL;

    return HTMLGenerator(markup);
}


/*	Dummy things in hypertext object @@@@ */

PUBLIC void HText_select() {}
PUBLIC void HText_selectAnchor() {}
PUBLIC void * HTMainAnchor = NULL;



/*	Retrieve a document
**	-------------------
**
**	Application programmers redifine this function.
**
*/
PUBLIC int HTRetrieve ARGS1(HTRequest *, req)
{
    int status;

    HTImServer = HTEscape(HTReqTranslated, URL_PATH);
    HTGetAttributes(req,HTReqTranslated);
    status = HTLoadToStream(HTReqArgPath, NO, req);
    FREE(HTImServer);

    return status;
}

