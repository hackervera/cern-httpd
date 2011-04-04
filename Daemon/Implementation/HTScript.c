
/* MODULE							HTScript.c
**		CALL A SCRIPT TO PRODUCE A DOCUMENT ON FLY
**
** AUTHORS:
**	AL	Ari Luotonen	luotonen@dxcern.cern.ch
**	MD	Mark Donszelmann    duns@vxdeop.cern.ch
**
** HISTORY:
**	31 Oct 93  AL	Written from scratch.
**	 6 Nov 93  MD	Made VMS compatibility.
**	13 Nov 93  MD	Escaped parameters and keywords correctly for VMS.
**	 3 Mar 94  MD 	Made VMS compatibility.
**	31 Mar 94  FM	Added code for using spawninit and scratchdir as
**			defined in rule file for VMS.
**			Fixed bug in error logging if script not found
**			on VMS.
**	 2 Apr 94  FM	Fixed bug which caused redirection on the fly to
**			fail on VMS.
**	 4 Apr 94  FM	Modified so that it also creates WWW_KEY symbols
**			for POST on VMS.  I see no other efficient way to
**			do it via mailboxes.
**			Fixed the loading of the WWW_IN mailbox so that
**			it's in records of a size which won't hang or
**			ACCVIO the httpd on the mailbox, and so that it
**			could actually be handled by DCL scripts.
**			Load reg->from into WWW_REMOTE_USER if username
**			was not obtained via the authorization routines.
**	 8 Apr 94 FM	Use "define/nolog sys$output WWW_OUT" instead of
**			"/out=WWW_OUT" for VMS scripts, so the mailbox is
**			inherited as stdout for printf()'s in DECC programs
**			invoked by the script.
**	 6 Jul 94 FM	Added code to have spawned VMS processes set default
**			to the default data tree root, if one was defined via
**			"ServerRoot" in the configuration file.
**	 		Added REFERER_URL (with WWW_ prefix on VMS) to
**			CGI_env_vars[] and HTScriptEnv (hope that's OK; I
**			added it to 216-1betavms last May, and VMSers are
**			using it).
**	 8 Jul 94  FM	Insulate free() from _free structure element.
**
**
** BUGS:
**
**
*/

#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef Mips
#include <sys/wait.h>
#endif

#include "HTUtils.h"

#ifdef VMS
#include <unixlib.h>
#include <descrip.h>
#include <dvidef.h>
#include <clidef.h>
#include <psldef.h>
#include <prvdef.h>
#include <ssdef.h>
#include <iodef.h>
#include <unixio.h>
/* BEGIN should have been defined in unixio.h */
#define X_OK 1 
/* END should have been defined in unixio.h */
#include "HTVMSUtils.h"
#endif /* VMS */

#include "HTAuth.h"	/* HTUser */
#include "HTRules.h"	/* HTBinDir, HTBINDIR */
#include "HTParse.h"	/* HTUnEscape() */
#include "HTFile.h"
#include "HTWriter.h"
#include "HTStream.h"
#include "HTTCP.h"
#include "tcp.h"
#include "HTRequest.h"
#include "HTDaemon.h"
#include "HTConfig.h"
#include "HTLog.h"


#ifdef VMS
#define INPUT_BUFFER_SIZE 256
#define CGI_ENV_VAR_CNT	   500	/* No check done */
#define MAX_DCL_COMMAND_LENGTH 200
#define MAX_DCL_PARAMETERS 8

#define CGI_VERSION	"CGI/1.0"

#else  /* not VMS */

#define INPUT_BUFFER_SIZE  4096
#define CGI_ENV_VAR_CNT	   100	/* Currently 18 */

#define CGI_VERSION	"CGI/1.1"

#endif /* not VMS */


extern char * HTClientHostName;
extern char * HTClientProtocol;
extern char * HTServerProtocol;
extern HTInputSocket *isoc;

#ifdef VMS
extern char * HTScratchDir;
#endif /* VMS */

PRIVATE char ** script_env = NULL;
PRIVATE char ** script_env_tail = NULL;
PRIVATE char ** script_env_boundary = NULL;
PRIVATE int cur_cgi_var_cnt = 0;
PRIVATE BOOL cgi_script = NO;

/*
 *	SCRIPT OUTPUT
 */
struct _HTStream {
    HTStreamClass * isa;
    /* ...and opaque stuff ... */
};


