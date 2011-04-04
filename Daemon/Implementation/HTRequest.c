
/* MODULE						HTRequest.c
**		HTTP REQUEST PARSE AND TRANSLATION
**
** AUTHORS:
**	AL	Ari Luotonen    luotonen@dxcern.cern.ch
**	MD	Mark Donszelmann    duns@vxdeop.cern.ch
**
** HISTORY:
**	11 Dec 93  AL	Written based on the old HTHandle().
**	 8 Jul 94  FM	Insulate free() from _free structure element.
**			Replaced free() with FREE() is some places where
**			the pointer might already have been freed.
*/

#include <string.h>
#include <stdio.h>
#include <time.h>

#include "HTUtils.h"
#include "HTAccess.h"	/* HTRequest */
#include "HTParse.h"	/* HTStrip() */
#include "tcp.h"
#include "HTRequest.h"	/* Implemented here */
#include "HTDaemon.h"
#include "HTConfig.h"
#include "HTTCP.h"	/* HTHostName() */
#include "HTAuth.h"

#define HTTP_VERSION	"1.0"

extern char * HTAppName;
extern char * HTAppVersion;
extern char * HTClientProtocol;
extern char * HTServerProtocol;
extern char * HTClientHost;

PRIVATE HTList * hbuf_head = NULL;
PRIVATE HTList * hbuf_tail = NULL;
PRIVATE int	 hbuf_total_len = 0;

typedef struct _HT_hbuf {
    char *	line;
    BOOL	proxy_header;
    BOOL	cgi_header;
} HT_hbuf;


#define SPACE(c) ((c==' ')||(c=='\t')||(c=='\n')||(c=='\r')) 	/*  DMX */


PRIVATE void hbuf_append ARGS1(char *, line)
{
    if (line && *line) {
	HT_hbuf * node = (HT_hbuf*)calloc(1,sizeof(HT_hbuf));
	if (!node) outofmem(__FILE__, "hbuf_append");

	StrAllocCopy(node->line, line);
	node->proxy_header = YES;
	node->cgi_header = YES;

	if (!hbuf_head) {
	    hbuf_tail = hbuf_head = HTList_new();
	    hbuf_total_len = 0;
	}
	hbuf_tail->next = (HTList*)malloc(sizeof(HTList));
	hbuf_tail = hbuf_tail->next;
	hbuf_tail->object = (void*)node;
	hbuf_tail->next = NULL;
	hbuf_total_len += strlen(line) + 3;
    }
}


PRIVATE void hbuf_proxy_cancel_last NOARGS
{
    if (hbuf_tail) {
	HT_hbuf * node = (HT_hbuf*)(hbuf_tail->object);
	if (node)
	    node->proxy_header = NO;
    }
}


PRIVATE void hbuf_cgi_cancel_last NOARGS
{
    if (hbuf_tail) {
	HT_hbuf * node = (HT_hbuf*)(hbuf_tail->object);
	if (node)
	    node->cgi_header = NO;
    }
}


PUBLIC char * hbuf_proxy_headers ARGS1(HTRequest *, req)
{
    char * ret;
    char * p;
    HT_hbuf * node;
    HTList * cur = hbuf_head;
    int extra = 0;

#if 0
    if (!hbuf_head || !hbuf_total_len)
	return NULL;
#endif

    if (req && HTUserAgent)
	extra = strlen(HTUserAgent);
    if (req && HTIfModifiedSince)
	extra += 60;
    p = ret = (char*)malloc(hbuf_total_len + extra + 100);

    if (HTIfModifiedSince) {
	sprintf(p,"If-Modified-Since: %s%c%c",
		http_time(&HTIfModifiedSince),CR,LF);
	p += strlen(p);
    }

    while ((node = (HT_hbuf*)HTList_nextObject(cur))) {
	if (node->line && node->proxy_header) {
	    char * line = node->line;
	    while (*line)
		*p++ = *line++;
	    *p++ = CR;
	    *p++ = LF;
	}
    }
    *p = 0;

#if 0
    sprintf(p, "User-Agent: %s%s%s/%s libwww/%s%c%c",
	    HTUserAgent		? HTUserAgent		: "",
	    HTUserAgent		? "  via proxy gateway  " : "Proxy gateway ",
	    HTAppName		? HTAppName		: "UnKnown",
	    HTAppVersion	? HTAppVersion		: "0.0",
	    HTLibraryVersion	? HTLibraryVersion	: "0.0",
	    CR, LF);
#else
    sprintf(p, "User-Agent: %s%c%cVia: %s %s (%s/%s)%c%c",
	    HTUserAgent	? HTUserAgent : "",
	    CR, LF,
	    HTTP_VERSION,
	    "Nobody",
	    HTAppName ? HTAppName : "UnKnown",
	    HTAppVersion ? HTAppVersion : "0.0",
	    CR, LF);
#endif

    return ret;
}


