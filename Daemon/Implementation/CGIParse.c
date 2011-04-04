
/* PROGRAM							cgiparse.c
**		PROGRAM FOR CGI/1.0 SCRIPTS TO PARSE QUERY_STRING
**		ENVIRONTMENT VARIABLE PASSED FROM THE SERVER, AND
**		TO READ CONTENT_LENGTH CHARACTERS FROM STDIN.
**
** USAGE:
**	See usage() function, or start with no parameters.
**
** AUTHORS:
**	AL	Ari Luotonen	luotonen@dxcern.cern.ch
**
** HISTORY:
**	 9 Jan 94  AL	Written on a melancholic Sunday evening.
**
** BUGS:
**
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "HTUtils.h"
#include "HTParse.h"


PRIVATE char * progname;

PRIVATE BOOL   parse_keywords = NO;
PRIVATE BOOL   parse_form = NO;
PRIVATE char * parse_value = NULL;
PRIVATE BOOL   echo = NO;
PRIVATE BOOL   init = NO;
PRIVATE char * separator = NULL;
PRIVATE char * prefix = NULL;
PRIVATE BOOL   count = NO;
PRIVATE int    nth = 0;
PRIVATE BOOL   quiet = NO;

#ifdef VMS
#include "HTStyle.h"
PUBLIC char *HTAppName = "CGIParse";
PUBLIC char *HTAppVersion = "Unknown";
PUBLIC HTStyleSheet *styleSheet;
#endif /* VMS */

PRIVATE void fatal2 ARGS3(int,	   exit_status,
			  char *,  part_1,
			  char *,  part_2)
{
    if (!quiet)
	fprintf(stderr, "%s: Error: %s %s\n", progname,
		part_1 ? part_1 : "",
		part_2 ? part_2 : "");
    exit(exit_status);
}


PRIVATE void fatal ARGS2(int,	 exit_status,
			 char *, err_msg)
{
    fatal2(exit_status, err_msg, "");
}



PRIVATE void usage NOARGS
{
    fprintf(stderr, "\n\t\t\t\t\t%s Version 1.1 CERN 1994\n\n%s\n%s\n%s\n\n",
	    progname,
	    "Program for CGI/1.0 scripts, to parse QUERY_STRING environment variable",
	    "(method GET) or standard input (method POST).  Can also be used to just",
	    "read CONTENT_LENGTH characters from stdin.");
    fprintf(stderr, "Usage:\n    %s %s\n    %s %s\n    %s %s\n    %s %s\n    %s %s\n\n",
	    progname, "-keywords",
	    progname, "-form [-sep <multivalue-separator>] -prefix <prefix>",
	    progname, "-value <fieldname> [-sep <multivalue-separator>]",
	    progname, "-read",
	    progname, "-init");
    fprintf(stderr, "Options:\n%s\n%s\n\n",
	    "  -sep xx    with -form      Specify multi-value separator, default \", \"",
	    "             with -value     The same, but default is newline");
    fprintf(stderr, "%s\n\n",
	    "  -prefix xx only with -form Set the shell variable suffix, default is FORM_");
    fprintf(stderr, "%s\n%s\n%s\n%s\n%s\n\n",
	    "  -count     with -keywords  Output the number of keywords",
	    "             with -form      Give the number of unique form fields",
	    "                             (multiple values counted as one)",
	    "             with -value xx  Give the number of multiple values of",
	    "                             field xx");
    fprintf(stderr, "%s\n%s\n%s\n%s\n\n",
	    "  -<num>     e.g. -2",
	    "             with -keyword   Give n'th keyword",
	    "             with -form      Give all values of n'th field",
	    "             with -value xx  Give n'th of multi-values of field xx");
    fprintf(stderr, "%s\n%s\n\n",
	    "  -quiet                     Suppress error messages (non-zero exit",
	    "                             status still indicates error)");
    fprintf(stderr, "%s\n\n",
	    "Options have one-character equivalents: -k -f -v -r -i -s -p -c -q");
    fprintf(stderr, "Exit statuses:\n    %s\n    %s\n    %s\n    %s\n    %s\n\n",
	    "0        Success",
	    "1        Illegal arguments",
	    "2        Environment not set correctly",
	    "3        Failed to get the information (no such field, QUERY_STRING",
	    "         contains keywords when form field values requested, etc)");
    exit(1);
}


PRIVATE void plusses_to_spaces ARGS1(char *, s)
{
    if (!s) return;

    for(; *s; s++) if (*s == '+') *s=' ';
}


PRIVATE BOOL valid_varname ARGS1(char *, varname)
{
    char *cur;

    if (!varname || !*varname) return NO;

    for(cur=varname; *cur; cur++) {
	if ((*cur < 'a'  ||  *cur > 'z') &&
	    (*cur < 'A'  ||  *cur > 'Z') &&
	    (*cur < '0'  ||  *cur > '9') &&
	    *cur != '_')
	    return NO;
    }
    return YES;
}