/* PRIVATE						HTLoadScriptResult()
**		PUSH DATA FROM SCRIPT OUTPUT FILE TO STREAM
**		USING CORRECT PROTOCOL VERSION (HTTP0/HTTP1).
**		DO REDIRECTION IF REQUESTED BY THE SCRIPT.
** ON ENTRY:
**	fp	open script output file, format:
**
**			Content-Type: .../...
**			<blank line>
**			Document
**		or:
**			Location: ...
**			<blank line>
**
**		which causes the server to send a redirection
**		reply.
**
**	sink	stream to push the data down to.
**	req	request.
**
** ON EXIT:
**	returns	HT_LOADED on success.
**		HT_REDIRECTION_ON_FLY if the daemon should redo the
**		retrieve (in case the redirection is a local file).
**		File is read until the EOF, but left open.
*/
PRIVATE int HTLoadScriptResult ARGS3(FILE *,		fp,
				     HTStream *,	sink,
				     HTRequest *,	req)
{
    HTStreamClass targetClass;
    char input_buffer[INPUT_BUFFER_SIZE+2];
    BOOL reading_headers = YES;
    char *headers = NULL;
    char *body = NULL;

#define MAX_HEADERS 500
    char * header_lines[MAX_HEADERS];
    int header_index = 0;

#ifdef VMS
    struct _iosb
    {
       short status;
       short length;
       long devspec;
    } iosb;
#endif

    if (!fp || !sink || !req)
	return HT_INTERNAL;

    HTLocation = NULL;

    targetClass = *(sink->isa);	/* Copy pointers to procedures */

    for(;;) {
#ifdef VMS
        int status = SYS$QIOW(0, (long)fp, IO$_READVBLK, &iosb, 0, 0, input_buffer, INPUT_BUFFER_SIZE, 0, 0, 0, 0);
        if (!(status & 0x01)) exit(status);

	if (iosb.status != SS$_NORMAL) { /* EOF or error */
	    if (iosb.status == SS$_ENDOFFILE) break;
	    CTRACE(stderr,"HTLoadScriptResult: Read error, VMS read returns %d\n",
		   iosb.status);
	    break;
	}

        input_buffer[iosb.length] = '\n';
        input_buffer[iosb.length+1] = '\0';
        status = iosb.length+1;
#else
	int status = fread(input_buffer, 1, INPUT_BUFFER_SIZE, fp);

	if (status == 0) { /* EOF or error */
	    if (ferror(fp) == 0) break;
	    CTRACE(stderr,"HTLoadScriptResult: Read error, read returns %d\n",
		   ferror(fp));
	    break;
	}
#endif /* not VMS */

	if (reading_headers) {
	    char *cur = input_buffer;
	    char *end = input_buffer + status;
	    
	    while (cur < end) {
		char * line_end = strchr(cur, '\n');
		if (line_end) {

		    if (*(line_end-1) == '\r')	*(line_end-1) = 0;
		    else			*line_end = 0;

		    if (!*cur) {    /* Empty line => end of header section */
			reading_headers = NO;
			cur = line_end+1;
			break;
		    }
		    else {
			char * colon = strchr(cur, ':');

			header_lines[header_index] = NULL;
			if (header_index < MAX_HEADERS) {
			    StrAllocCopy(header_lines[header_index], cur);
			    header_index++;
			} else {
			    if (TRACE) fprintf(stderr, "Script...... too many header lines\n");
			    break;
			}

			if (colon) {
			    BOOL no_copy = NO;
			    char * p = colon + 1;
			    *colon = 0;
			    while (*p && WHITE(*p)) p++;

			    if (0 == strcasecomp(cur, "Location")) {
				StrAllocCopy(HTLocation,p);
				no_copy = YES;
			    }
			    else if (0 == strcasecomp(cur, "Content-Type")) {
				HTStrip(p);
				req->content_type = HTAtom_for(p);
				no_copy = YES;
			    }
			    else if (0 == strcasecomp(cur, "Status")) {
				while(*p && WHITE(*p)) p++;
				out.status_code = atoi(p);
				p = strchr(p,' ');
				if (p) while (*p && WHITE(*p)) p++;
				if (p && *p) {
				    HTReasonLine = NULL;
				    StrAllocCopy(HTReasonLine, p);
				    /* This should be freed but it isn't   */
				    /* because usually HTReasonLine points */
				    /* to a constant string. What should I */
				    /* do?				   */
				}
				else {
				    HTReasonLine =
					get_http_reason(out.status_code);
				}
				no_copy = YES;
			    }
			    if (no_copy) {
				header_index--;
				FREE(header_lines[header_index]);
			    }
			} /* Found colon */
		    } /* Non-empty line */
		    cur = line_end+1;
		} /* an entire line read */
		else {
		    /* Should buffer for next time... @@@@ */
		    cur = end;
		}
	    } /* for the entire buffer */

            if (!reading_headers) { /* headers have passed */
	    if (HTLocation) {	/* Redirection */
		if (HTLocation[0] == '/') {	/* Local file -- do retrieve */
		    if (!strchr(HTLocation, '#')) {
			return HT_REDIRECTION_ON_FLY;
		    }
		    else {	/* Need to do actual redirection because */
			        /* there is a label after the file name. */
			char * new_loc = HTFullSelfReference(HTLocation);
			if (new_loc) {
			    free(HTLocation);
			    HTLocation = new_loc;
			}
			/* And then continue as if the script had */
			/* originally given a full URL.           */
		    }
		}
		out.status_code = 302;
		HTReasonLine = "Found";
		body = HTRedirectionMsg(HTLocation);
		req->content_type = HTAtom_for("text/html");
		out.content_length = strlen(body);

		if (HTClientProtocol) {
		    headers=HTReplyHeadersWith(req,header_lines,header_index);
		    if (headers) {
			(*targetClass.put_string)(sink, headers);
			free(headers);
		    }
		    else {
			(*targetClass.put_string)(sink, "\
HTTP/1.0 500 Internal error -- couldn't compose regular header lines\r\n\r\n");
		    }
		}
		if (req->method != METHOD_HEAD)
		    (*targetClass.put_string)(sink, body);
		free(body);
		goto done;
	    }
	    else { /* No redirection, just send the document */
		sink = HTScriptWrapper(req,header_lines,header_index,sink);
		targetClass = *(sink->isa);
		(*targetClass.put_block)(sink, cur, end-cur);
            } /* not relocation */
	    } /* not reading headers */
	} /* if reading headers */
	else { /* else reading body */
	    (*targetClass.put_block)(sink, input_buffer, status);
	} /* reading body */

    } /* next bufferload */

  done:

    (*targetClass._free)(sink);

    /*
     * Important that these are freed only *after* the sink is freed;
     * sink's free routine may still reference these.
     */
    header_index--;
    for(; header_index >=0; header_index--)
	FREE(header_lines[header_index]);

    return HT_LOADED;
}


PRIVATE void plusses_to_spaces ARGS1(char *, s)
{
    if (!s) return;

    for(; *s; s++) if (*s == '+') *s=' ';
}



PRIVATE char ** HTScriptArgv ARGS1(HTRequest *, req)
{
    static char * pool = NULL;	/* Contains all the strings in keywordvec */
    static char ** argv = NULL;
    char ** cur_argv = NULL;
    int arg_cnt = 5;	/* argv[0], NULL terminator and just in case */
    char *cur;
    char *dest;
    char *word;

    FREE(argv);		/* From previous call */
    FREE(pool);		/* 	- " -	      */

    if (!req) {
	CTRACE(stderr, "INTERNAL... HTScriptArgv: error: req is NULL!!\n");
	return NULL;
    }

    if (!cgi_script) arg_cnt++;	/* For pathinfo as argv[1] */

    if (HTReqArgKeywords  &&  HTReqArgKeywords[0]) {

	if (strchr(HTReqArgKeywords, '=')) {	/* Form request */

#ifndef VMS
/* for VMS we parse anyway, since we convert the args into env later. We then
   do NOT send the args... */
	    if (cgi_script) {	/* CGI forms ==> no preparsing */

		argv = (char**)malloc(arg_cnt * sizeof(char*));
		if (!argv) outofmem(__FILE__, "HTScriptArgv");
		argv[1] = NULL;	/* Terminate -- argv[0] will be set later */

	    }
	    else 
#endif /* not VMS */
		 {	/* Non-CGI form request */

		int equal_signs = 0;	/* Number of = signs in form req */
		BOOL form_value = NO;	/* Are we parsing form field value */
	                                /* or name? */
		/*
		** Count form fields and values.
		*/
		for (cur=HTReqArgKeywords;  *cur;  cur++) {
		    if ('&' == *cur) arg_cnt++;
		    else if ('=' == *cur) equal_signs++;
		}
		arg_cnt += equal_signs;
		arg_cnt++;	/* Last one */

		/*
		** Allocate argv[]
		*/
		argv = (char**)malloc(arg_cnt * sizeof(char*));
		if (!argv) outofmem(__FILE__, "HTScriptArgv");

#ifdef VMS
                if (cgi_script)
     		   cur_argv = argv + 1;	/* For argv[0] */
                else
 		   cur_argv = argv + 2;	/* For argv[0] and argv[1] */
#else /* not VMS */
		cur_argv = argv + 2;	/* For argv[0] and argv[1] */
#endif /* not VMS */

		/*
		** Copy keyword string so that after each equal sign
		** there is space for an extra NULL.
		*/
		pool = (char*)malloc(strlen(HTReqArgKeywords) + equal_signs + 1);
		if (!pool) outofmem(__FILE__, "HTScriptArgv");

		word=dest=pool;
		cur=HTReqArgKeywords;
		for(;;) {
		    *dest = *cur;
		    if (!*cur) {
			if (form_value) {
			    plusses_to_spaces(word);
			    *(cur_argv++) = HTUnEscape(word);
			}
			break;
		    }
		    else if (!form_value && '=' == *cur) {
			*(++dest) = 0;				/* Terminate */
			plusses_to_spaces(word);
			*(cur_argv++) = HTUnEscape(word);	/* Put to argv */
			word = dest+1;				/* Start next arg */
			form_value = YES;
		    }
		    else if (form_value && '&' == *cur) {
			*dest = 0;				/* Terminate */
			plusses_to_spaces(word);
			*(cur_argv++) = HTUnEscape(word);	/* Put to argv */
			word = dest+1;				/* Start next arg */
			form_value = NO;
		    }
		    cur++;
		    dest++;
		}
		*(cur_argv++) = NULL;	/* Terminate argv */

	    } /* Non-CGI form request */
	}
	else {	/* Search request */
	    /*
	    ** Count search keywords and allocate argv
	    */
	    for(cur = HTReqArgKeywords; *cur; cur++)
		if (*cur == '+') arg_cnt++;
	    arg_cnt++;	/* Last one */

	    /*
	    ** Allocate argv[]
	    */
	    argv = (char**)malloc(arg_cnt * sizeof(char*));
	    if (!argv) outofmem(__FILE__, "HTScriptArgv");
	    cur_argv = argv + 1;		/* For argv[0] */
	    if (!cgi_script) cur_argv++;	/* For argv[1] */

	    /*
	    ** Put keywords to argv decoded.
	    */
	    pool = (char*)malloc(strlen(HTReqArgKeywords) + 1);
	    word=dest=pool;
	    cur=HTReqArgKeywords;
	    for(;;) {
		*dest = *cur;
		if (!*cur) {
		    *cur_argv++ = HTUnEscape(word);
		    break;
		}
		else if ('+' == *cur) {
		    *dest = 0;
		    *(cur_argv++) = HTUnEscape(word);
		    word = dest+1;
		}
		dest++;
		cur++;
	    }
	    *cur_argv = NULL;	/* Terminate argv */

	} /* else search request */
    } /* if search keywords given */
    else {
	argv = (char**)malloc(3 * sizeof(char*));
	if (!argv) outofmem(__FILE__, "HTScriptArgv");
	argv[1] = NULL;
	argv[2] = NULL;
    }

    /*
    ** Set argv[0]
    */
    if ((cur = strrchr(HTReqScript, '/')))
	argv[0] = cur+1;
    else argv[0] = HTReqScript;

    /*
    ** Set argv[1]
    */
    if (!cgi_script) {
	if (HTScriptPathInfo)
	    argv[1] = HTScriptPathInfo;
	else argv[1] = "";
    }

    if (TRACE) {
	int i;
	fprintf(stderr, "Info........ HTScriptArgv: returning:\n");
	for(i=0; argv[i]; i++)
	    fprintf(stderr, "\targv[%d] = \"%s\"\n", i, argv[i]);
    }
    return argv;
}