PRIVATE char * do_it_all ARGS4(char *,		env_name,
			       HTList *,	parts,
			       int,		len,
			       char **,		cur)
{
    char * result = NULL;

    if (parts && parts->next) {
	int mylen = 0;
	char * p = (char*)parts->next->object;
	p = strchr(p,':');
	if (p) {
	    p++;
	    while (*p && WHITE(*p)) p++;
	    mylen = strlen(p);
	    len += 2 + mylen;
	}
	result = do_it_all(env_name, parts->next, len, cur);
	if (p) {
	    strcpy(*cur,p);
	    *cur += mylen;
	    if (parts->object) {
		strcpy(*cur, ", ");
		*cur += 2;
	    }
	}
    }
    else if (parts) {
	result = (char*)malloc(len + 20);
	if (!result) outofmem(__FILE__, "do_it_all");

	strcpy(result, env_name);
	*cur = result + strlen(env_name);
    }
    return result;
}



PUBLIC HTList * hbuf_http_env_vars ARGS1(HTRequest *, req)
{
    HTList * result = HTList_new();
    HTList * cur1;

    if (!hbuf_head || !hbuf_head->next)
	return result;

    cur1 = hbuf_head->next;
    while (cur1) {
	HT_hbuf * node1 = (HT_hbuf*)cur1->object;
	char * colon;

	if (node1 && node1->cgi_header && node1->line &&
	    (colon = strchr(node1->line,':'))) {

	    HTList * parts = HTList_new();
	    char * name = NULL;
	    int len;

	    /* get header field name */
	    *colon = 0;
	    StrAllocCopy(name, node1->line);
	    StrAllocCat(name, ":");
	    len = strlen(name);
	    *colon = ':';

	    {	/* collect all the fields that have the same name */
		HTList * cur2 = cur1;
		while (cur2) {
		    HT_hbuf * node2 = (HT_hbuf*)cur2->object;
		    if (node2->cgi_header &&
			!strncasecomp(node2->line,name,len)) {
			node2->cgi_header = NO;
			HTList_addObject(parts, (void*)node2->line);
		    }
		    cur2 = cur2->next;
		}
	    }

	    {	/* make environment variable name */
		char * cur = name;
		char * envname = NULL;

		while (*cur && *cur!=':') {
		    if (*cur=='-') *cur = '_';
		    else *cur = TOUPPER(*cur);
		    cur++;
		}
		if (*cur) *cur = '=';
		StrAllocCopy(envname, "HTTP_");
		StrAllocCat(envname, name);
		free(name);
		name = envname;
	    }

	    {
		/* collapse headers creating a commaseparated list  */
		/* which is an env.var.declaration at the same time */

		char * foo = NULL;
		char * envvar = do_it_all(name, parts, strlen(name), &foo);

		if (HTUser && HTUser->username &&
		    !strncmp(envvar,"HTTP_AUTHORIZATION=",19)) {
		    CTRACE(stderr,
		    "Not......... propagating HTTP_AUTHORIZATION to child\n");
		    FREE(envvar);
		}
		else {
		    HTList_addObject(result, envvar);
		}
		free(name);
		HTList_delete(parts);
	    }
	}
	cur1 = cur1->next;
    }
    return result;
}



PRIVATE void hbuf_free NOARGS
{
    HT_hbuf * node;
    HTList * killme;
    HTList * cur = hbuf_head;

    while (cur) {
	node = (HT_hbuf*)cur->object;
	if (node) {
	    FREE(node->line);
	    free(node);
	}
	killme = cur;
	cur = cur->next;
	free(killme);
    }
    hbuf_head = NULL;
    hbuf_tail = NULL;
    hbuf_total_len = 0;
}



PRIVATE BOOL fix_meta_field ARGS1(char *, buf)
{
    char * cur = buf;
    BOOL content = NO;

    if (!buf) return NO;

    while (*cur  &&  *cur!='\r'  &&  *cur!='\n') {
	if (!content  &&  *cur!=' ' &&  *cur!='\t')
	    content = YES;
	cur++;
    }
    if (!content) return NO;

    *cur++ = '\r';
    *cur++ = '\n';
    *cur = 0;
    return YES;
}


PUBLIC char *HTReplyHeaders ARGS1(HTRequest *, req)
{
    return HTReplyHeadersWith(req, NULL, 0);
}