PRIVATE char * sh_escape ARGS1(char *, arg)
{
    static char *escaped = NULL;
    static int allocated = 0;
    int needed;
    char *cur;
    char *dest;

    if (!arg) return "";

    for(cur=arg,needed=3;  *cur;  cur++,needed++) {
#ifdef VMS
	if (*cur == '\"') needed += 1;
#else
	if (*cur == '\'') needed += 3;
#endif
    }

    if (allocated < needed) {
	allocated = needed;
	if (escaped) free(escaped);
	escaped = (char*)malloc(needed);
	if (!escaped) outofmem(__FILE__, "escaped-malloc");
    }

    dest = escaped;
#ifdef VMS
    *(dest++) = '\"';		/* Open quotes			*/
#else
    *(dest++) = '\'';		/* Open quotes			*/
#endif

    for(cur=arg;  *cur;  cur++) {
#ifdef VMS
	if (*cur == '\"') {
	    *(dest++) = '\"';	/* Create double quotes		*/
	    *(dest++) = '\"';	
	}
#else
	if (*cur == '\'') {
	    *(dest++) = '\'';	/* First close current quotes	*/
	    *(dest++) = '\\';	/* Escape quote with backslash	*/
	    *(dest++) = '\'';	/* Quote itself			*/
	    *(dest++) = '\'';	/* Open quotes again		*/
	}
#endif
	else *(dest++) = *cur;	/* Just copy the character	*/
    }
#ifdef VMS
    *(dest++) = '\"';		/* Close quotes			*/
#else
    *(dest++) = '\'';		/* Close quotes			*/
#endif
    *dest = 0;			/* Terminate string		*/

    return escaped;
}


PRIVATE int already_in_array ARGS2(char **, array,
				   char *, field)
{
    int i;

    if (!array || !field) return -1;

    for (i=0; array[i]; i+=2) {
	if (!strcmp(array[i], field))
	    return i+1;
    }
    return -1;
}