PRIVATE void HTPutScriptEnv ARGS2(CONST char *,	name,
				  CONST char *,	value)
{
    char *var;

    if (++cur_cgi_var_cnt > CGI_ENV_VAR_CNT) {
	if (TRACE) {
	    fprintf(stderr, "INTERNAL.... HTPutScriptEnv: Too many CGI env vars!\n");
	    fprintf(stderr, "Explanation. Adjust CGI_ENV_VAR_CNT macro, currently %d",
		    CGI_ENV_VAR_CNT);
	}
	return;
    }

    if (!name) {
	CTRACE(stderr, "INTERNAL.... HTPutScriptEnv: Error: name is NULL!\n");
	return;
    }

#ifdef VMS
    var = (char *)malloc(strlen(name) + strlen(value?value:"") + 11);
    if (!var) outofmem(__FILE__, "HTPutScriptEnv");

    sprintf(var, "WWW_%s == \"%s\"", name, value?value:"");
#else /* not VMS */
    var = (char *)malloc(strlen(name) + strlen(value?value:"") + 2);
    if (!var) outofmem(__FILE__, "HTPutScriptEnv");

    sprintf(var, "%s=%s", name, value?value:"");
#endif /* not VMS */
    CTRACE(stderr, "Environ..... %s\n", var);
    *(script_env_tail++) = var;
    *script_env_tail = NULL;
}


PRIVATE char * CGI_env_vars[] =
{
    "SERVER_SOFTWARE",	"SERVER_NAME",	"GATEWAY_INTERFACE",
    "SERVER_PROTOCOL",	"SERVER_PORT",	"REQUEST_METHOD",
    "HTTP_ACCEPT",
    "PATH_INFO",	"PATH_TRANSLATED",
    "SCRIPT_NAME",	"QUERY_STRING",
    "REMOTE_HOST",	"REMOTE_ADDR",
    "AUTH_TYPE",	"REMOTE_USER",	"REMOTE_IDENT",
    "CONTENT_TYPE",	"CONTENT_LENGTH",
    "REFERER_URL",
    NULL
};


PRIVATE BOOL CGI_env_var ARGS1(char *, s)
{
    char ** cur;

    if (s) {
	for(cur=CGI_env_vars; *cur; cur++)
	    if (!strncmp(*cur, s, strlen(*cur))) return YES;
    }
    return NO;
}


PRIVATE void HTInitScriptEnv()
{
    if (!script_env) {
	extern char ** environ;
	char ** cur;
	int len = CGI_ENV_VAR_CNT + 1;
	char buf[60];

	for(cur=environ; *cur; cur++)
	    len++;

	script_env = (char**)malloc(len * sizeof(char*));
	if (!script_env) outofmem(__FILE__, "HTInitScriptEnv");

	script_env_tail = script_env;
#ifndef VMS	/* not part of the CGI spec anyway */
	for (cur=environ; *cur; cur++) {
	    if (CGI_env_var(*cur)) {
		CTRACE(stderr, "WARNING..... Reserved CGI env.var. \"%s\"\n", *cur);
		CTRACE(stderr, "............ not propagated to child\n");
	    }
	    else *(script_env_tail++) = *cur;
	}
#endif /* not VMS */
	*script_env_tail = NULL;

	cur_cgi_var_cnt = 0;
	sprintf(buf, "CERN/%s", VD);
	HTPutScriptEnv("SERVER_SOFTWARE", buf);

	if (!sc.hostname) sc.hostname = (char*)HTGetHostName();
	HTPutScriptEnv("SERVER_NAME", sc.hostname);

	sprintf(buf, "%d", HTServerPort);
	HTPutScriptEnv("SERVER_PORT", buf);
	script_env_boundary = script_env_tail;
    }
    else {
	char **cur = script_env_boundary;
	while (*cur) {
	    free(*cur);
	    *cur = NULL;
	    cur++;
	}
	cur_cgi_var_cnt = 3;
        script_env_tail = script_env_boundary;
    }
}