PUBLIC char *HTReplyHeadersWith ARGS3(HTRequest *,	req,
				      char **,		extras,
				      int,		extra_cnt)
{
    char *headers;
    FILE * meta_file = NULL;
    int len;

    out.http_header_sent = YES;

    if (!out.status_code) out.status_code = 500;
    if (!HTReasonLine) HTReasonLine = "Internal error";

    len = strlen(HTReasonLine) + 1024 +
	  (HTLocation		? strlen(HTLocation)		: 0) +
	  (HTWWWAuthenticate	? strlen(HTWWWAuthenticate)	: 0);

    if (extras) {
	int i;
	for (i=0; i<extra_cnt; i++) {
	    if (extras[i])
		len += strlen(extras[i]) + 3;
	}
    }

    if (HTMetaFile) {
	struct stat stat_meta;

	if (HTStat(HTMetaFile, &stat_meta) != -1) {
	    len += stat_meta.st_size;
	    meta_file = fopen(HTMetaFile, "r");
	}
    }

    headers = (char*)malloc(len);
    if (!headers) outofmem(__FILE__, "HTReplyHeaders");

    sprintf(headers,
	    "%s %d %s\r\n%s%s",
	    HTServerProtocol, out.status_code, HTReasonLine,
	    "Server: CERN/", VD);

#ifndef VMS
    if (cur_time) {
	strcat(headers, "\r\nDate: ");
	strcat(headers, http_time(&cur_time));
    }
#endif	

    if (HTLocation) {		/* Do redirection */
	strcat(headers, "\r\nLocation: ");
	strcat(headers, HTLocation);
    }

    if (req->content_language) {
	strcat(headers, "\r\nContent-Language: ");
	strcat(headers, HTAtom_name(req->content_language));
    }

    if (req->content_encoding) {
	char * name = HTAtom_name(req->content_encoding);
	if (name && strcmp(name,"7bit") && strcmp(name,"8bit") &&
	    strcmp(name,"binary")) {
	    strcat(headers, "\r\nContent-Encoding: ");
	    strcat(headers, name);
	}
    }

    if (req->content_type) {
	strcat(headers, "\r\nContent-Type: ");
	strcat(headers, HTAtom_name(req->content_type));
    }

    if (out.content_length > 0) {
	char content_len[30];
	sprintf(content_len, "\r\nContent-Length: %d", out.content_length);
	strcat(headers, content_len);
    }

    if (HTLastModified) {
	strcat(headers, "\r\nLast-Modified: ");
	strcat(headers, HTLastModified);
    }

    if (HTExpires) {
	strcat(headers, "\r\nExpires: ");
	strcat(headers, HTExpires);
    }

    /* Filter out keep-alive and connection headers */
    if (extras) {
	int i;
	for (i=0; i<extra_cnt; i++) {
	    if (extras[i] && *(extras[i]) &&
		strncasecomp(extras[i], "keep-alive", 10) &&
		strncasecomp(extras[i], "connection", 10)) {
		strcat(headers, "\r\n");
		strcat(headers, extras[i]);
	    }
	}
    }

    strcat(headers, "\r\n");
    if (HTWWWAuthenticate) {
	strcat(headers, HTWWWAuthenticate);
    }

    if (meta_file) {
	char * buf = (char*)malloc(1001);
	if (!buf) outofmem(__FILE__, "HTReplyHeaders");

	while (fgets(buf, 1000, meta_file)) {
	    if (fix_meta_field(buf))
		strcat(headers, buf);
	}
	fclose(meta_file);
	free(buf);		/* Leak fixed AL 27 Feb 1994 */
    }

    strcat(headers, "\r\n");

    VTRACE(stderr,
	   "............ Header section for client\n%s%s",
	   headers, "............ End of headers\n");

    out.header_length = strlen(headers);

    if (TRACE) {
	fprintf(stderr, "HTTP header. length: %d bytes\n", out.header_length);
	fprintf(stderr, "............ Headers for the client\n");
	fprintf(stderr, "%s............ End of headers\n", headers);
    }

    return headers;
}


/*
 * Makes a full reference to the server itself given a URL with only
 * the path portion.  I.e. will add http:, hostname and port parts.
 *
 * Returns NULL if URL is already full.
 */
PUBLIC char * HTFullSelfReference ARGS1(CONST char *, url)
{
    char * result = NULL;

    if (url[0] == '/') {
	char portstr[10];
	if (!sc.hostname) sc.hostname = (char*)HTGetHostName();
	if (!sc.hostname) sc.hostname = "CANT.GET.MY.HOST.NAME";

	if (HTServerPort > 0  &&  HTServerPort != 80)
	    sprintf(portstr, ":%d", HTServerPort);
	else
	    *portstr = 0;

	result = (char*)malloc(strlen(sc.hostname)+strlen(url)+20);
	if (!result) outofmem(__FILE__, "HTFullSelfReference");
	sprintf(result, "http://%s%s%s", sc.hostname, portstr, url);
    }
    return result;
}


