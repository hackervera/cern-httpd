
/* PROGRAM							cgiutils.c
**	Utility program for CGI scripts to generate HTTP header
**	fields for response or the entire HTTP response.
**
** USAGE:
**	See usage() function, or start with no parameters.
**
** AUTHORS:
**	AL	Ari Luotonen	luotonen@dxcern.cern.ch
**
** HISTORY:
**	11 Mar 94  AL	Written in a terrible hurry just before
**			going to Italy...
**
** BUGS:
**
*/

#define PROGNAME	"cgiutils"
#define PROGVERSION	"1.0"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "HTUtils.h"
#include "tcp.h"
#include "HTSUtils.h"

#ifdef VMS
#include "HTStyle.h"
PUBLIC char *HTAppName = "cgiutils";
PUBLIC char *HTAppVersion = "1.0";
PUBLIC HTStyleSheet *styleSheet;
#endif /* VMS */


PRIVATE time_t cur_t;

PRIVATE void usage NOARGS
{
    fprintf(stderr, "\n\t\t\t\t\t%s version %s  (c) CERN 1994\n",
	    PROGNAME, PROGVERSION);
    fprintf(stderr,
	    "Utility program for generating HTTP response from CGI scripts\n\n");
    fprintf(stderr,"Usage:\t%s -version\n", PROGNAME);
    fprintf(stderr,"\t%s [-verbose] [-nodate] [-noel] -opt param -opt param ...\n", PROGNAME);
    fprintf(stderr,"  -verbose      Verbose output\n");
    fprintf(stderr,"  -nodate       Don't produce Date: header\n");
    fprintf(stderr,"  -noel         Don't print empty line after headers\n");
    fprintf(stderr,"  -status nnn   Give full HTTP response with nnn as status code\n");
    fprintf(stderr,"  -reason xxx   Use xxx reason line instead of default for nnn status\n");
    fprintf(stderr,"  -ct xxx/yyy   Content-Type:\n");
    fprintf(stderr,"  -ce xxx       Content-Transfer-Encoding:\n");
    fprintf(stderr,"  -cl xxx       Content-Language:\n");
    fprintf(stderr,"  -length nnn   Content-Length:\n");
    fprintf(stderr,"  -uri xxx      URI:\n");
    fprintf(stderr,"  -expires time Specify how long document will be up-to-date\n\n");
    fprintf(stderr,"  Time is specified as a combination of:\n");
    fprintf(stderr,"\tn secs, n mins, n hours, n days, n weeks, n months, n years\n");
    fprintf(stderr,"  Or:\tnow\tmeaning immediate expiry\n\n");
    fprintf(stderr,"  If there is a header that can't be specified otherwise, use:\n");
    fprintf(stderr, "\t-extra \"xxx: yyy\"\n\n");
    exit(1);
}

PRIVATE void fatal ARGS2(char *,	msg,
			 char *,	param)
{
    fprintf(stderr, "\ncgiparse error: %s %s\n\n",
	    msg ? msg : "Oops, dunno what went wrong...",
	    param ? param : "");
    exit(2);
}

PRIVATE char * parse_expiry ARGS1(char *, str)
{
    time_t t;
    char * s;
    char * ret = NULL;

    if (!strcmp(str,"now"))
	t = 0;
    else if (!parse_time(str,1,&t))
	fatal("Invalid time specifier:",str);
    t += cur_t;
    s = http_time(&t);
    StrAllocCopy(ret,s);
    return ret;
}


PUBLIC int main ARGS2(int,	argc,
		      char **,	argv)
{
    int i;
    int status = 0;
    char * reason = NULL;
    char * expires = NULL;
    char * location = NULL;
    char * content_type = NULL;
    char * content_encoding = NULL;
    char * content_language = NULL;
    char * content_length = NULL;
    char * uri = NULL;
    char * extras = NULL;
    char * server_protocol = getenv("SERVER_PROTOCOL");
    char * server_software = getenv("SERVER_SOFTWARE");
    BOOL no_date = NO;
    BOOL no_empty_line = NO;

    if (!server_protocol) server_protocol = "HTTP/1.0";
    time(&cur_t);

    if (argc < 2) usage();
    for(i=1; i<argc; i++) {
	if (!strcmp(argv[i],"-version")) {
	    fprintf(stderr, "This is %s, version %s  (c) CERN 1994\n",
		    PROGNAME, PROGVERSION);
	    exit(0);
	}
	else if (!strncmp(argv[i],"-verbose",2))
	    WWW_TraceFlag = 1;
	else if (!strncmp(argv[i],"-nodate",4))
	    no_date = YES;
	else if (!strncmp(argv[i],"-noel",4))
	    no_empty_line = YES;
	else {
	    if (i+1 >= argc) usage();

	    if (!strncmp(argv[i],"-status",2))
		status = atoi(argv[i+1]);
	    else if (!strncmp(argv[i],"-reason",2))
		reason = argv[i+1];
	    else if (!strncmp(argv[i],"-expires",2))
		expires = parse_expiry(argv[i+1]);
	    else if (!strncmp(argv[i],"-location",3))
		location = argv[i+1];
	    else if (!strncmp(argv[i],"-ct",3))
		content_type = argv[i+1];
	    else if (!strncmp(argv[i],"-length",3))
		content_length = argv[i+1];
	    else if (!strncmp(argv[i],"-ce",3))
		content_encoding = argv[i+1];
	    else if (!strncmp(argv[i],"-cl",3))
		content_language = argv[i+1];
	    else if (!strncmp(argv[i],"-uri",2))
		uri = argv[i+1];
	    else if (!strncmp(argv[i],"-extra",2)) {
		char * buf = (char*)malloc(strlen(argv[i+1]) + 3);
		sprintf(buf, "%s\r\n", argv[i+1]);
		StrAllocCat(extras, buf);
		free(buf);
	    }
	    else fatal("Unknown option:",argv[i]);
	    i++;
	}
    }

    if (status) {
	if (!reason) reason = get_http_reason(status);
	printf("%s %d %s\r\nMIME-Version: 1.0\r\n",
	       server_protocol, status, reason);
	if (server_software)
	    printf("Server: %s\r\n", server_software);
	if (!no_date)
	    printf("Date: %s\r\n", http_time(&cur_t));
    }
    if (location) printf("Location: %s\r\n", location);
    if (uri) printf("URI: %s\r\n", uri);
    if (content_type) printf("Content-Type: %s\r\n", content_type);
    if (content_encoding) {
	printf("Content-Encoding: %s\r\n", content_encoding);
	printf("Content-Transfer-Encoding: %s\r\n", content_encoding);
    }
    if (content_language) printf("Content-Language: %s\r\n", content_language);
    if (content_length) printf("Content-Length: %s\r\n", content_length);
    if (expires) printf("Expires: %s\r\n", expires);
    if (extras) printf("%s", extras);
    if (!no_empty_line) printf("\r\n");

    return 0;
}