PRIVATE char ** HTScriptEnv ARGS1(HTRequest *, req)
{
    HTInitScriptEnv();

    if (!req) return script_env;

    if (cgi_script)
	HTPutScriptEnv("GATEWAY_INTERFACE", CGI_VERSION);
    else
	HTPutScriptEnv("GATEWAY_INTERFACE", "CERN-PreParsed");

    if (HTClientProtocol)
	HTPutScriptEnv("SERVER_PROTOCOL", HTClientProtocol);
    else
	HTPutScriptEnv("SERVER_PROTOCOL", "HTTP/0.9");

    if (req->method != METHOD_INVALID)
	HTPutScriptEnv("REQUEST_METHOD", HTMethod_name(req->method));
    else
	HTPutScriptEnv("REQUEST_METHOD", NULL);


#ifdef VMS	/* CGI 1.0 !!!! */
    {
        char accept[20]; /* VMS */

	int s = 500;
	int remains = s - 1;
	int cnt = 0;
	char *buf = (char*)malloc(s);
	HTList *cur = req->conversions;
	HTPresentation *pres;

	if (!buf) outofmem(__FILE__, "HTScriptEnv");
	*buf = 0;

	while ((pres = (HTPresentation*)HTList_nextObject(cur))) {
	    if (pres->rep) {
		char *name = HTAtom_name(pres->rep);
		if ((remains -= ((int)strlen(name) + 3)) <= 0) {
		    remains += s;
		    s *= 2;
		    buf = (char*)realloc(buf, s);
		}
                cnt++;
                sprintf(accept,"HTTP_ACCEPT_%d",cnt);
                HTPutScriptEnv(accept,name);		
	    }
	}
        sprintf(accept,"%d",cnt);
	HTPutScriptEnv("HTTP_ACCEPT", accept);
	FREE(buf);	/* Leak fixed AL 6 Feb 1994 */
    }
#endif	/* VMS-ONLY SOLUTION  CGI 1.0 !!!! */


    HTPutScriptEnv("PATH_INFO", HTScriptPathInfo);
    HTPutScriptEnv("PATH_TRANSLATED", HTScriptPathTrans);
    HTPutScriptEnv("QUERY_STRING", HTReqArgKeywords);

    if (!HTScriptPathInfo) {
	HTPutScriptEnv("SCRIPT_NAME", HTReqArgPath);
    } else {
	int i = strlen(HTReqArgPath) - strlen(HTScriptPathInfo);
	if (i > 0) {
	    char saved = HTReqArgPath[i];
	    HTReqArgPath[i] = '\0';
	    HTPutScriptEnv("SCRIPT_NAME", HTReqArgPath);
	    HTReqArgPath[i] = saved;
	} else {
	    HTPutScriptEnv("SCRIPT_NAME", HTReqArgPath);
	}
    }

    if (HTClientHostName)
	HTPutScriptEnv("REMOTE_HOST", HTClientHostName);

    HTPutScriptEnv("REMOTE_ADDR", HTClientHost);

    if (req->scheme != HTAA_NONE && HTUser && HTUser->username) {
	HTPutScriptEnv("AUTH_TYPE", HTAAScheme_name(req->scheme));
	HTPutScriptEnv("REMOTE_USER", HTUser->username);
    }
    else {
	HTPutScriptEnv("AUTH_TYPE", NULL);
	HTPutScriptEnv("REMOTE_USER", req->from);
    }

    if (remote_ident && strcmp(remote_ident,"unknown"))
	HTPutScriptEnv("REMOTE_IDENT", remote_ident);
    else
	HTPutScriptEnv("REMOTE_IDENT", NULL);

    if (req->content_type)
	HTPutScriptEnv("CONTENT_TYPE", HTAtom_name(req->content_type));
    if (req->content_length > 0  ||  req->content_type) {
	char buf[20];
	sprintf(buf, "%d", req->content_length);
	HTPutScriptEnv("CONTENT_LENGTH", buf);
    }

    if (HTReferer)
        HTPutScriptEnv("REFERER_URL", HTReferer);
    else
        HTPutScriptEnv("REFERER_URL", NULL);

    if (cgi_script) {
	HTList * list = hbuf_http_env_vars(req);
	HTList * cur = list;
	char * envvar;

	while ((envvar = (char*)HTList_nextObject(cur))) {

	    if (++cur_cgi_var_cnt > CGI_ENV_VAR_CNT) {
		CTRACE(stderr, "INTERNAL.... %s\nExplanation. %s %d\n",
		       "HTScriptEnv: Too many CGI env vars!\n",
		       "Adjust CGI_ENV_VAR_CNT macro, currently",
		       CGI_ENV_VAR_CNT);
		break;
	    }

#ifdef VMS
	    {
		char * eq = strchr(envvar, '=');
		char * var = (char *)malloc(strlen(envvar) + 12);
		if (!var) outofmem(__FILE__, "HTScriptEnv");

		if (eq) *eq++ = 0;
		sprintf(var, "WWW_%s == \"%s\"", envvar, eq ? eq : "");
		free(envvar);
		envvar = var;
	    }
#endif /* VMS */

	    CTRACE(stderr, "Environ..... %s\n", envvar);
	    *(script_env_tail++) = envvar;
	    *script_env_tail = NULL;
	}
	HTList_delete(list);
    }

    return script_env;
}