/*
 * Produce a redirection message document for browsers without
 * redirection support.
 */
PUBLIC char * HTRedirectionMsg ARGS1(char *, url)
{
    char * body = (char*)malloc(strlen(url) + 500);
    if (!body) outofmem(__FILE__, "HTRedirectionMsg");
    /*
     * Arrgh... ugly!
     */
    sprintf(body, "%s%s%s%s%s%s%s %s%s",
	    "<HTML>\n<HEAD>\n<TITLE>Redirection</TITLE>\n</HEAD>\n<BODY>\n",
	    "<H1>Redirection</H1>\nThis document can be found\n<A HREF=\"",
	    url,
	    "\">elsewhere.</A><P>\nYou see this message because your browser ",
	    "doesn't support automatic\nredirection handeling. <P>\n<HR>\n",
	    "<ADDRESS><A HREF=\"http://www.w3.org\">",
	    HTAppName,
	    HTAppVersion,
	    "</A></ADDRESS>\n</BODY>\n</HTML>\n");
    return body;
}


PUBLIC int HTLoadRedirection ARGS1(HTRequest *, req)
{
    if (req && HTLocation && 
	(HTReason == HTAA_OK_REDIRECT || HTReason == HTAA_OK_MOVED) &&
	req->output_stream) {
	char * body = NULL;

	{
	    char * full = HTFullSelfReference(HTLocation);
	    if (full) {
		free(HTLocation);
		HTLocation = full;
	    }
	}

	body = HTRedirectionMsg(HTLocation);
	req->content_type = HTAtom_for("text/html");
	out.content_length = strlen(body);

	if (HTClientProtocol) {
	    char * headers = HTReplyHeaders(req);
	    if (headers) {
		HTLoadStrToStream(req->output_stream, headers);
		free(headers);
	    }
	}
	if (req->method != METHOD_HEAD)
	    HTLoadStrToStream(req->output_stream, body);
	free(body);
	HTCloseStream(req->output_stream);
	return HT_LOADED;
    }
    return HT_INTERNAL;
}



/*
 *	MIME output with content-length calculation
 *
 *	This stream also buffers the result to find out the content length.
 *	If a maximum buffer limit is reached Content-Length is calculated
 *	for logs but it is not sent to the client -- rather the buffer is
 *	flushed right away.
 */

#define DEFAULT_CNT_BUF_SIZE 8096

typedef struct _HTBufItem {
    int			len;
    char *		buf;
    struct _HTBufItem *	next;
} HTBufItem;

struct _HTStream {
    HTStreamClass *	isa;
    HTRequest *		req;
    int			content_count;
    int			last_verbose;
    BOOL		give_up;
    char *		tmp_buf;
    int			tmp_ind;
    int			tmp_max;
    HTBufItem *		head;
    HTBufItem *		tail;
    HTStream *		sink;
    char **		extra_hdrs;
    int			extra_cnt;
};


PRIVATE void buf_free_bufs ARGS1(HTStream *, me)
{
    HTBufItem * cur = me->head;
    HTBufItem * killme;

    while (cur) {
	killme = cur;
	cur = cur->next;
	FREE(killme->buf);
	free(killme);
    }
    me->head = me->tail = NULL;
}

PRIVATE void buf_append_buf ARGS1(HTStream *, me)
{
    HTBufItem * b = (HTBufItem*)calloc(1,sizeof(HTBufItem));
    if (!b) outofmem(__FILE__, "buf_append_buf");

    b->len = me->tmp_ind;
    b->buf = me->tmp_buf;
    me->tmp_ind = 0;
    me->tmp_max = 0;
    me->tmp_buf = 0;

    if (me->tail)
	me->tail->next = b;
    else
	me->head = b;
    me->tail = b;
}

PRIVATE void buf_flush ARGS1(HTStream *, me)
{
    char * headers = HTReplyHeadersWith(me->req,me->extra_hdrs,me->extra_cnt);
    HTBufItem * cur;

    if (headers) {
	(*me->sink->isa->put_string)(me->sink, headers);
	free(headers);
    }
    if (me->tmp_buf)
	buf_append_buf(me);
    cur = me->head;
    while (cur) {
	(*me->sink->isa->put_block)(me->sink, cur->buf, cur->len);
	cur = cur->next;
    }
    buf_free_bufs(me);
}

