
/* MODULE							HTAnnotate.c
**		HANDLE DOCUMENT LOADS
**		(WHEN WE FEEL LIKE NOT USING THE LIBWWW FUNCTIONS)
**
** AUTHORS:
**	AL	Ari Luotonen	luotonen@dxcern.cern.ch
**
** HISTORY:
**	18 Jan 94  AL	Made of 100% recycled electrons that I've
**			personally hand-picked from my garden of
**			Volkswagen spareparts.
**
** BUGS:
**	Don't believe in miracles -- rely on them!
**	So no bugs here ;-)
*/

#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef VMS
#include <types.h>
#include <stat.h>
#else
#include <sys/types.h>
#ifndef Mips
#include <sys/stat.h>
#endif
#endif

#include "HTFile.h"
#include "HTUtils.h"
#include "HTParse.h"	/* HTUnEscape() */

#include "tcp.h"
#include "HTTCP.h"

#include "HTFormat.h"	/* HTInputSocket */
#include "HTAccess.h"
#include "HTRules.h"	/* HTBinDir, HTBINDIR */
#include "HTWriter.h"
#include "HTStream.h"

#include "HTRequest.h"
#include "HTDaemon.h"


PUBLIC int HTLoadHead ARGS1(HTRequest *, req)
{
    char * headers;

    if (req && req->output_stream) {
	if (out.status_code == 200)
	    req->content_type = HTFileFormat(HTReqTranslated,
					     &req->content_encoding,
					     &req->content_language);
	headers = HTReplyHeaders(req);
	if (headers) {
	    HTLoadStrToStream(req->output_stream, headers);
	    free(headers);
	    HTCloseStream(req->output_stream);
	    return HT_LOADED;
	}
    }
    return HT_INTERNAL;
}