/* PUBLIC							HTCallScript()
**		CALL A SCRIPT AND SEND RESULTS BACK TO CLIENT
** ON ENTRY:
**	req		the request.
**	req->output_stream
**			stream to dump the reply.
**	HTReqScript	script to call.
**	HTScriptPathInfo
**			path info after script name part in URL.
**	HTReqArgKeywords
**			search keywords/form fields
**
**				/htbin/foo/bar/x/y
**
**			is called as:
**
**			    <HTBinDir>/foo /bar/x/y keywords...
**
**			and:
**				/htbin/foo
**
**			is called as:
**
**			    <HTBinDir>/foo '' keywords...
**
** ON EXIT:
**	returns		HT_LOADED on success.
*/
#ifndef VMS
PUBLIC int HTCallScript ARGS1(HTRequest *, req)
{
    static char *msg = NULL;		/* Auto-freed */
    int status = HT_LOADED;
    char *nph;
    char *suff;
    char ** argv = NULL;
    char ** envp = NULL;
    BOOL nph_script = NO;

    FREE(msg);		/* From previous call */

    if (!req) {
	CTRACE(stderr, "INTERNAL.... HTCallScript: internal error: req NULL\n");
	return HT_INTERNAL;
    }

    /*
    ** Check that it really is a /htbin request
    */
    if (!HTReqScript) {
	CTRACE(stderr,
	       "INTERNAL.... HTCallScript called when HTReqScript NULL (%s)\n",
	       HTReqArg ? HTReqArg : "-null-");
	return HTLoadError(req, 400, "Bad script execution request");
    }

    CTRACE(stderr, "Resolving... calling scheme for \"%s\"\n", HTReqScript);

    if ((nph = strrchr(HTReqScript, '/')) &&
	!strncasecomp(nph, "/nph-", 5))
	nph_script = YES;
    else
	nph_script = NO;

    cgi_script = NO;	
    if ((suff = strrchr(HTReqScript, '.')) && 0==strcasecomp(suff, ".pp")) {
	/* URL contains directly .pp suffix */
	CTRACE(stderr, "Easy...... URL contains .pp, this is clear\n");
	if (-1 == access(HTReqScript, X_OK)) {
	    if (!(msg = (char*)malloc(strlen(HTReqScript) + 50)))
		outofmem(__FILE__, "HTCallScript");
	    sprintf(msg, "Bad preparse script request -- '%s' not executable",
		    HTReqScript);
	    CTRACE(stderr, "ERROR....... %s\n", msg);
	    return HTLoadError(req, 500, msg);
	}
    }
    else if (0==access(HTReqScript, X_OK)) { /* No suffix in URL nor filename*/
	CTRACE(stderr, "Found out... no .pp suffix, directly executable\n");
	cgi_script = YES;
    }
    else {	/* Try replacing unknown suffix with .pp */
	char *test = (char*)malloc(strlen(HTReqScript) + 4);
	char *s;
	CTRACE(stderr, "Trying...... to find executable by appending .pp\n");
	strcpy(test, HTReqScript);
	s = strrchr(test, '.');
	strcat(test, ".pp");	/* Try appending .pp */
	CTRACE(stderr, "Trying...... \"%s\"\n", test);
	if (-1==access(test, X_OK)) {  /* Then try replacing suffix with .pp */
	    if (s) {
		*s = 0;
		strcat(s, ".pp");
		CTRACE(stderr, "Bad luck.... now trying \"%s\"\n", test);
		if (-1==access(test, X_OK)) {	/* INVALID */
		    if (!(msg = (char*)malloc(3*strlen(test) + 100)))
			outofmem(__FILE__, "HTCallScript");

		    sprintf(msg,
	"Bad script request -- none of '%s' and '%s.pp' is executable",
			    HTReqScript, test );
		    free(test);
		    CTRACE(stderr, "ERROR....... %s\n", msg);
		    return HTLoadError(req, 500, msg);
		}
	    }
	    else {
		if (!(msg = (char*)malloc(2*strlen(test) + 100)))
		    outofmem(__FILE__, "HTCallScript");
		sprintf(msg,
		"Bad script request -- neither '%s' nor '%s' is executable",
			HTReqScript, test);
		free(test);
		CTRACE(stderr, "ERROR....... %s\n", msg);
		return HTLoadError(req, 500, msg);
	    }
	}
	CTRACE(stderr, "Yippee...... that was OK\n");
	free(HTReqScript);
	HTReqScript = test;
    }
    CTRACE(stderr, "Scheme...... %s\n",
	   cgi_script ? CGI_VERSION : "CERN-PreParsed");

    envp = HTScriptEnv(req);
    argv = HTScriptArgv(req);
    if (!*argv[0]) {
        char *msg = "Bad script request -- no script name";
        CTRACE(stderr, "ERROR....... %s\n", msg);
        return HTLoadError(req, 400, msg);
    }

    {
	int pin[2];
	int pout[2];
	int pid;

	if (pipe(pin) < 0  ||  pipe(pout) < 0) {
	    return HTLoadError(req, 500,
			       "Internal error: Failed to create pipe");
	}
	pid = fork();
	if (pid < 0) {	/* fork() failed */
	    char msg[100];
	    sprintf(msg, "Internal error: fork() failed with %s script",
		    (nph_script ? "NPH-" : HTMethod_name(req->method)));
	    return HTLoadError(req, 500, msg);
	}

	if (!pid) {
	    /*
	    ** Child process
	    */
	    CTRACE(stderr, "Child....... is alive\n");

	    /*
	    ** Standalone server, redirect socket to stdin/stdout.
	    */
	    CTRACE(stderr, "Child....... doing IO redirections\n");
	    if (req->content_length > 0) {	/* Need to give stdin */
		if (pin[0] != 0) {
		    CTRACE(stderr, "Child....... stdin from input pipe\n");
		    dup2(pin[0], 0);
		    close(pin[0]);
		    CTRACE(stderr, "Child....... done\n");
		}
	    }
	    if (nph_script) {
		if (HTSoc != 0) {
		    CTRACE(stderr, "Child....... stdout directly to socket\n");
		    dup2(HTSoc, 1);
		    CTRACE(stderr, "Child....... done\n");
		}
	    }
	    else {
		if (pout[1] != 1) {
		    CTRACE(stderr, "Child....... stdout to pipe\n");
		    dup2(pout[1], 1);
		    close(pout[1]);
		    CTRACE(stderr, "Child....... done\n");
		}
	    }
	    CTRACE(stderr,
		   "Child....... Standalone specific IO redirections done.\n");
	    CTRACE(stderr, "Child....... redirecting stderr to stdout, and then doing execve()\n");
	    dup2(1, 2);	/* Redirect stderr to stdout */
	    if (-1 == execve(HTReqScript, argv, envp)) {
		CTRACE(stderr, "ERROR....... execve() failed\n");
		HTLoadError(req, 500, "Internal error: execve() failed");
		exit(1);
	    }
	    /* NEVER REACHED */
	}

	/*
	** Parent process
	*/
	close(pin[0]);
	close(pout[1]);

	script_timeout_on(pid);

	if (req->content_length > 0  ||  req->content_type) {
	    int remain = req->content_length;
	    int i = remain;
	    char * buf;

	    /*
	     * For some reason POST doesn't work unless we ignore
	     * SIGPIPE during write.
	     */
	    ignore_sigpipes = YES;

	    while (remain>0 && (buf = HTInputSocket_getBlock(req->isoc, &i))) {
		int status = write(pin[1], buf, i);
#if 0
		if (TRACE) {
		    buf[i] = 0;
		    fprintf(stderr, "\nWHEN REMAINING %d...\n", remain);
		    fprintf(stderr, "READ %d... \"%s\"\n", i, buf);
		    fprintf(stderr, "WROTE %d... \"%s\"\n", status, buf);
		    fprintf(stderr, "REMAIN %d...\n\n", remain - i);
		}
#endif
		if (status < 0) {
		    CTRACE(stderr,
			   "ERROR....... Can't write to child's input pipe\n");
		    break;
		}
		remain -= i;
		i = remain;
	    }
	    /*
	     * Kludge that will make sure that non-CGI-compliant scripts
	     * expecting a newline after the body get it and won't hang.
	     */
	    write(pin[1],"\r\n",2);

	    ignore_sigpipes = sigpipe_caught = NO;
	}

	if (!nph_script) {
	    FILE *fp = fdopen(pout[0], "r");

	    if (!fp) {
		CTRACE(stderr, "ERROR....... Can't read script output pipe\n");
		return HTLoadError(req, 500,
		       "Internal error: can't read script output pipe");
	    }
	    status = HTLoadScriptResult(fp, req->output_stream, req);
	    fclose(fp);
	}
	else {
	    if (req->output_stream) {
		(*req->output_stream->isa->_free)(req->output_stream);
		req->output_stream = NULL;
	    }
	    out.status_code = 0;	/* Status not known for nph-scripts */
	    status = HT_LOADED;
	}

#ifdef NeXT
	wait((union wait *)0);
#else /* !NeXT */
#if ( defined(POSIXWAIT) || defined(__hpux)) && !defined(Mips)
#define USE_WAITPID
#endif /* POSIXWAIT / __hpux / !Mips */

	{
#ifdef USE_WAITPID
            waitpid(pid, NULL, 0);
#else /* ! USE_WAITPID */
	    int status;				   /* union wait is obsolete */
	    wait3(&status, 0, NULL);
#endif /* USE_WAITPID */
	}
#endif /* NeXT */

	timeout_off();

	return status;
    }
}

#else /* VMS */