PRIVATE BOOL buf_alloc_new ARGS2(HTStream *,	me,
				 int,		size)
{
    if (me->content_count >= sc.max_content_len_buf) {
	CTRACE(stderr,
      "\nGiving up... buffering to find out content-length [limit %d bytes]\n",
	       sc.max_content_len_buf);
	out.content_length = 0;
	buf_flush(me);
	return NO;
    }
    else {
	me->tmp_ind = 0;
	me->tmp_max = size;
	me->tmp_buf = (char *)malloc(size);
	if (!me->tmp_buf) outofmem(__FILE__, "buf_put_char");
	return YES;
    }
}

PRIVATE void buf_put_char ARGS2(HTStream *,	me,
				char,		c)
{
    me->content_count++;

    if (!me->give_up) {
	if (me->tmp_ind >= me->tmp_max)
	    buf_append_buf(me);
	if (!me->tmp_buf && !buf_alloc_new(me, DEFAULT_CNT_BUF_SIZE))
	    me->give_up = YES;
    }
    if (me->give_up)
	(*me->sink->isa->put_character)(me->sink,c);
    else
	me->tmp_buf[me->tmp_ind++] = c;
}

PRIVATE void buf_put_block ARGS3(HTStream *,	me,
				 CONST char *,	b,
				 int,		l)
{
    me->content_count += l;

    if (TRACE && me->content_count > me->last_verbose + DEFAULT_CNT_BUF_SIZE) {
	me->last_verbose = me->content_count;
	fprintf(stderr, " [%dK]", me->content_count/1024);
    }

    if (!me->give_up) {
	if (me->tmp_buf &&  me->tmp_max - me->tmp_ind <= l) {  /* Still room */
	    memcpy(me->tmp_buf + me->tmp_ind, b, l);
	    me->tmp_ind += l;
	    return;
	}
	else {
	    if (me->tmp_buf)
		buf_append_buf(me);
	    if (!buf_alloc_new(me,l))
		me->give_up = YES;
	    else {
		memcpy(me->tmp_buf, b, l);
		me->tmp_ind = l;
		buf_append_buf(me);
	    }
	}
    }
    if (me->give_up)
	(*me->sink->isa->put_block)(me->sink,b,l);
}

PRIVATE void buf_put_string ARGS2(HTStream *,	me,
				  CONST char *,	s)
{
    if (s) {
	int len = strlen(s);
	buf_put_block(me,s,len);
    }
}


PRIVATE void buf_free ARGS1(HTStream *,	me)
{
    out.content_length = me->content_count;
    CTRACE(stderr,"\nCalculated.. content-length: %d\n", out.content_length);
    if (!me->give_up)
	buf_flush(me);

    (*me->sink->isa->_free)(me->sink);
    FREE(me);
}

PRIVATE void buf_abort ARGS2(HTStream *,	me,
			     HTError,		e)
{
    if (!me->give_up)
	buf_free_bufs(me);
    (*me->sink->isa->abort)(me->sink,e);
    FREE(me);
}

HTStreamClass HTContentCounterClass = {
    "ContentCounter",
    buf_free,
    buf_abort,
    buf_put_char,
    buf_put_string,
    buf_put_block
};

PRIVATE HTStream * HTContentCounter ARGS4(HTRequest *,	req,
					  HTStream *,	sink,
					  char **,	extra_hdrs,
					  int,		extra_cnt)
{
    HTStream * me = (HTStream*)calloc(1,sizeof(HTStream));
    if (!me) outofmem(__FILE__, "HTContentCounter");

    CTRACE(stderr, "Calculating. content-length on the fly\n");

    me->isa = &HTContentCounterClass;
    me->req = req;
    me->sink = sink;
    me->extra_hdrs = extra_hdrs;
    me->extra_cnt = extra_cnt;
    return me;
}



/*
 *	Wrap document into MIME
 *
 *	If out.content_length is set just writes HTTP response
 *	headers to sink and returns the same sink.
 *
 *	Otherwise returns a new stream that buffers the input,
 *	calculates content-length, and only after that sends
 *	the entire reply down to sink.
 */
PUBLIC HTStream * HTMIMEWrapper ARGS5(HTRequest *,	req,
				      void *,		param,
				      HTFormat,		input_format,
				      HTFormat,		output_format,
				      HTStream *,	sink)
{
    CTRACE(stderr, "Ok.......... Content-type %s\n",
	   HTAtom_name(input_format));

    req->content_type = input_format;

    if (HTClientProtocol) {
	if (out.content_length > 0) {
	    char *headers = HTReplyHeaders(req);
	    if (headers) {
		(*sink->isa->put_string)(sink, headers);
		free(headers);
	    }
	    CTRACE(stderr, "Already..... known content-length: %d\n",
		   out.content_length);
	}
	else
	    return HTContentCounter(req,sink,NULL,0);
    }
    else {			/* Old protocol */
        if (input_format == WWW_PLAINTEXT)
	    (*sink->isa->put_string)(sink, "<PLAINTEXT>\r\n");
    }

    return sink;
}


