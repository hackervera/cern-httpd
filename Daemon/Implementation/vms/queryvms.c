
/* PROGRAM							queryvms.c
**		PROGRAM FOR CGI/1.0 SCRIPTS TO PARSE WWW_KEY
**		ENVIRONTMENT VARIABLES PASSED FROM THE SERVER, AND
**		RETURN A REPORT OF name/value PAIRS TO THE CLIENT.
**		  Also loads a structure with name/value pairs
**		  (entries[x].name, entries[x].val) as a model
**		  for programs which USE the name value pairs,
**		  rather than simply reporting them.  TEXTAREA
**		  values are returned to the client with PRE
**		  format, and are loaded into entries[x].val
**		  as stream_LF.  They might instead be written
**		  to a temporary file, for mailing or analysis
**		  by a script or program.
**		FOR USE WITH BOTH POST AND GET FORMS ON VMS.
**
** USAGE (called from a script, e.g., query.com):
**	$ queryvms := $device:[directory]queryvms.exe
**	$ queryvms [-headers]	! -h is sufficient
**		If called with the switch, outputs Content-Type header,
**		a TITLE, and a text header.  Call without the switch to
**		insert a name/value report into the body of a script
**		output with its own TITLE and surrounding text.
**
** AUTHORS:
**	FM	Foteos Macrides		macrides@sci.wfeb.edu
**
** HISTORY:
**	 7 Apr 94  FM	Written for use with my modifications of the
**			v216-1betavms httpd.
**
** BUGS:
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <unixlib.h>

#define MAX_ENTRIES 1000	/* Maxiumun name/value pairs to handle */

typedef struct {    		/* Structure for holding name/value pairs  */
    char *name;
    char *val;
} entry;

int main(int argc, char **argv)
{
    entry entries[MAX_ENTRIES];
    register int x=0, count, key, line, linecount;
    char *cp, *METHOD, keysymbol[20];
    unsigned short HEADERS = FALSE;

    /*
    ** Check for the switch
    */
    if (argc > 1 && !strncmp(argv[1], "-headers", 2))
    	HEADERS = TRUE;

    /*
    ** Put out the MIME type header if requested
    */
    if (HEADERS)
    	printf("Content-Type: text/html\n\n");

    /*
    ** Verify that the symbols are the content from a POST or GET form
    */
    if ((METHOD=getenv("WWW_REQUEST_METHOD")) == NULL ||
        (strcmp(METHOD,"POST") && strcmp(METHOD, "GET"))) {
	if (HEADERS)
    	    printf("<HEAD><TITLE>Report of form entries</TITLE></HEAD>\n");
    	printf("<H1>Query Status</H1>");
        printf("This script should be referenced with a METHOD of POST or GET.\n");
        printf("If you don't understand this, see this ");
        printf("<A HREF=\"http://info.cern.ch/hypertext/WWW/Daemon/User/CGI/Overview.html\">CGI overview</A>.\n");
        exit(0);
    }
    if (0==strcmp(METHOD, "POST")) {
        if ((cp=getenv("WWW_CONTENT_TYPE")) == NULL ||
            strcmp(cp,"application/x-www-form-urlencoded")) {
	    if (HEADERS)
    	        printf("<HEAD><TITLE>Report of %s form entries</TITLE></HEAD>\n", METHOD);
    	    printf("<H1>Query Status</H1>");
            printf("Error -- 'application/x-www-form-urlencoded' was not specified for a form with METHOD=POST.\n");
            exit(0);
	}
    }

    /*
    ** Make sure something is there, and not too much
    */
    if ((cp=getenv("WWW_KEY_COUNT")) == NULL || *cp == '0') {
        printf("No name/value pairs were submitted.\n");
        exit(0);
    } else { 
    	count = atoi(cp);
	if (count > MAX_ENTRIES)
	    count = MAX_ENTRIES;
    }

    /*
    ** Output the leader, and headers if requested
    */
    if (HEADERS) {
        printf("<HEAD><TITLE>Report of %s form entries</TITLE><HEAD>\n", METHOD);
        printf("<H1>Query Results</H1>");
    }
    printf("You submitted the following name/value pairs:<p>\n");
    printf("<ul>\n");

    /*
    ** Load each name and value pair into entries structure,
    ** and output each pair to the client
    */
    for (key = 1; count > 0; count -= 2) {

        /*
	** Get the name
	*/
        sprintf(keysymbol, "WWW_KEY_%d", key++);

        /*
	** Load the name field into the entries structure
	*/
        entries[x].name = getenv(keysymbol);

	/*
	** Trim the terminal '=' from the name
	*/
	if (entries[x].name[strlen(entries[x].name)-1] == '=')
	    entries[x].name[strlen(entries[x].name)-1] = '\0';

        /*
	** Check whether the value field is a multi-line TEXTAREA
	*/
	sprintf(keysymbol, "WWW_KEY_%d_COUNT", key);

	if ((cp=getenv(keysymbol)) == NULL || (linecount=atoi(cp)) < 2) {
	/*
	** Value is a single string
	*/
	    /*
	    ** Load the value into the entries structure
	    */
	    if (cp == NULL)
	        sprintf(keysymbol, "WWW_KEY_%d", key++);
	    else
	        sprintf(keysymbol, "WWW_KEY_%d_%d", key++, linecount);
	    entries[x].val = getenv(keysymbol);

            /*
	    ** Output name/value pairs as "name = value"
	    */
	    printf("<li> <code>%s = %s</code>\n",
			       entries[x].name, entries[x].val);

	} else {
	/*
	** Value is a multi-line TEXTAREA
	*/
	    unsigned short FirstLine = TRUE;
	    
	    /*
	    ** Output the TEXTAREA name as a single line with a colon
	    */
	    printf("<li> <code>%s: </code></ul>\n", entries[x].name);

	    /*
	    ** Get each line and append '\n', concatenate into value of
	    ** entries structure as a stream_LF string, and output as
	    ** PRE formatted text (use the "obsolete" PLAINTEXT or LISTING
	    ** if you want *no* interpretation; browsers still respect them,
	    ** and interpret some tags in PRE)
	    */
	    FirstLine = TRUE;
	    line = 1;
	    while (line <= linecount) {
	        sprintf(keysymbol, "WWW_KEY_%d_%d", key, line++);
		cp = getenv(keysymbol);
		strcat(cp, "\n");

		/*
		** Add current line to value in the entries structure
		*/
		if (FirstLine) {
		    entries[x].val = cp;
		    FirstLine = FALSE;
		}
		else
		    strcat(entries[x].val, cp);
	    }
	    /*
	    ** Output the TEXTAREA value
	    */
	    printf("<PRE>%s</PRE><ul>\n", entries[x].val);
	    key++;
	}

	/*
	** Increment entries index
	*/
	x++;
    }
    printf("</ul>\n");

    exit(0);
    return(0);
} /* main */
