 /*
  * rfc931() speaks a common subset of the RFC 931, AUTH, TAP and IDENT
  * protocols. The code queries an RFC 931 etc. compatible daemon on a remote
  * host to look up the owner of a connection. The information should not be
  * used for authentication purposes. This routine intercepts alarm signals.
  * 
  * Diagnostics are reported through syslog(3).
  * -- NOT anymore, they go to httpd error log -- AL.
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#ifndef lint
static char sccsid[] = "@(#) rfc931.c 1.8 93/12/13 22:23:20";
#endif

#include "HTUtils.h"
#include "tcp.h"
#include "HTTCP.h"
#include "HTLog.h"

#ifndef _HPUX_SOURCE
#define _HPUX_SOURCE
#endif

/* System libraries. */
#ifndef VMS

#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>

/*
 * For some reason I get a complaint about redefinitions of
 * struct linger, sockaddr, sockproto, msghdr if I include
 * this on our DecStation.  Correct me if I'm wrong.
 *
 * BTW, how can I tell that I'm compiling on a DecStation,
 * now I just do -Ddecstation on the command line.
 *					<httpd@info.cern.ch>
 */
#if !defined(decstation) && !defined(Mips)
#include <sys/socket.h>
#endif

#if !defined(__BSD__)
#include <netinet/in.h>
#endif

#include <setjmp.h>
#include <signal.h>

extern char *inet_ntoa();

/* Local stuff. */

/* #include "log_tcp.h" */

#define RFC931_TIMEOUT	30		/* 30 secs is more than enough */
#define	RFC931_PORT	113		/* Semi-well-known port */
#define	ANY_PORT	0		/* Any old port will do */
#define FROM_UNKNOWN  "unknown"

int     rfc931_timeout = RFC931_TIMEOUT;/* Global so it can be changed */

PRIVATE jmp_buf timebuf;

/* fsocket - open stdio stream on top of socket */

PRIVATE FILE *fsocket ARGS3(int,	domain,
			    int,	type,
			    int,	protocol)
{
    int     s;
    FILE   *fp;

    if ((s = socket(domain, type, protocol)) < 0) {
	HTLog_error2("RFC931: socket() failed:",HTErrnoString());
#if 0
	syslog(LOG_ERR, "socket: %m");
#endif
	return (0);
    } else {
	if ((fp = fdopen(s, "r+")) == 0) {
	    HTLog_error2("RFC931: fdopen() failed:",HTErrnoString());
#if 0
	    syslog(LOG_ERR, "fdopen: %m");
#endif
	    close(s);
	}
	return (fp);
    }
}

/* bind_connect - bind both ends of a socket */

PRIVATE int bind_connect ARGS4(int,			s,
			       struct sockaddr *,	local,
			       struct sockaddr *,	remote,
			       int,			length)
{
    if (bind(s, local, length) < 0) {
	HTLog_error2("RFC931: bind() failed:",HTErrnoString());
#if 0
	syslog(LOG_ERR, "bind: %m");
#endif
	return (-1);
    } else {
	return (connect(s, remote, length));
    }
}

/* timeout - handle timeouts */

PRIVATE void timeout(sig)
int     sig;
{
    longjmp(timebuf, sig);
}

/* rfc931 - return remote user name, given socket structures */

PUBLIC char * rfc931 ARGS2(struct sockaddr_in *,	rmt_sin,
			   struct sockaddr_in *,	our_sin)
{
    unsigned rmt_port;
    unsigned our_port;
    struct sockaddr_in rmt_query_sin;
    struct sockaddr_in our_query_sin;
    static char user[256];		/* XXX */
    char    buffer[512];		/* XXX */
    char   *cp;
    char   *result = FROM_UNKNOWN;	/* XXX */
    FILE   *fp;

    CTRACE(stderr, "RFC931...... Getting remote ident\n");

    /*
     * Use one unbuffered stdio stream for writing to and for reading from
     * the RFC931 etc. server. This is done because of a bug in the SunOS
     * 4.1.x stdio library. The bug may live in other stdio implementations,
     * too. When we use a single, buffered, bidirectional stdio stream ("r+"
     * or "w+" mode) we read our own output. Such behaviour would make sense
     * with resources that support random-access operations, but not with
     * sockets.
     */

    if ((fp = fsocket(AF_INET, SOCK_STREAM, 0)) != 0) {
	setbuf(fp, (char *) 0);

	/*
	 * Set up a timer so we won't get stuck while waiting for the server.
	 */

	if (setjmp(timebuf) == 0) {
	    signal(SIGALRM, timeout);
	    alarm(rfc931_timeout);

	    /*
	     * Bind the local and remote ends of the query socket to the same
	     * IP addresses as the connection under investigation. We go
	     * through all this trouble because the local or remote system
	     * might have more than one network address. The RFC931 etc.
	     * client sends only port numbers; the server takes the IP
	     * addresses from the query socket.
	     */

	    our_query_sin = *our_sin;
	    our_query_sin.sin_port = htons(ANY_PORT);
	    rmt_query_sin = *rmt_sin;
	    rmt_query_sin.sin_port = htons(RFC931_PORT);

	    if (bind_connect(fileno(fp),
			     (struct sockaddr *) & our_query_sin,
			     (struct sockaddr *) & rmt_query_sin,
			     sizeof(our_query_sin)) >= 0) {

		/*
		 * Send query to server. Neglect the risk that a 13-byte
		 * write would have to be fragmented by the local system and
		 * cause trouble with buggy System V stdio libraries.
		 */
		CTRACE(stderr,"RFC931...... connected, sending ident request\n");
		fprintf(fp, "%u,%u\r\n",
			ntohs(rmt_sin->sin_port),
			ntohs(our_sin->sin_port));
		fflush(fp);

		/*
		 * Read response from server. Use fgets()/sscanf() so we can
		 * work around System V stdio libraries that incorrectly
		 * assume EOF when a read from a socket returns less than
		 * requested.
		 */
		CTRACE(stderr,"RFC931...... reading response from identd\n");
		if (fgets(buffer, sizeof(buffer), fp) != 0
		    && ferror(fp) == 0 && feof(fp) == 0
		    && sscanf(buffer, "%u , %u : USERID :%*[^:]:%255s",
			      &rmt_port, &our_port, user) == 3
		    && ntohs(rmt_sin->sin_port) == rmt_port
		    && ntohs(our_sin->sin_port) == our_port) {

		    /*
		     * Strip trailing carriage return. It is part of the
		     * protocol, not part of the data.
		     */

		    if ((cp = strchr(user, '\r')))
			*cp = 0;
		    result = user;
		}

	    }
	    else {
		CTRACE(stderr, "RFC931...... connect() failed: %s\n",
		       HTErrnoString());
	    }
	    alarm(0);
	}
	fclose(fp);
    }

    CTRACE(stderr,"RFC931...... Remote ident: %s\n", result ?result :"-none-");
    return (result);
}
#else

PUBLIC char * rfc931 ARGS2(struct sockaddr_in *,	rmt_sin,
			   struct sockaddr_in *,	our_sin)
{
   return("Unidentified");
}
#endif /* VMS */