PRIVATE int HTResolveCallingScheme ARGS2(HTRequest *, req,
                                         BOOL *, nph_script)
{
static char *msg = NULL;		/* Auto-freed */
char *test;
char *nph;
char *suff;

    FREE(msg);		/* From previous call */

    CTRACE(stderr, "Resolving... calling scheme for \"%s\"\n", HTReqScript);

    if ((nph = strrchr(HTReqScript, '/')) &&
	!strncasecomp(nph, "/nph-", 5))
	nph_script = YES;
    else
	nph_script = NO;

    cgi_script = NO;
    /* if request ends in .pp and file exists, then pre-parsed. If file does
       not exist, error */
    if ( (suff = strrchr(HTReqScript, '.')) && 
         (0==strcasecomp(suff, ".pp")) ) {
	CTRACE(stderr, "Easy...... URL contains .pp, this is clear\n");
	if (-1 == access(HTReqScript, X_OK)) {
	    if (!(msg = (char*)malloc(strlen(HTReqScript) + 50)))
		outofmem(__FILE__, "HTCallScript");
	    sprintf(msg, "Bad preparse script request -- '%s' not executable",
		    HTReqScript);
	    CTRACE(stderr, "ERROR....... %s\n", msg);
	    HTLoadError(req, 500, msg);
            return(0);
	}
    } else
    /* if request ends in extension ending in _pp and file exists, then pre-parsed. If file does
       not exist, error */
    if ( (strrchr(HTReqScript, '.')) && 
         (suff = strrchr(HTReqScript, '_')) && 
         (0==strcasecomp(suff, "_pp")) ) {
	CTRACE(stderr, "Easy...... URL contains .xxxx_pp, this is clear\n");
        if (-1 == access(HTReqScript, X_OK)) {
	    if (!(msg = (char*)malloc(strlen(HTReqScript) + 50)))
	        outofmem(__FILE__, "HTCallScript");
	    sprintf(msg, "Bad preparse script request -- '%s' not executable",
		    HTReqScript);
	    CTRACE(stderr, "ERROR....... %s\n", msg);
	    HTLoadError(req, 500, msg);
            return(0);
   	}
    } else
    /* if request specifies existing file, then CGI. */
    if (0==access(HTReqScript, X_OK)) { /* No suffix in URL nor filename*/
	CTRACE(stderr, "Found out... no .pp suffix, directly executable\n");
	cgi_script = YES;
    } else 
    /* check for extension */
    if (strrchr(HTReqScript, '.') == NULL) {
    /* no extension */
       /* if request had no extension, add .pp and if file exists, then pre-parsed. */
       test = (char*)malloc(strlen(HTReqScript) + 5);

       strcpy(test, HTReqScript);
       strcat(test, ".pp");	/* Try appending .pp */
       CTRACE(stderr, "Trying...... No extension, trying '%s'\n",test);
       if (-1 == access(test, X_OK)) {
          /* if request had no extension, add .com and if file exists, then CGI. */
          strcpy(test, HTReqScript);
          strcat(test, ".com");	/* Try appending .pp */
          CTRACE(stderr, "Trying...... No extension, trying '%s'\n",test);
          if (-1 == access(test, X_OK)) {

             if (!(msg = (char*)malloc(3*(strlen(HTReqScript)) + 70)))
	             	outofmem(__FILE__, "HTCallScript");
        		    sprintf(msg,
	     "Bad script request -- none of '%s', '%s.pp', '%s.com' is executable",
	        		    HTReqScript, HTReqScript, HTReqScript);
	     CTRACE(stderr, "ERROR....... %s\n", msg);
             free(test);
             HTLoadError(req, 500, msg);
             return(0);
          } /* no access to .com file */
          cgi_script = YES;
       } /* no access to .pp file */

       CTRACE(stderr, "Yippee...... that was OK\n");
       free(HTReqScript);
       HTReqScript = test;
    } else {
    /* has extension */
       /* if request has extension, add _pp and if file exists, then pre-parsed. */
       test = (char*)malloc(strlen(HTReqScript) + 4);

       strcpy(test, HTReqScript);
       strcat(test, "_pp");	/* Try appending _pp */
       CTRACE(stderr, "Trying...... Has extension, trying '%s'\n",test);
       if (-1 == access(test, X_OK)) {
          /* if request has extension, replace by .pp and if file exists, then pre-parsed. */
          suff = strrchr(test,'.');
          *suff = '\0';
          strcat(test, ".pp");
          CTRACE(stderr, "Trying...... Has extension, trying replacing it, trying '%s'\n",test);                    
          if (-1 == access(test, X_OK)) {
             /* otherwise fail. */
             if (!(msg = (char*)malloc(2*(strlen(HTReqScript)) + 70)))
         	             	outofmem(__FILE__, "HTCallScript");
	     sprintf(msg,
	        "Bad script request -- none of '%s_pp', '%s' is executable",
		     	    HTReqScript, test);
	     CTRACE(stderr, "ERROR....... %s\n", msg);
             free(test);
	     HTLoadError(req, 500, msg);         
             return(0);
          } /* no access to replace .pp file */
       } /* no access to _pp file */

       CTRACE(stderr, "Yippee...... that was OK\n");
       free(HTReqScript);
       HTReqScript = test;

    } /* has extension */
    
    CTRACE(stderr, "Scheme...... %s\n",
	   cgi_script ? CGI_VERSION : "CERN-PreParsed");
    return(1);
}

PRIVATE int HTConvArgvToEnv ARGS1(char **, argv)
{
       char count[20];
       int keycnt = 0;
       int i = 1;
       if (!cgi_script)
          i = 2;

       for ( ; argv[i] ; i++)
       {
          char key[20];
          char *src, *dst;
          char value[256];
          int linecnt = 1;

          keycnt++;
          src = argv[i];
          dst = &value;
          while (*src)
          {
             if (*src == '\"')
             {  /* escape double quote by double double quote */
                /* "" converts into "" for fprintf */
                *(dst++) = '\"';
                *(dst++) = '\"';
             }
             else
             { 
                if (*src == '\n')
                {  /* newline, split up keyword... */
                   *(dst++) = '\0';
                   sprintf(key,"KEY_%d_%d",keycnt,linecnt);
                   HTPutScriptEnv(key,value);
                   linecnt++;
                   dst = &value;
                }
                else
                {  /* ordinary character */
                   *(dst++) = *src;
                }
             }
             src++;
          }   

          /* end with a zero */
          *(dst++) = '\0';

          if (linecnt == 1)
          {  /* one line */
             sprintf(key,"KEY_%d",keycnt);
             HTPutScriptEnv(key,value);
          }
          else
          {  /* multi line */

             /* add last one only if not empty */
             if (*value != '\0')
             {
                sprintf(key,"KEY_%d_%d",keycnt,linecnt);
                HTPutScriptEnv(key,value);
                linecnt++;
             } 
             sprintf(key,"KEY_%d_COUNT",keycnt);
             sprintf(count,"%d",linecnt-1);
             HTPutScriptEnv(key,count); 
          }
       }
       sprintf(count,"%d",keycnt);
       HTPutScriptEnv("KEY_COUNT",count);      

   return(1);
}