PUBLIC int main ARGS2(int,	argc,
		      char **,	argv)
{
    char * query_string = getenv("QUERY_STRING");
    char * request_method = getenv("REQUEST_METHOD");
    char * content_length = getenv("CONTENT_LENGTH");
    int    content_len = 0;
    BOOL   form_request = NO;
    char * cur;
    int    i;

    progname = argv[0];

    /*
    ** Parse command line options
    */
    if (argc < 2) usage();
    for (i=1; i<argc; i++) {
	if      (!strncmp(argv[i], "-keywords",	2)) parse_keywords = YES;
	else if (!strncmp(argv[i], "-form",	2)) parse_form = YES;
	else if (!strncmp(argv[i], "-value",	2)) {
	    if (++i >= argc) usage();
	    parse_value = argv[i];
	}
	else if (!strncmp(argv[i], "-read",	2)) echo = YES;
	else if (!strncmp(argv[i], "-init",	2)) init = YES;
	else if (!strncmp(argv[i], "-separator",2)) {
	    if (++i >= argc) usage();
	    separator = argv[i];
	}
	else if (!strncmp(argv[i], "-prefix",	2)) {
	    if (++i >= argc) usage();
	    prefix = argv[i];
	}
	else if (!strncmp(argv[i], "-count",	2)) count = YES;
	else if (!strncmp(argv[i], "-quiet",	2)) quiet = YES;
	else if (argv[i][0] == '-'  &&  isdigit(argv[i][1])) {
	    char *num = argv[i]+1;
	    nth = atoi(num);
	    if (nth <= 0) usage();
	}
	else usage();
    }

    /*
    ** Checks
    */
    if ((parse_keywords && (parse_form || parse_value || echo || init)) ||
	(parse_form && (parse_value || echo || init)) ||
	(parse_value && (echo || init)) ||
	(echo && init))
	fatal2(1, "Only one of -keywords, -form, -value, -read and -init",
	       "can be specified at a time");

    if (content_length) content_len = atoi(content_length);
    if (!request_method) request_method = "UNDEFINED";

    if (echo)
	if (!strcmp(request_method, "GET"))
	    fatal(3, "Won't read from stdin with method GET");
	else if (!strcmp(request_method, "HEAD"))
	    fatal(3, "Won't read from stdin with method HEAD");

    if (!content_length) {
	if (!query_string || echo)
	    fatal(2, "Both QUERY_STRING and CONTENT_LENGTH not defined");
    }
    else if (echo) {
	int cnt, ch;
	for (cnt=content_len;  cnt>0 && (ch=getchar())!=EOF;  cnt--)
	    putchar(ch);
	exit(0);
    }
    else if (!query_string || !*query_string) {
	int cnt, ch;
	char * cur;

	cur = query_string = (char*)malloc(content_len + 1);
	if (!query_string) outofmem(__FILE__, "query_string-malloc");

	for(cnt=content_len;  cnt>0 && (ch=getchar())!=EOF;  cnt--,cur++)
	    *cur = (char)ch;
	*cur = 0;

	if (init) {
	    printf("QUERY_STRING='%s'; export QUERY_STRING\n", query_string);
	    exit(0);
	}
    }
    else if (init) exit(0);

    if (strchr(query_string, '='))
	form_request = YES;

    if (form_request  &&  parse_keywords)
	fatal2(3, (!strcmp(request_method, "POST") ? "Body" : "QUERY_STRING"),
	       "contains fomr request -- not search keywords");

    if (!form_request  &&  !parse_keywords  &&  *query_string)
	fatal2(3, (!strcmp(request_method, "POST") ? "Body" : "QUERY_STRING"),
	       "contains search keywords -- not a form request");

    if (parse_keywords) {
	char ** keywordvec;
	int     cnt = 1;	/* Last keyword not ended with a plus */

	for (cur=query_string;  *cur;  cur++) {
	    if ('+' == *cur) cnt++;
	}
	keywordvec = (char**)calloc(cnt, sizeof(char*));
	if (!keywordvec) outofmem(__FILE__, "keywordvec-calloc");

	for(cur=query_string, i=0;  cur;  i++) {
	    char *end = strchr(cur, '+');
	    if (end) *(end++) = 0;
	    HTUnEscape(cur);
	    keywordvec[i] = cur;
	    cur = end;
	}
	if (count)
	    printf("%d\n", cnt);
	else if (nth) {
	    if (nth <= cnt)
		printf("%s\n", keywordvec[nth-1]);
	    else fatal(3, "Not so many keywords");
	}
	else for(i=0; i<cnt; i++)
	    printf("%s\n", keywordvec[i]);
	exit(0);
    }
    else if (parse_form) {
	char ** formvec;
	int     cnt = 1;	/* Last name-value pair not ended with & */

	for (cur=query_string;  *cur;  cur++) {
	    if ('&' == *cur) cnt++;
	}
	formvec = (char**)calloc(2*cnt+1, sizeof(char*));
	if (!formvec) outofmem(__FILE__, "formvec-calloc");
	formvec[0] = NULL;

	cur=query_string;
	i=0;
	cnt=0;
	while (cur) {
	    char *end1 = strchr(cur, '=');
	    char *end2 = (end1 ? strchr(end1, '&') : NULL);

	    if (end1) {
		int pos;
		*(end1++) = 0;
		if (end2) *(end2++) = 0;
		HTUnEscape(cur);
		plusses_to_spaces(end1);
		HTUnEscape(end1);
		pos = already_in_array(formvec, cur);
		if (pos > 0) {
		    char *str=(char*)malloc(strlen(formvec[pos]) +
					    strlen(end1) + 1 +
					    (separator? strlen(separator):2));
		    if (!str) outofmem(__FILE__, "multi-value-malloc");
		    sprintf(str, "%s%s%s", formvec[pos],
			    (separator ? separator : ", "), end1);
		    formvec[pos] = str;
		    /*
		    ** Previous formvec[pos] can't be free()d, because
		    ** it may be part of the QUERY_STRING env.var.
		    ** It doesn't matter anyway in this kind of small
		    ** program that exits sooner than you can blink.
		    */
		}
		else {
		    formvec[i++] = cur;
		    formvec[i++] = end1;
		    formvec[i] = NULL;
		    cnt++;
		}
	    }
	    cur = end2;
	}
	if (count)
	    printf("%d\n", cnt);
	else if (nth) {
	    if (nth <= cnt) {
		if (valid_varname(formvec[(nth-1)*2]))
		    printf("%s%s=%s",
			   (prefix ? prefix : "FORM_"),
			   formvec[(nth-1)*2], sh_escape(formvec[nth*2-1]));
		else fatal(3,"Invalid form field name form shell var.name");
	    }
	    else fatal(3, "Not so many unique form fields");
	}
	else {
	    for(i=0; formvec[i]; i+=2) {
		if (valid_varname(formvec[i]))
		    printf("%s%s%s=%s", (i ? "; " : ""),
			   (prefix ? prefix : "FORM_"),
			   formvec[i], sh_escape(formvec[i+1]));
	    }
	    printf("\n");
	}
	exit(0);
    }
    else if (parse_value) {
	int found = 0;
	for(cur=query_string;  cur;  ) {
	    char *end1 = strchr(cur, '=');
	    char *end2 = (end1 ? strchr(end1, '&') : NULL);

	    if (end1) {
		*(end1++) = 0;
		if (end2) *(end2++) = 0;
		HTUnEscape(cur);
		if (!strcmp(cur, parse_value)) {
		    found++;
		    if (!count  &&  (!nth || nth==found)) {
			plusses_to_spaces(end1);
			HTUnEscape(end1);
			if (separator) {
			    if (found > 1) printf("%s", separator);
			    printf("%s", end1);
			}
			else printf("%s\n", end1);
			if (nth) exit(0);
		    }
		}
	    }
	    cur = end2;
	}
	if (count)
	    printf("%d\n", found);
	else if (!found) fatal(3, "No matching field name found");
	else {
	    if (separator) printf("\n");
	    exit(0);
	}
    }
    else fatal(1, "Internal error -- seems like I have noting to do??");

    return 0;	/* Exit program -- never reached */
}

