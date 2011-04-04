
/*
 *	IF-MODIFIED-SINCE STREAM
 *
 *	Checks the MIME headers of an HTTP response and modifies
 *	it into 304 Not modified response if the Last-Modified
 *	field doesn't give a more recent date and time than
 *	what is in HTIfModifiedSince variable.
 *
 * Author:
 *	Ari Luotonen, 1994, luotonen@www.cern.ch
 *
 * HISTORY:
 *	 8 Jul 94  FM	Insulate free() from _free structure element.
 */

#include <time.h>

#include "HTims.h"
#include "HTSUtils.h"
#include "HTDaemon.h"

#define IMS_BUF_SIZE 8096

typedef enum {
    IMS_EOL = 0,
    IMS_LINE,
    IMS_BODY,
    IMS_JUNK
} IMS_State;

struct _HTStream {
    HTStreamClass *	isa;
    HTStream *		sink;
    IMS_State		state;
    time_t		limit;
    char		buffer[IMS_BUF_SIZE];
    int			index;
};

PRIVATE void IMS_decide ARGS1(HTStream *, me)
{
    char * mime = strchr(me->buffer,LF);

    if (mime && (int)strlen(mime) > 20) {
	char * search = "deifidom-tsal";	/* last-modified backwards */
	char * p = strchr(mime+12, ':');

	while (p) {
	    char * p1 = search;
	    char * p2 = p-1;
	    for ( ; *p1 && TOLOWER(*p2)==*p1; p1++, p2--);
	    if (!*p1 && (p2<me->buffer || *p2==LF)) {
		time_t t = parse_http_time(p+1);
		if (t && t <= me->limit) {
		    me->state = IMS_JUNK;
		    out.status_code = 304;
		    (*me->sink->isa->put_string)(me->sink,
					     "HTTP/1.0 304 Not modified\r\n");
		    (*me->sink->isa->put_string)(me->sink,mime+1);
		    return;
		}
	    }
	    p = strchr(p+1,':');
	}
    }
    me->state = IMS_BODY;
    (*me->sink->isa->put_block)(me->sink, me->buffer, me->index);
}

PRIVATE void IMS_put_char ARGS2(HTStream *, me, char, c)
{
    if (me->state == IMS_JUNK)
	return;
    if (me->state == IMS_BODY) {
	(*me->sink->isa->put_character)(me->sink,c);
	return;
    }
    me->buffer[me->index++] = c;
    if (c==LF) {
	if (me->state == IMS_EOL) {
	    me->buffer[me->index] = 0;
	    IMS_decide(me);
	}
	else me->state = IMS_EOL;
    }
    else if (c!=CR)
	me->state = IMS_LINE;
}

PRIVATE void IMS_put_block ARGS3(HTStream *, me, CONST char *, b, int, l)
{
    if (!b || me->state==IMS_JUNK)
	return;

    if (me->state != IMS_BODY) {
	while (me->state!=IMS_BODY && me->state!=IMS_JUNK && l-- > 0) {
	    IMS_put_char(me,*b);
	    b++;
	}
    }
    if (me->state == IMS_BODY) {
	(*me->sink->isa->put_block)(me->sink,b,l);
	return;
    }
}

PRIVATE void IMS_put_string ARGS2(HTStream *, me, CONST char*, s)
{
    int len = s ? strlen(s) : 0;
    if (len > 0)
	IMS_put_block(me,s,len);
}

PRIVATE void IMS_free ARGS1(HTStream *, me)
{
    (*me->sink->isa->_free)(me->sink);
    if (me->state == IMS_JUNK) {
	/*
	 * Make sure that the log shows zero bytes transferred.
	 */
	out.content_length = 0;
    }
    free(me);
}

PRIVATE void IMS_abort ARGS2(HTStream *, me, HTError, e)
{
    (*me->sink->isa->abort)(me->sink,e);
    free(me);
}


/*
 *	IMS stream class
 */
PRIVATE HTStreamClass HTIMSStreamClass =
{
    "IfModifiedSinceStream",
    IMS_free,
    IMS_abort,
    IMS_put_char,
    IMS_put_string,
    IMS_put_block
};

PUBLIC HTStream * HTIfModSinceStream ARGS2(time_t,	limit,
					   HTStream *,	sink)
{
    HTStream * me = (HTStream*)calloc(1,sizeof(HTStream));
    if (!me) outofmem(__FILE__, "HTIfModSinceStream");

    me->isa = &HTIMSStreamClass;
    me->limit = limit;
    me->sink = sink;
    return me;
}