PUBLIC HTStream * HTScriptWrapper ARGS4(HTRequest *,	req,
					char **,	extra_headers,
					int,		extra_count,
					HTStream *,	sink)
{
    if (!req->content_type) {
	req->content_type = HTAtom_for("text/plain");
	CTRACE(stderr, "Default..... content-type text/plain\n");
    }
    else {
	CTRACE(stderr, "Ok.......... script result content-type %s\n",
	       HTAtom_name(req->content_type));
    }

    if (HTClientProtocol) {
	if (out.content_length > 0) {
	    char *headers = HTReplyHeadersWith(req,extra_headers,extra_count);
	    if (headers) {
		(*sink->isa->put_string)(sink, headers);
		free(headers);
	    }
	    CTRACE(stderr, "Already..... known content-length: %d\n",
		   out.content_length);
	}
	else
	    return HTContentCounter(req,sink,extra_headers,extra_count);
    }
    else {			/* Old protocol */
        if (req->content_type == WWW_PLAINTEXT)
	    (*sink->isa->put_string)(sink, "<PLAINTEXT>\r\n");
    }

    return sink;
}


PUBLIC HTStream * HTHackWrapper ARGS5(HTRequest *,	req,
				      void *,		param,
				      HTFormat,		input_format,
				      HTFormat,		output_format,
				      HTStream *,	sink)
{
    CTRACE(stderr,
    "Hack........ Sending to client even there was no corresponding Accept\n");
    CTRACE(stderr, "Ok.......... Content-type %s\n",
	   HTAtom_name(input_format));

    req->content_type = input_format;

    if (HTClientProtocol) {
	char * extras[2];
	char * headers;
	extras[0] =
    "X-httpd-warning: Your broser didn't send the Accept header line for this";
	extras[1] = NULL;
	headers = HTReplyHeadersWith(req, extras, 1);
	if (headers) {
	    (*sink->isa->put_string)(sink, headers);
	    free(headers);
	}
    } else {			/* Old protocol */
        if (input_format == WWW_PLAINTEXT) {
	    (*sink->isa->put_string)(sink, "<PLAINTEXT>\r\n");
	}
    }

    return sink;
}
    


PRIVATE char * get_part ARGS2(char **,	source,
			      char *,	separator)
{
    char * word;
    char * p;
    char * q;

    if (!source || !*source) return NULL;
    word = *source;

    while (*word && (*word==' ' || *word=='\t'))
	word++;

    for (p=word;  *p && *p != ';' && *p != ',';  p++) {
	if (*p == ' ') {	/* @@@ HACK TO UNDERSTAND OLD Accept: FORMAT */
	    *p = 0;		/* @@@ REMOVE THESE 4 LINES ONCE OLD         */
	    break;		/* @@@ BROWSERS DISAPPEAR (besides, this     */
	}			/* @@@ is VERY WRONG).			     */
    }
    *separator = *p;
    if (*p) {
	*source = p+1;
	*p = 0;
    }
    else *source = p;

    for (q=p-1; q>word; q--) {
	if (*q==' ' || *q=='\t')
	    *q = 0;
	else break;
    }

    return word;
}


PRIVATE char * get_word ARGS1(char **, source)
{
    char * word;
    char * p;

    if (!source || !*source) return NULL;
    word = *source;

    while (*word && (*word==' ' || *word=='\t'))
	word++;

    for (p=word;  *p && *p != ' ' && *p != '\t';  p++);
    while (*p && (*p == ' ' || *p == '\t'))
	*p++ = 0;
    *source = p;

    return word;
}



PRIVATE char * parse_entry ARGS5(char **,	source,
				 char **,	accept_x,
				 float *,	q,
				 float *,	mxs,
				 float *,	mxb)
{
    char sep;

    if (!source || !*source || !accept_x || !q || !mxs || !mxb)
	return NULL;

    *q = 1.0;
    *mxb = 0.0;
    *mxs = 0.0;
    *accept_x = get_part(source, &sep);
    if (!*accept_x || !**accept_x)
	return NULL;
    while (sep == ';') {
	float value;
	char *part = get_part(source, &sep);
	char *equals = part ? strchr(part, '=') : NULL;

	if (!equals) continue;    /* bad syntax -- forget it! */
	*equals++ = 0;	/* Split at equals */
	if (sscanf(equals, "%f", &value)) {
	    char * attrib = HTStrip(part);
	    if (!strcasecomp(attrib, "q"))
		*q = value;
	    else if (!strcasecomp(attrib, "mxb"))
		*mxb = value;
	    else if (!strcasecomp(attrib, "mxs"))
		*mxs = value;
	}
    } /* scan attributes */
    return *accept_x;
}