PRIVATE int HTWriteEnvToFile ARGS3(char **, envp,
                                   char **, argv, 
                                   char *, tmpscriptfile)
{
   char *src, *dst, param[256];
   FILE *fp = fopen(tmpscriptfile, "w");
   int length, plen, numparam;
   int i;

   if (!fp) return(0);
   
   /* disable all crap messages */
   if ((TRACE) && (trace_all == YES))
      fprintf(fp,"$ set verify\n");
   else
      fprintf(fp,"$ set noverify\n");

   /* set default to the default data tree root, if specified */
   if (sc.server_root) {
      char *Server_Root;
      if (strchr(sc.server_root, '/') != NULL)
         Server_Root = HTVMS_name("", sc.server_root);
      else
	 Server_Root = sc.server_root;
      fprintf(fp,"$ set default %s", Server_Root);
   }

   /* write out environment to set up symbols in DCL */
   for (i=0; i<cur_cgi_var_cnt;i++)
   {
      if ((strlen(envp[i])+4) > MAX_DCL_COMMAND_LENGTH)
      {  /* shorten */
         envp[i][MAX_DCL_COMMAND_LENGTH-4] = '\0';
         envp[i][MAX_DCL_COMMAND_LENGTH-3] = '\"';
      }
      fprintf(fp,"$ %s\n",envp[i]);
   }

   /* in case of a cgi-form-script, we only give one argument */
   if (HTReqArgKeywords && HTReqArgKeywords[0] &&
       (strchr(HTReqArgKeywords, '=')) && (cgi_script))
      argv[1] = NULL;

   /* write out definitions of the mailboxes..., and the actual command */
   fprintf(fp,"$ open/read www_in www_mbox_in\n");
   fprintf(fp,"$ open/write www_out www_mbox_out\n");
   fprintf(fp,"$ define/nolog sys$error www_out\n");
   if ((TRACE) && (trace_all == YES))
      fprintf(fp ,"$ @%s /output=www_out", HTVMS_name("",HTReqScript));
   else
   {
      fprintf(fp,"$ define/nolog sys$output www_out\n");
      fprintf(fp,"$ @%s", HTVMS_name("",HTReqScript));
   }

   length = 24 + strlen(HTVMS_name("",HTReqScript));
   numparam = 1;
   for(i=1; argv[i]; i++)
   {
      /* escape " as """" (e.g. stop ", "" for ", start ") */
      for (src = argv[i],dst=param ; *src ;)
      {
         if (*src == '\"')
         {  /* quotes */
            *dst = '\"'; dst++;
            *dst = '\"'; dst++;
            *dst = '\"'; dst++;
            *dst = '\"'; dst++;
            src++;
         }
         else if (*src == '\n')
         {
            *dst = '%'; dst++;
            *dst = '0'; dst++;
            *dst = 'a'; dst++;
            src++;
         } 
         else
         {  /* normal characters */
            *dst = *src; dst++;
            src++;
         }
      } 
      *dst = '\0';
      plen = strlen(param);
      if ( ((plen + length) <= MAX_DCL_COMMAND_LENGTH) &&
           (numparam <= MAX_DCL_PARAMETERS) )
      {  /* only write when command line not too long yet */
         fprintf(fp, " \"%s\"", param);
         length += plen;
         numparam++;
      }
   }
   fprintf(fp,"\n");
   fprintf(fp,"$ set noverify\n");
   fprintf(fp,"$ deassign sys$error\n");
   if ((TRACE) && (trace_all == YES))
      fprintf(fp,"$ deassign sys$output\n");
   fprintf(fp,"$ close/nolog www_in\n");
   fprintf(fp,"$ close/nolog www_out\n");

   fclose(fp);
   return(1);
}

PRIVATE int HTLoadNPHScriptResult ARGS3(long, 		OutChan,
					HTStream *,     sink,
					HTRequest *,	req)
{
HTStreamClass targetClass;
char mbox_buffer[INPUT_BUFFER_SIZE+2];
long Function;
long Status;
struct _iosb
{
   short status;
   short length;
   long devspec;
} iosb;

   targetClass = *(sink->isa);
   
   /* copy everything in output mailbox into sink */
   /* read the script results */
   Function = IO$_READVBLK;
   Status = SYS$QIOW(0, OutChan, Function, &iosb, 0, 0, mbox_buffer, INPUT_BUFFER_SIZE, 0, 0, 0, 0);
   if (!(Status & 0x01)) exit(Status);

   while (iosb.status == SS$_NORMAL) 
   {
      mbox_buffer[iosb.length] = '\n';
      mbox_buffer[iosb.length+1] = '\0';
/*
      printf("Received... %s\n",mbox_buffer);
*/
      (*targetClass.put_block)(sink,mbox_buffer,iosb.length+1);

      Status = SYS$QIOW(0, OutChan, Function, &iosb, 0, 0, mbox_buffer, INPUT_BUFFER_SIZE, 0, 0, 0, 0);
      if (!(Status & 0x01)) exit(Status);
   }


   (*targetClass._free)(sink);
   out.status_code = 0;
   return(1);
}