/* PUBLIC						HTParseRequest()
**		READ REQUEST FROM HTInputSocket OBJECT
**		AND PARSE IT INTO A HTRequest STRUCTURE
** ON ENTRY:
**	isoc	input socket structure.
**
** ON EXIT:
**	returns	a newly allocated HTRequest object describing
**		parsed request.
**		NULL if something fails badly.
**	isoc	is read until the end of header section (empty
**		line is last thing read), so isoc input pointer
**		points to the beginning of message body.
*/
PUBLIC HTRequest * HTParseRequest ARGS1(HTInputSocket *, isoc)
{
    HTRequest *req;
    char *line;
    char *p, *p1, *p2, *p3;
    BOOL text_html = NO;
    BOOL text_plain = NO;

    if (!isoc)
	return NULL;

    req = HTRequest_new();
    HTReason = HTAA_OK;
    HTClientProtocol = NULL;	/* Assume HTTP0 */

    /*
    ** Parse first line of request
    */
    p = line = HTInputSocket_getLine(isoc);
    if (line) {
	VTRACE(stderr, "Client sez.. %s\n", line);
	StrAllocCopy(HTReqLine, line);

	p1 = get_word(&p);
	p2 = get_word(&p);
	p3 = get_word(&p);

	if (p1) req->method = HTMethod_enum(p1);
	if (p2) StrAllocCopy(HTReqArg, p2);
	if (p3) StrAllocCopy(HTClientProtocol, p3);
	free(line);	/* Leak fixed AL 6 Feb 94 */
    }

    /*
    ** Check that the third argument actually is a valid
    ** client protocol specifier (if it is not we might wait
    ** for an eternity for the rest of an HTTP1 request when it
    ** will never come, becuase it's actually an HTTP0 request.
    */
    if (HTClientProtocol &&
	0 != strncasecomp(HTClientProtocol, "HTTP/", 5)) {
	free(HTClientProtocol);
	HTClientProtocol = NULL;
    }

#ifndef NOCONVERT
    if (!req->conversions)
	req->conversions = HTList_new();
    if (sc.do_accept_hack)
	HTSetConversion(req->conversions,
			"*/*", "www/present", HTHackWrapper,
			0.00001, 0.0, 0.0);
#endif			
    
    hbuf_free();	/* From previous call */

    if (HTClientProtocol) {
      
	while ((line = HTInputSocket_getUnfoldedLine(isoc))) {

	    /* Don't include keep-alive and connection */
	    if (strncasecomp(line, "keep-alive", 10) &&
		strncasecomp(line, "connection", 10)) {
		hbuf_append(line);
	    }
	    if (!*line) {
		free(line);	/* Leak fixed AL 6 Feb 1994 */
		break;		/* Empty line terminating header section */
	    }
	    VTRACE(stderr, "Client sez.. %s\n", line);
	    p = strchr(line, ':');
	    if (!p) continue;			/* Bad format -- junk line */
	    *p++ = 0;					/* Overwrite colon */
	    for ( ;  *p==' ' || *p=='\t';  p++);	/* Skip whitespace */

	    if (!strcasecomp(line, "Accept") ||
		!strcasecomp(line, "Accept-Encoding") ||
		!strcasecomp(line, "Accept-Language")) {
		char *name = NULL;
		float quality;
		float maxbytes;
		float maxsecs;

		while (parse_entry(&p,&name,&quality,&maxbytes,&maxsecs)) {  
		    if (!strcasecomp(line, "Accept")) {

#ifdef VMS	/* In Unix we do this in the general HTTP_XXX way */
			hbuf_cgi_cancel_last();
#endif
			VTRACE(stderr,
			       "Accept...... %s (q=%.2f,mxb=%.1f,mxs=%.1f)\n",
			       name, quality, maxbytes, maxsecs);
#ifndef NOCONVERT
			if (!strcasecomp(name, "text/html"))
			    text_html = YES;
			else if (!strcasecomp(name, "text/plain"))
			    text_plain = YES;
                        if (!strcmp(name, "www/source"))
				HTSetConversion(req->conversions,
					"*/*", "www/present", HTMIMEWrapper,
					quality, maxbytes, maxsecs);
                        else
  				HTSetConversion(req->conversions,
					name, "www/present", HTMIMEWrapper,
					quality, maxbytes, maxsecs);
#endif
		    }
		    else if (!strcasecomp(line, "Accept-Encoding")) {
			CTRACE(stderr, "Encoding.... %s (q=%.2f)\n",
			       name, quality);
			if (!req->encodings)
			    req->encodings = HTList_new();
			HTAcceptEncoding(req->encodings, name, quality);
		    }
		    else {
			CTRACE(stderr, "Language.... %s (q=%.2f)\n",
			       name, quality);
			if (!req->languages)
			    req->languages = HTList_new();
			HTAcceptLanguage(req->languages, name, quality);
		    }
		}

	    } else if (0==strncasecomp(line, "Content-Type", 12)) {

		hbuf_cgi_cancel_last();
		p1 = get_word(&p);
		if (p1) {
		    req->content_type = HTAtom_for(p1);
		    CTRACE(stderr, "Content-Type %s\n", p1);
		}

	    } else if (0==strncasecomp(line, "Content-Length", 14)) {

		hbuf_cgi_cancel_last();
		p1 = get_word(&p);
		if (p1) {
		    req->content_length = atoi(p1);
		    CTRACE(stderr, "Content-Length %d\n", req->content_length);
		}

	    } else if (0==strncasecomp(line, "Authorization", 13)) {

		StrAllocCopy(req->authorization, p);
		p1 = get_word(&p);
		p2 = get_word(&p);
		CTRACE(stderr, "Authorization scheme \"%s\" params \"%s\"\n",
		       (p1 ? p1 : "-null-"),  (p2 ? p2 : "-null-"));
		if (p1 && p2) {
		    req->scheme = HTAAScheme_enum(p1);
		    StrAllocCopy(HTAuthString, p2);
		}
		else CTRACE(stderr, "Invalid..... Authorization: field\n");

	    } else if (0==strncasecomp(line, "From", 4)) {

		StrAllocCopy(req->from, HTStrip(p));
		CTRACE(stderr, "From........ %s\n", req->from);

	    } else if (0==strncasecomp(line, "User-Agent", 10)) {

		hbuf_proxy_cancel_last();
		StrAllocCopy(HTUserAgent, HTStrip(p));
		CTRACE(stderr, "User-Agent.. %s\n", HTUserAgent);

	    } else if (0==strncasecomp(line, "Referer", 7)) {

		StrAllocCopy(HTReferer, HTStrip(p));
		CTRACE(stderr, "Referer..... %s\n", HTReferer);

	    } else if (0==strncasecomp(line, "If-Modified-Since", 17)) {

		hbuf_proxy_cancel_last();
		HTIfModifiedSince = parse_http_time(HTStrip(p));
		CTRACE(stderr, "Give only... if modified since (localtime) %s",
		       ctime(&HTIfModifiedSince));

	    } else if (0==strncasecomp(line, "Pragma", 6)) {
		char * name = NULL;
		float junk1, junk2, junk3;

		/*
		 * Pragmas will be passed to the remote server too by proxy
		 * (so not doing hbuf_proxy_cancel_last()).
		 */
		while (parse_entry(&p,&name,&junk1,&junk2,&junk3)) {
		    if (!strcasecomp(name,"no-cache")) {
			in.no_cache_pragma = YES;
			CTRACE(stderr,
			       "Pragma...... no-cache (force refresh)\n");
		    }
		    else {
			CTRACE(stderr,
			       "Unknown..... pragma ignored: %s\n",name);
		    }
		}

	    } else if (0==strncasecomp(line, "Charge-To", 9)) {

		/* Add code here */

	    }
	    /* else invalid */
	    free(line);		/* Leak fixed AL 6 Feb 94 */

	} /* scan lines */
    } /* if HTTP1 protocol */
#ifndef NOCONVERT
    else {
	/*
	** Old protocol browsers have to be assumed to allow binary and
	** plain old source.  This is for XMosaic 1.x, and we don't have
	** to support it.. soon.
	** This is risky & more trouble.
	*/	
	HTSetConversion(req->conversions,
			"application/octet-stream","www/present",HTMIMEWrapper,
			0.7, 0.0, 0.0);
	HTSetConversion(req->conversions,
			"www/source", "www/present", HTMIMEWrapper,
			0.7, 0.0, 0.0);
    }
#endif

#ifndef NOCONVERT
    if (!text_html)
	HTSetConversion(req->conversions,
			"text/html", "www/present", HTMIMEWrapper,
			1.0, 0.0, 0.0);
    if (!text_plain)
	HTSetConversion(req->conversions,
			"text/plain", "www/present", HTMIMEWrapper,
			1.0, 0.0, 0.0);
#endif			

    compute_server_env();

    return req;
}