PRIVATE int HTRunScript ARGS3(HTRequest *, req,
                              char *, tmpscriptfile,
                              BOOL, nph_script)
{
extern char * HTSpawnInit;
int Status, Result;
long ExecChan, InChan, OutChan, ErrorChan;
$DESCRIPTOR(MboxInDesc,"WWW_MBOX_IN");
$DESCRIPTOR(MboxOutDesc,"WWW_MBOX_OUT");
struct _iosb
{
   short status;
   short length;
   long devspec;
} iosb;
long maxmsg = req->content_length;
long bufquo = req->content_length;
long Function;
char mbx_name[20], error_mbx_name[20], command[256], script_err[20], script_out[20];

struct dsc$descriptor_s
          d_err = {0, DSC$K_DTYPE_T, DSC$K_CLASS_S, 0};
struct dsc$descriptor_s
          d_out = {0, DSC$K_DTYPE_T, DSC$K_CLASS_S, 0};
   
$DESCRIPTOR(d_image, "sys$system:loginout.exe");
struct dsc$descriptor_s
          d_input = {0, DSC$K_DTYPE_T, DSC$K_CLASS_S, 0};
long privs[2] = {PRV$M_NETMBX|PRV$M_TMPMBX, 0};
long unit, pid;
struct itemlist3 {
         short buflen;
         short itmcode;
         int *bufadr;
         short *retadr;
   } itmlst[2] = {
         {4, DVI$_UNIT, (int *) &unit, 0},
         {0, 0, 0, 0}
   };

   /* set debug output... */
   if ((TRACE) && (trace_all == YES))
   {
      strcpy(script_err,"SCRIPT.ERR");
      strcpy(script_out,"SCRIPT.OUT");
   }
   else
   {
      strcpy(script_err,"NL:");
      strcpy(script_out,"NL:");
   }
   d_err.dsc$w_length = (short) strlen(script_err);
   d_err.dsc$a_pointer = script_err;
   d_out.dsc$w_length = (short) strlen(script_out);
   d_out.dsc$a_pointer = script_out;

   /* open input mailbox, this is where the input to the script goes */
   Status = SYS$CREMBX(0, &InChan, 0, 0, 0, PSL$C_USER, &MboxInDesc, 0);
   if (!(Status & 0x01)) exit(Status);

   /* open output mailbox, this is where the output of the mailbox comes */
   Status = SYS$CREMBX(0, &OutChan, 0, 0, 0, PSL$C_USER, &MboxOutDesc, 0);
   if (!(Status & 0x01)) exit(Status);

   /*
   ** Copy remaining input to the input mailbox, if there is any.
   ** We already got it all if it's a POST.
   ** Break it up into INPUT_BUFFER_SIZE records, if
   ** longer, so we don't hang or ACCVIO on the mailbox.
   */
   if (req->method != METHOD_POST &&
       (req->content_length > 0 || req->content_type))
   {
      int remain = req->content_length;
      int i = remain, j, len;
      char *buf;

      Function = IO$_WRITEVBLK | IO$M_NOW;
      while (remain > 0 && (buf = HTInputSocket_getBlock(req->isoc, &i)))
      {      
	 len = i;
	 while (len > 0)
	 {
	     if (len > INPUT_BUFFER_SIZE)
    	         j = INPUT_BUFFER_SIZE;
 	     else
	         j = len;

             Status = SYS$QIOW(0, InChan, Function, &iosb, 0, 0,
	     		       buf+i-len, j, 0, 0, 0, 0);
             if (!(Status & 0x01)) exit(Status);

	     len -= j;
	 }
         remain -= i;
         i = remain;
      }
   }

   /* write an end of file */
   Function = IO$_WRITEOF | IO$M_NOW;
   Status = SYS$QIOW(0, InChan, Function, &iosb, 0, 0, 0, 0, 0, 0, 0, 0);
   if (!(Status & 0x01)) exit(Status);

   /* Create a mailbox for passing DCL commands to the subprocess. */
   Status = SYS$CREMBX(0, &ExecChan, 0, 0, 0, 0, 0, 0);
   if (!(Status & 0x01)) exit(Status);

   /*  Identify the mailbox for the d_input descriptor. */
   Status = SYS$GETDVIW(0, ExecChan, 0, itmlst, 0, 0, 0, 0);
   if (!(Status & 0x01)) exit(Status);
   sprintf(mbx_name, "_MBA%d:", unit);
   d_input.dsc$w_length = (short) strlen(mbx_name);
   d_input.dsc$a_pointer = mbx_name;

   /*  Create the subprocess with only TMPMBX and NETMBX privileges. */
   Status = SYS$CREPRC(&pid, &d_image, &d_input, &d_out, &d_err,
		&privs, 0, 0, 4, 0, 0, 0);
   if (!(Status & 0x01)) exit(Status);

   /* 
    *  The subprocess doesn't execute SYLOGIN.COM, and it's F$MODE()
    *  is "OTHER", so pass it the EGREP foreign command, and other
    *  symbols and logicals you want the subprocess to have, via a
    *  command file.  But make sure the subprocess can execute the
    *  command file.
    */
   if (!HTSpawnInit)
   	  StrAllocCopy(HTSpawnInit, "httpd_dir:spawninit");
   if (access(HTSpawnInit, 1) == 0) {
          sprintf(command, "$ @%s", HTSpawnInit);
          Status = SYS$QIOW(0, ExecChan, IO$_WRITEVBLK, &iosb, 0, 0, command,
                       strlen(command), 0, 0, 0, 0);
          if (!(Status & 0x01)) exit(Status);
   }

   /*  Mail the server's DCL command to the subprocess. */
   sprintf(command,"$ @%s",tmpscriptfile);
   Status = SYS$QIOW(0, ExecChan, IO$_WRITEVBLK, &iosb, 0, 0, command,
                       strlen(command), 0, 0, 0, 0);
   if (!(Status & 0x01)) exit(Status);

   /* load the result */
   if (nph_script)
      HTLoadNPHScriptResult(OutChan, req->output_stream, req);
   else
      Result = HTLoadScriptResult((FILE *)OutChan, req->output_stream, req);     

   /* delete the subprocess, we can do this because the subprocess closed its
      output mailbox, so it has finished... */
   Status = SYS$DELPRC(&pid,0);
   if (!(Status & 0x01)) exit(Status);

   /* delete the mailboxes !!! */
   Status = SYS$DELMBX(ExecChan);
   if (!(Status & 0x01)) exit(Status);
   Status = SYS$DASSGN(ExecChan);
   if (!(Status & 0x01)) exit(Status);

   Status = SYS$DELMBX(InChan);
   if (!(Status & 0x01)) exit(Status);
   Status = SYS$DASSGN(InChan);
   if (!(Status & 0x01)) exit(Status);

   Status = SYS$DELMBX(OutChan);
   if (!(Status & 0x01)) exit(Status);
   Status = SYS$DASSGN(OutChan);
   if (!(Status & 0x01)) exit(Status);

   if (Result == HT_REDIRECTION_ON_FLY)
       return(Result);
   else
       return(1);
}


PUBLIC int HTCallScript ARGS1(HTRequest *, req)
{
    static char *tmpscriptfile = NULL;	/* Auto-freed */

    int status;
    char ** argv = NULL;
    char ** envp = NULL;
    BOOL nph_script = NO;

    FREE(tmpscriptfile);

    if (!req) {
	CTRACE(stderr, "INTERNAL.... HTCallScript: internal error: req NULL\n");
	return HT_INTERNAL;
    }

    /*
    ** Check that it really is a /htbin request
    */
    if (!HTReqScript) {
	CTRACE(stderr,
	       "INTERNAL.... HTCallScript called when HTReqScript NULL (%s)\n",
	       HTReqArg ? HTReqArg : "-null-");
	return HTLoadError(req, 400, "Bad script execution request");
    }

    /*
    ** Get tmpfile names
    */
    if (HTScratchDir) {
    	/* Also frees the one from previous call */
        StrAllocCopy(tmpscriptfile, HTScratchDir);
        StrAllocCat(tmpscriptfile, tmpnam(NULL));
    } else {
        /* Also frees the one from previous call */
        StrAllocCopy(tmpscriptfile, tmpnam(NULL));
    }
    StrAllocCat(tmpscriptfile,".TMP");

   if (!HTResolveCallingScheme(req, &nph_script))
      return(HT_LOADED);

   if (req->method == METHOD_POST &&
       (req->content_length > 0 || req->content_type)) {
       /*
       ** Get the QUERY_CONTENT now, so we can parse it
       ** and pass it to the script via WWW_KEY symbols
       */
       int remain = req->content_length;
       int i = remain;
       char *buf, *cp;
       BOOLEAN first = TRUE;

       while (remain > 0 && (buf = HTInputSocket_getBlock(req->isoc, &i))) {
           if (first) {
               StrAllocCopy(HTReqArgKeywords, buf);
	       first = FALSE;
	   } else {
	       StrAllocCat(HTReqArgKeywords, buf);
	   }

           remain -= i;
           i = remain;
        }
        if (HTReqArgKeywords && HTReqArgKeywords[0] != '\0')
	    while (iscntrl(HTReqArgKeywords[strlen(HTReqArgKeywords)-1]))
	        HTReqArgKeywords[strlen(HTReqArgKeywords)-1] = '\0';
    }

   envp = HTScriptEnv(req);
   argv = HTScriptArgv(req);
   if (!*argv[0]) {
       char *msg = "Bad script request -- no script name";
       CTRACE(stderr, "ERROR....... %s\n", msg);
       return HTLoadError(req, 400, msg);
   }
   HTConvArgvToEnv(argv);

   if (!HTWriteEnvToFile(envp,argv,tmpscriptfile))
   {
	CTRACE(stderr,
	       "CallScript... HTCallScript could not create temporary script file (%s)\n",
	       tmpscriptfile);
	return HTLoadError(req, 400, "Could not execute script. Server error.");
   }

   status = HTRunScript(req,tmpscriptfile,nph_script);

   /* delete temporary file */
#ifndef SCRIPT_DEBUG
   delete(tmpscriptfile);
#endif

   if (status == HT_REDIRECTION_ON_FLY)
       return(status);
   else
       return(HT_LOADED);
}

#endif /* VMS */
