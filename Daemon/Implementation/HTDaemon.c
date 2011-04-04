/*              TCP/IP based server for HyperText               HTDaemon.c
**              ---------------------------------
**
**
** Compilation options:
**      DIR_OPTIONS     If defined, -d options control directory access
**      POSIXWAIT       If defined, uses waitpid instead of wait3
**
**  Authors:
**      TBL     Tim Berners-Lee, CERN
**      JFG     Jean-Francois Groff, CERN
**      JS      Jonathan Streets, FNAL
**      AL      Ari Luotonen, CERN
**      MD      Mark Donszelmann, CERN
**      FM      Foteos Macrides, WFEB
**
**  History:
**         Sep 91  TBL  Made from earlier daemon files. (TBL)
**      26 Feb 92  JFG  Bug fixes for Multinet.
**       8 Jun 92  TBL  Bug fix: Perform multiple reads in case we don't get
**                      the whole command first read.
**      25 Jun 92  JFG  Added DECNET option through TCP socket emulation.
**       6 Jan 93  TBL  Plusses turned to spaces between keywords
**       7 Jan 93  JS   Bug fix: addrlen had not been set for accept() call
**                      Logging in GMT to file-YYMM in name
**         Sep 93  AL   Added Access Authorization, and
**                      some minor fixes to make server more tolerant of
**                      missing stuff (NULLs here and there). It can be
**                      expected that new browsers might screw up with
**                      Access Authorization at first.
**      20 Oct 93  AL   Now makes sure that it is really an HTTP1
**                      request and not just an HTTP0 request with
**                      spaces in the URL.
**       6 Nov 93  MD   Bug fix: gmtime replace by localtime on VMS.
**                      Bug fix: changed vms into VMS (ifdef is case sensitive)
**                      calls to strcasecmp were changed into strcasecomp 
**                      (same for strncasecmp).
**                      Added file sharing of the logfile (VMS).
**                      Added switching off of SYSPRV after bind (VMS).
**                      disabled Forking (VMS).
**      12 Nov 93  AL   Understands wildcards in Content-Type.
**                      Handles SIGPIPE.
**      23 Nov 93  MD   Bug fix: Corrected timestamp if running in stand-alone
**                      mode. Used to give same time after start.
**                      Flushing of buffers now also flushes RMS buffer.
**       2 Dec 93  MD   Fixed reuse of ports with open connections (VMS)
**      11 Dec 93  AL   Added HTRequest, HTHandle() heavily rewritten.
**                      Added also a new module, HTRequest.c, which
**                      now needs to be linked together with daemon.
**      14 Dec 93  AL   Forking moved to the beginning of HTHandle().
**      22 Feb 94  MD   Changed for VMS.
**                      Added user init routines
**      25 Mar 94  MD   Took out HTCacheHTTP
**      27 Mar 94  AL   Added SOCKS stuff by Ian Dunkin <imd1707@ggr.co.uk>
**      31 Mar 94  FM   Added Inetd support for VMS.
**                      Added HTVMS_disableAllPrv for turning off all
**                      unnecessary privileges.
**      04 Apr 94  FM   Fixed to set HTServerPort when running under
**                      Inetd on VMS.
**      05 Jul 94  FM   Replaced HTLoadRules() with HTLoadConfig() for
**                      RunFromInetd on VMS.
**	07 Jul 94  FM	Cast Content-type to text/html in HTErrorMsg.
**			Added code for setting server's default directory
**			to the default data tree root on VMS, if one was
**			defined via "ServerRoot" in the configuration file.
**			Commented out dead extern declarations.
**	 8 Jul 94  FM	Insulate free() from _free structure element.
*/

/* (c) CERN WorldWideWeb project 1990-1992. See Copyright.html for details */


/*      Module parameters:
**      -----------------
**
**  These may be undefined and redefined by syspec.h
*/

#include <time.h>

#define FORKING

#ifdef __svr4__
#define LISTEN_BACKLOG 32	/* Number of pending connect requests (TCP) */
#else
#define LISTEN_BACKLOG 5	/* Number of pending connect requests (TCP) */
#endif /* __svr4__ */

#define MAX_CHANNELS 20         /* Number of channels we will keep open */
#define WILDCARD '*'            /* Wildcard used in addressing */
#define FIRST_TCP_PORT  5000    /* When using dynamic allocation */
#define LAST_TCP_PORT 5999

#define MAX_LINE 512            /* HTTP request field line */

#ifndef RULE_FILE
#ifdef VMS
#define RULE_FILE               "httpd_dir:httpd.conf"
#else /* not VMS */
#define RULE_FILE               "/etc/httpd.conf"
#endif /* not VMS */
#endif

#ifndef DEFAULT_EXPORT
#define DEFAULT_EXPORT		"/Public"
#endif

#ifdef ISC3		/* Lauren */
#define SIGSTP    
#define POSIXWAIT 
#define _POSIX_SOURCE   
#include <sys/types.h>  
#include <sys/wait.h>  
#include <net/errno.h> 
#endif

#ifdef __osf__
#define _OSF_SOURCE
#define _BSD
#endif

#include "HTUtils.h"
#include "tcp.h"                /* The whole mess of include files */
#include "HTTCP.h"              /* Some utilities for TCP */
#include "HTFormat.h"
#include "HTInit.h"
#include "HTSUtils.h"

#ifdef VMS
#include "HTVMSUtils.h"
#include <descrip.h>
#include <ssdef.h>
#include <stdlib.h>
#include <unixlib.h>
#undef FORKING
#endif /* VMS */

#include "HTRules.h"

#include "HTWriter.h"
#include "HTMulti.h"
#include "HTAAServ.h"

#include "HTFile.h"
#include "HTParse.h"
#include "HTRequest.h"
#include "HTAuth.h"             /* HTUser */
#include "HTConfig.h"
#include "HTDaemon.h"
#include "HTLog.h"
#include "HTCache.h"
#include "HTUserInit.h"
#include "HTFWriter.h"
#include "HTims.h"
#include "HTError.h"


/* Forking */
#ifdef FORKING
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/param.h>
#include <errno.h>

#ifndef SIGCLD
#ifdef SIGCHLD
#define SIGCLD SIGCHLD
#endif
#endif

#ifdef SIGTSTP	/* BSD */
#ifndef ISC3	/* Lauren */
#include <sys/file.h>
#endif
#include <sys/ioctl.h>
#endif /* BSD */

#endif /* FORKING */


#ifndef VMS
#ifdef SUN4
#include <sys/timeb.h>
#endif

#include <pwd.h>        /* Unix password file routine: getpwnam() */
#include <grp.h>        /* Unix group file routine: getgrnam()    */
#endif /* not VMS */

#ifdef __osf__
#include <sys/wait.h>
#include <sys/resource.h>
#endif



#ifdef VMS
#define gc()
/* dummy declaration to make sure that HTML.OBJ is not included
   and other references (used in clients only) are resolved */
#ifdef __DECC
#include "HText.h"
PUBLIC CONST HTStructuredClass HTMLPresentation;
PUBLIC int HTDiag;
PUBLIC HText * HTMainText;
PUBLIC BOOL interactive;
#endif /* DECC */
#endif /* VMS */

/*      Module-Global Variables
**      -----------------------
*/
PRIVATE enum role_enum {master, slave, transient, passive} role;
PRIVATE SockA   server_soc_addr;
PRIVATE SockA   client_soc_addr;
PRIVATE int     master_soc;     /* inet socket number to listen on */
PRIVATE int     com_soc;        /* inet socket number to read on */
#ifdef SELECT
PRIVATE fd_set  open_sockets;   /* Mask of channels which are active */
PRIVATE int     num_sockets;    /* Number of sockets to scan */
#endif
PRIVATE BOOLEAN dynamic_allocation;     /* Search for port? */

#ifdef FORKING
PRIVATE BOOL    HTForkEnabled = NO; /* Do we fork() when serving.       */
                                    /* This is enabled when server is   */
                                    /* standalone (-a or -p option).    */
#endif /* FORKING */
PRIVATE int script_pid = 0;

/*      Program-Global Variables
**      ------------------------
*/
PUBLIC char * HTAppName = "CERN-HTTPD"; /* Application name             */
PUBLIC char * HTAppVersion = VD;        /* Application version          */

PUBLIC char * HTClientProtocol = 0;     /* Protocol and version number  */
PUBLIC char * HTServerProtocol = "HTTP/1.0";
extern char * HTClientHost;             /* Clients IP address           */
PUBLIC char * HTClientHostName = NULL;  /* Clients host name            */
PUBLIC int HTServerPort = 80;           /* The portnumber of this server*/
PUBLIC int child_serial = 0;            /* Serial number of child       */

PUBLIC BOOL ignore_sigpipes = NO;       /* Should we ignore SIGPIPE     */
PUBLIC BOOL sigpipe_caught = NO;        /* So have we caught a SIGPIPE  */

char * rfc931 PARAMS((struct sockaddr_in *      rmt_sin,
                      struct sockaddr_in *      our_sin));


/*      Server environment when handling requests
**      -----------------------------------------
*/
PUBLIC char *           HTReasonLine            = NULL;
PUBLIC int              HTSoc                   = 0;
PUBLIC char *           HTReqLine               = NULL;
PUBLIC char *           HTReqArg                = NULL;
PUBLIC char *           HTReqArgPath            = NULL;
PUBLIC char *           HTReqArgKeywords        = NULL;
PUBLIC char *           HTReqTranslated         = NULL;
PUBLIC char *           HTReqScript             = NULL;
PUBLIC char *           HTScriptPathInfo        = NULL;
PUBLIC char *           HTScriptPathTrans       = NULL;
PUBLIC char *           HTLocation              = NULL;
PUBLIC char *           HTLastModified          = NULL;
PUBLIC char *           HTExpires               = NULL;
PUBLIC char *           HTMetaFile              = NULL;
PUBLIC char *           HTAuthString            = NULL;
PUBLIC char *           HTWWWAuthenticate       = NULL;
PUBLIC HTAAFailReason   HTReason                = HTAA_OK;
PUBLIC char *           HTUserAgent             = NULL;
PUBLIC char *           HTReferer               = NULL;
PUBLIC time_t           HTIfModifiedSince       = 0;

PUBLIC char *           remote_ident            = NULL;

PUBLIC long             HTCacheUsage            = 0L;
PUBLIC int              HTChildExitStatus       = 0;
PUBLIC long             HTCachedBytes           = 0;

PUBLIC char *           gc_info_file            = NULL;
PUBLIC int              gc_pid                  = 0;
PUBLIC time_t           gc_last_time            = 0;

PUBLIC BOOL             cache_hit               = NO;
PUBLIC BOOL             proxy_access            = NO;

PUBLIC time_t           cur_time                = 0;

PUBLIC BOOL             trace_cache             = NO;
PUBLIC BOOL             trace_all               = NO;

PUBLIC HTInStruct in;
PUBLIC HTOutStruct out;

PUBLIC void reset_server_env NOARGS
{
    FREE(HTReqLine);
    FREE(HTReqArg);
    FREE(HTReqArgPath);
    FREE(HTReqTranslated);
    FREE(HTReqScript);
    FREE(HTScriptPathTrans);
    FREE(HTLocation);
    FREE(HTLastModified);
    FREE(HTExpires);
    FREE(HTMetaFile);
    FREE(HTAuthString);
    FREE(HTWWWAuthenticate);
    FREE(HTProxyHeaders);
    FREE(HTUserAgent);
    FREE(HTReferer);
    HTIfModifiedSince = 0;
    HTReasonLine = NULL;
    HTSoc = 0;
    HTReqArgKeywords = NULL;
    HTScriptPathInfo = NULL;
    HTReason = HTAA_OK;
    HTProxyHeaders = NULL;

    HTUser = NULL;
    HTProt = NULL;
    HTDefProt = NULL;

    HTChildExitStatus = 0;
    HTCachedBytes = 0;
    cache_hit = 0;

    memset((char*)&in, 0, sizeof(in));
    memset((char*)&out, 0, sizeof(out));
}


PUBLIC void compute_server_env NOARGS
{
    char *keywords;

    FREE(HTReqArgPath);
    FREE(HTReqArgKeywords);

    FREE(HTReqTranslated);
    FREE(HTReqScript);
    FREE(HTScriptPathInfo);
    FREE(HTScriptPathTrans);
    FREE(HTLocation);

    if (HTReqArg) {
        int len;

        if ((keywords = strchr(HTReqArg, '?'))) {
            *keywords = 0;              /* Temporarily truncate */
            StrAllocCopy(HTReqArgPath, HTReqArg);
            StrAllocCopy(HTReqArgKeywords, keywords+1);
            *keywords = '?';            /* Reconstruct */
        }
        else StrAllocCopy(HTReqArgPath, HTReqArg);

        HTUnEscape(HTReqArgPath);
        HTReqArgPath = HTSimplify(HTReqArgPath);	 /* Remove ".." etc. */

        /*
        ** HTSimplify will leave in a "/../" at the top, which can
        ** be a security hole.
        */
        len = strlen(HTReqArgPath);
        if (strstr(HTReqArgPath, "/../") ||
            !strncmp(HTReqArgPath,"/..",3) ||
            !strncmp(HTReqArgPath,"..",2) ||
            (len >= 3  &&  !strcmp(&HTReqArgPath[len-3], "/.."))) {
            CTRACE(stderr, "Illegal..... attempt to use /../ (\"%s\")\n",
                   HTReqArg);
            HTReason = HTAA_DOTDOT;
        }
    }
}





#if defined(Mips)
PRIVATE int     pgrp;
#else
#if defined(__hpux) || defined(POSIXWAIT)
PRIVATE pid_t   pgrp;
#else
PRIVATE int     pgrp;
#endif
#endif


extern HTAAProt * HTProt;
extern HTAAProt * HTDefProt;
#ifdef NOT_USED
extern HTList * HTNoLog;        /* Machines for which log entry not made */
extern BOOL     HTDisabled[(int)MAX_METHODS + 1];       /* Disabled methods */
#endif /* NOT_USED */
extern BOOL     HTImProxy;


PRIVATE void read_gc_results NOARGS
{
    FILE * fp;
    char buf[MAX_LINE];

    if (!cc.cache_root) {
        HTLog_error("BUG: gc returned yet we're not caching -- weird");
        return;
    }
    if (!gc_info_file) {
        gc_info_file = (char*)malloc(strlen(cc.cache_root) + 20);
        sprintf(gc_info_file, "%s/.gc_info", cc.cache_root);
        CTRACE(stderr, "BugFix...... gc info file wasn't set");
    }

    fp = fopen(gc_info_file, "r");
    if (!fp) {
        if (child_serial)
            HTLog_error2("Can't open gc report file for reading:",
                         gc_info_file);
        return;
    }
    if (fgets(buf, MAX_LINE, fp)) {
        int pid;
        long bytes_used = 0;
        long bytes_freed = 0;
        int files_used = 0;
        int files_freed = 0;
        long corrected = 0;

        sscanf(buf, "%d %ld %ld %d %d",
               &pid, &bytes_used, &bytes_freed, &files_used, &files_freed);

        if (gc_pid  &&  pid != gc_pid) {
            HTLog_error(
        "WARNING: gc report has wrong process id - real gc may have crashed");
        }

        corrected = HTCacheUsage - bytes_freed/1024;
        if (TRACE) {
            fprintf(stderr, "GC report... from pid %d\n", pid);
            fprintf(stderr, "............ %ldK used (%d files)\n",
                    bytes_used/1024, files_used);
            fprintf(stderr, "............ %ldK collected (%d files)\n",
                    bytes_freed/1024, files_freed);
            fprintf(stderr, "Parent fix.. %ldK used => %ldK used\n",
                    HTCacheUsage, corrected);
        }
        if (corrected < 0 ||
            corrected < bytes_used/1140 ||
            corrected > bytes_used/930) {
            CTRACE(stderr, "Parent...... Correcting %ldK => %ldK used\n",
                   corrected, bytes_used/1024);
            corrected = bytes_used/1024;
            if (corrected<0) corrected = 0;
        }
        HTCacheUsage = corrected;
        CTRACE(stderr,"Parent...... %ldK of cache in use\n",HTCacheUsage);
    }
    else HTLog_error2("gc report file is empty:", gc_info_file);
    fclose(fp);
}


#ifdef FORKING
/* PRIVATE                                                      sig_child()
**
** This function is taken from:
**      W.Richard Stevens: UNIX Network Programming,
**      Prentice Hall 1990, page 82.
**
** This is a 4.3BSD SIGCLD signal handler that can be used by a
** server that's not interested in its child's exit status, but needs
** to wait for them , to avoid clogging up the system with zombies.
**
** Beware that the calling process may get an interrupted system
** call when we return, so they had better handle that.
*/
PRIVATE int sig_child NOARGS
{
#if defined(SIGTSTP) || defined(SIGCLD)    /* Signals which use this handler */

#if defined(POSIXWAIT) || defined(__hpux)
#if !defined(Mips)
#define USE_WAITPID
#endif /* Mips */
#endif

#if defined(NeXT) || defined(_AIX)
    union wait status;
#else
    int status;
#endif /* NeXT */

#ifdef USE_WAITPID
    pid_t pid;
    while ((pid = waitpid((pid_t)-1, &status, WNOHANG)) > 0)
#else
    int         pid;
    /* union wait  status; union wait is obsolete */
    while ((pid = wait3(&status, WNOHANG, (struct rusage*)NULL)) > 0)
#endif /* USE_WAITPID */
      {
        CTRACE(stderr,"Parent...... child pid %d has finished\n",(int)pid);
        if (pid == gc_pid) {
            CTRACE(stderr, "Parent...... gc has completed\n");
            read_gc_results();
            gc_pid = 0;
        }
        if (cc.cache_root  &&  WIFEXITED(status)) {
            int child_usage = HTExitStatusToKilos(WEXITSTATUS(status));
            HTCacheUsage += child_usage;
            CTRACE(stderr,
                   "Parent...... %dK taken up by child, total %ldK used\n",
                   child_usage, HTCacheUsage);
        }
      }
#endif  /* SIGTSTP or SIGCLD (POSIX) */

#ifdef UTS2
    /* Restart the signal handler so we don't leave zombies everywhere */
    set_signals();
#endif /* UTS2 */

    return 0;   /* Just to avoid a compiler warning */
}
#endif /* FORKING */


#ifndef VMS
PRIVATE void sig_term NOARGS
{
    if (sc.standalone) {
#if defined(__svr4__) || defined(_POSIX_SOURCE) || defined(__hpux)
        kill(-pgrp, SIGKILL);
#else
        killpg(pgrp, SIGKILL);
#endif
        shutdown(master_soc, 2);
        close(master_soc);
    }
}


PRIVATE void dump_core NOARGS
{
    if (HTReqLine)
        HTLog_error2("Crashing request was:", HTReqLine);
    if (sc.server_root && !sc.no_bg)
        chdir(sc.server_root);
    abort();
}

PRIVATE void sig_segv NOARGS
{
    timeout_off();      /* So timeouts don't go to parent */
    HTLog_error("CRASHED (segmentation fault, core dumped)");
    dump_core();
}

PRIVATE void sig_bus NOARGS
{
    timeout_off();      /* So timeouts don't go to parent */
    HTLog_error("CRASHED (bus error, core dumped)");
    dump_core();
}


PRIVATE void set_signals NOPARAMS;

PRIVATE void sig_hup NOARGS
{
    HTLog_error("SIGHUP caught - reloading configuration files");
    if (HTServerInit())
        HTLog_error("Restart successful");
    else
        HTLog_error("Restart failed");
    set_signals();
}

PRIVATE void sig_pipe NOARGS
{
    if (!sigpipe_caught) {
        if (HTReqLine)
            HTLog_error2("Connection interrupted [SIGPIPE], req:",HTReqLine);
        else
            HTLog_error("Connection interrupted [SIGPIPE]");
    }

    if (ignore_sigpipes) {
        CTRACE(stderr, "SIGPIPE..... ignored (writing to cache)\n");
        sigpipe_caught = YES;
        set_signals();
    }
    else {
        timeout_off();  /* So timeouts don't go to parent */
        CTRACE(stderr, "SIGPIPE..... caught, exiting...\n");
        if (com_soc > 0) {
            shutdown(com_soc, 2);
            NETCLOSE(com_soc);
        }
        exit(0);
    }
}

#endif /* not VMS */


PRIVATE void set_signals NOARGS
{
#ifndef VMS
    signal(SIGTERM, (void (*)())sig_term);
    signal(SIGHUP, (void (*)())sig_hup);
    signal(SIGSEGV, (void (*)())sig_segv);
    signal(SIGBUS, (void (*)())sig_bus);

    /*
    ** Ignore the terminal stop signals (BSD)
    */
#ifdef SIGTTOU
    signal(SIGTTOU, SIG_IGN);
#endif
#ifdef SIGTTIN
    signal(SIGTTIN, SIG_IGN);
#endif
#ifdef SIGTSTP
    signal(SIGTSTP, SIG_IGN);
#endif

    /*
    ** Parent process is not interested in child exit statuses.
    ** We have to prevent the children from becoming zombies.
    ** In System V we just ignore SIGCLD signal.
    ** In BSD we should handle SIGCLD signal and call wait3().
    ** However, because we don't want to handle interrupted
    ** system calls on BSD, we just call sig_child() after every
    ** fork() to check if any previous childs have finished.
    */
#if 0
#ifndef SIGTSTP /* SysV */
    signal(SIGCLD, SIG_IGN);
#endif
#endif

    signal(SIGCLD, (void (*)())sig_child);

    /*
    ** Ignore SIGPIPE signals so we don't die if client
    ** suddenly closes connection and we keep on writing
    ** to the socket.
    */
#if 0
    signal(SIGPIPE, SIG_IGN);
#else
    signal(SIGPIPE, (void (*)())sig_pipe);
#endif

    /*
     * Make sure we don't die if a child sets a timer and then
     * crashes itself and we get the SIGALRM signal instead.
     */
    signal(SIGALRM, SIG_IGN);

#endif /* not VMS */
}


/*
 *      Return time in 100ths of seconds to the next daily gc.
 */
PRIVATE int get_next_timeout NOARGS
{
    struct tm * t;
    int secs;
    int timeout;

    time(&cur_time);
    t = localtime(&cur_time);
    secs = t->tm_hour*3600 + t->tm_min*60 + t->tm_sec;

    if (!cc.gc_disabled) {
        if (!cc.cache_root) {
            CTRACE(stderr,"Disabling... gc [caching not enabled either] #1\n");
            cc.gc_disabled = YES;
        }
        else if (cc.cache_no_connect) {
            CTRACE(stderr,"Disabling... gc [standalone cache mode] #1\n");
            cc.gc_disabled = YES;
        }
    }

    if (cc.gc_disabled) {
        CTRACE(stderr, "Gc.......... disabled altogether\n");
        return -1;
    }
    if (!cc.gc_daily_gc) {
        CTRACE(stderr, "Daily gc.... not turned on\n");
        return -1;
    }
    if (gc_pid) {
        CTRACE(stderr, "Daily gc.... Another gc has not yet finished\n");
        return -1;
    }

    timeout = cc.gc_daily_gc - secs;
    if (timeout < 0) {
        if (cur_time - gc_last_time < 3600) {
            CTRACE(stderr,
                   "Daily gc.... Another gc done less than an hour ago\n");
            return 100 * (timeout + 86400);
        }
        else if (timeout < 120)
            return 100 * (timeout + 86400);
        else return 0;
    }
    else return 100 * timeout;
}



/*
 *      Find out if it's time to do garbage collection.
 */
PRIVATE BOOL time_to_do_gc NOARGS
{
    BOOL flag = NO;

    if (!cc.gc_disabled) {
        if (!cc.cache_root) {
            CTRACE(stderr,"Disabling... gc [caching not enabled either] #2\n");
            cc.gc_disabled = YES;
        }
        else if (cc.cache_no_connect) {
            CTRACE(stderr,"Disabling... gc [standalone cache mode] #2\n");
            cc.gc_disabled = YES;
        }
    }

    if (cc.gc_disabled) {
        CTRACE(stderr, "Cache....... I never do gc\n");
        return NO;
    }

    if (cc.cache_max_k > 0  &&  HTCacheUsage >= cc.cache_max_k) {
        CTRACE(stderr, "Cache....... limit reached: %ldK used (max %ldK)\n",
               HTCacheUsage, cc.cache_max_k);
        flag = YES;
    }

    if (cur_time - gc_last_time >= 600 &&
        get_next_timeout() >= 0 && get_next_timeout() < 120) {
        CTRACE(stderr, "Cache....... Time for daily gc\n");
        flag = YES;
    }

    if (flag)
        gc_last_time = cur_time;

    return flag;
}




/*
 *      HEAD STREAM
 *
 *      This stream makes sure that only the header section is sent
 *      to the client.
 */
struct _HTStream {
    HTStreamClass *     isa;    /* HEAD & BODY streams */
    HTStream *          sink;   /* HEAD & BODY streams */
    int                 state;  /* HEAD & BODY streams */
    int                 length; /* Only BODY stream */
};

PRIVATE void HEAD_put_char ARGS2(HTStream *, me, char, c)
{
    if (me->state >= 2)
        return;
    if (c==LF)
        me->state++;
    else if (c!=CR)
        me->state = 0;
    (*me->sink->isa->put_character)(me->sink,c);
}

PRIVATE void HEAD_put_string ARGS2(HTStream *, me, CONST char*, s)
{
    if (!s) return;
    while (*s && me->state < 2) {
        HEAD_put_char(me,*s);
        s++;
    }
}

PRIVATE void HEAD_put_block ARGS3(HTStream *, me, CONST char *, b, int, l)
{
    if (!b) return;
    while (l-- > 0  &&  me->state < 2) {
        HEAD_put_char(me,*b);
        b++;
    }
}

PRIVATE void HEAD_free ARGS1(HTStream *, me)
{
    out.content_length = 0;
    (*me->sink->isa->_free)(me->sink);
    free(me);
}

PRIVATE void HEAD_abort ARGS2(HTStream *, me, HTError, e)
{
    out.content_length = 0;
    (*me->sink->isa->abort)(me->sink,e);
    free(me);
}


/*
 *      HEAD stream class
 */
PRIVATE HTStreamClass HTHeadStreamClass =
{
    "HeadStream",
    HEAD_free,
    HEAD_abort,
    HEAD_put_char,
    HEAD_put_string,
    HEAD_put_block
};

PUBLIC HTStream * HTHeadStream ARGS1(HTStream *, sink)
{
    HTStream * me = (HTStream*)calloc(1,sizeof(HTStream));
    if (!me) outofmem(__FILE__, "HTHeadStream");

    me->isa = &HTHeadStreamClass;
    me->sink = sink;
    return me;
}


/*
 *      BODY STREAM
 *
 *      This stream calculates the length of the body when we are a 
 *      proxy without caching (when we *are* caching it will be
 *      calculated as a side-product and this is not necessary).
 */

PRIVATE void BODY_put_char ARGS2(HTStream *, me, char, c)
{
    if (me->state >= 2)
        me->length++;
    else if (c==LF)
        me->state++;
    else if (c!=CR)
        me->state = 0;
    (*me->sink->isa->put_character)(me->sink,c);
}

PRIVATE void BODY_put_block ARGS3(HTStream *, me, CONST char *, b, int, l)
{
    if (!b) return;
    while (me->state < 2 && l-- > 0) {
        BODY_put_char(me,*b);
        b++;
    }
    if (l > 0) {
        me->length += l;
        (*me->sink->isa->put_block)(me->sink,b,l);
    }
}

PRIVATE void BODY_put_string ARGS2(HTStream *, me, CONST char*, s)
{
    if (!s) return;
    BODY_put_block(me,s,strlen(s));
}

PRIVATE void BODY_free ARGS1(HTStream *, me)
{
    out.content_length = me->length;
    (*me->sink->isa->_free)(me->sink);
    free(me);
}

PRIVATE void BODY_abort ARGS2(HTStream *, me, HTError, e)
{
    out.content_length = me->length;
    (*me->sink->isa->abort)(me->sink,e);
    free(me);
}


/*
 *      BODY stream class
 */
PRIVATE HTStreamClass HTBodyStreamClass =
{
    "BodyStream",
    BODY_free,
    BODY_abort,
    BODY_put_char,
    BODY_put_string,
    BODY_put_block
};

PUBLIC HTStream * HTBodyStream ARGS1(HTStream *, sink)
{
    HTStream * me = (HTStream*)calloc(1,sizeof(HTStream));
    if (!me) outofmem(__FILE__, "HTBodyStream");

    me->isa = &HTBodyStreamClass;
    me->sink = sink;
    return me;
}




/*                      Catch error messages
**                      --------------------
**
**      This shouldn't be necessary most of the time as HTLoadError
**      will be called.
**
**      These entry points suppress the loading of HTAlert from the WWW library.
**      These could be cleaned up and made very useful, esp
**      remote progress reporting...
*/

PUBLIC void HTAlert ARGS1(CONST char *, Msg)
{
    CTRACE(stderr, "Alert....... %s\n", Msg ? Msg : "-null-");
}


PUBLIC void HTProgress ARGS1(CONST char *, Msg)
{
    CTRACE(stderr, "Progress.... %s\n", Msg ? Msg : "-null-");
}


PUBLIC BOOL HTConfirm ARGS1(CONST char *, Msg)
{
    CTRACE(stderr, "Confirm..... %s [Autoreply: NO]\n", Msg ? Msg : "-null-");
    return(NO);
}

PUBLIC char * HTPrompt ARGS2(CONST char *, Msg, CONST char *, deflt)
{
    char * rep = 0;
    StrAllocCopy(rep, deflt);
    CTRACE(stderr, "Prompt...... %s [Replied default: %s]\n",
           Msg ? Msg : "-null-", rep ? rep : "-null");
    return rep;
}

PUBLIC char * HTPromptPassword ARGS1(CONST char *, Msg)
{
    char * rep = NULL;
    StrAllocCopy(rep, "");
    CTRACE(stderr, "PromptPassword %s [Replied empty string]\n",
           Msg ? Msg : "-null-");
    return rep;
}

PUBLIC void HTPromptUsernameAndPassword ARGS3(CONST char *,     Msg,
                                              char **,          username,
                                              char **,          password)
{
    *username = HTPrompt("Username: ", *username);
    *password = HTPromptPassword("Password: ");
}


/*      Write Error Message
**      -------------------
**
**
*/
PUBLIC int HTLoadError ARGS3(HTRequest *,       req,
                             int,               num,
                             CONST char *,      msg)
{
    CTRACE(stderr, "Returning... ERROR %d:\n ** %s\n", num,
           msg ? msg : "-no-message-");

    if (num >= 500 || num==400 || num <= 0) {  /* To make error logs smaller */
        char * buf = (char*)malloc((HTReqLine ? strlen(HTReqLine) : 0) + 20);
        if (!buf) outofmem(__FILE__, "HTLoadError");

        if (HTReqLine)
            sprintf(buf, "(%d) \"%s\"", num, HTReqLine);
        else
            sprintf(buf, "(%d)", num);
        HTLog_error2(msg, buf);
        free(buf);
    }
    else HTLog_error2(HTReqArg,NULL); /* Other info goes there automatically */

    if (req && req->output_stream) {
	char * errorURL = HTError_findUrl(num);
	if (errorURL) {
	    int status = 0;

	    /*
	    **  If HEAD add a stream that makes sure we only send the head
	    **/
	    if (req->method == METHOD_HEAD)
		req->output_stream = HTHeadStream(req->output_stream);

	    /* Add any authentication headers from the original URL */
	    if (HTClientProtocol) {
		if (num == 401)
		    HTWWWAuthenticate = HTAA_composeAuthHeaders(req);
	    }

	    /* Load the error file */

	    {
		char * absError = NULL;
		char * physical = NULL;
		struct stat stat_info;
		StrAllocCopy(absError, sc.server_root);
		if (*(absError+strlen(absError)-1) != '/')
		    StrAllocCat(absError, "/");
		StrAllocCat(absError, errorURL);
		physical = HTMulti(req, absError, &stat_info);
		HTImServer = HTEscape(physical ? physical : absError,URL_PATH);
		if (TRACE)
		    fprintf(stderr, "Load Error.. loading `%s\'\n", absError);
		if ((status = HTLoadAbsolute(absError, req)) != YES) {
		    if (TRACE)
			fprintf(stderr, "Load Error.. FAILED LOADING ERROR URL\n");
		}
		FREE(HTImServer);
		FREE(absError);
		FREE(physical);
	    }
	} else {
	    char * buf;
	    int len = 500;
	    if (msg) len += strlen(msg);

	    buf = (char*)malloc(len);
	    if (!buf) outofmem(__FILE__, "HTLoadError");
	    sprintf(buf, "%s<H1>Error %d</H1>\n\n%s\n",
		    "<HTML>\n<HEAD>\n<TITLE>Error</TITLE>\n</HEAD>\n<BODY>\n",
		    num, (msg ? msg : ""));

	    sprintf(buf + strlen(buf),"\n<P><HR>%s%s %s%s\n</BODY>\n</HTML>\n",
		    "<ADDRESS><A HREF=\"http://www.w3.org\">",
		    HTAppName,
		    HTAppVersion,
		    "</A></ADDRESS>");

	    req->content_type = HTAtom_for("text/html");
	    out.content_length = strlen(buf);
	    out.status_code = num;
	    HTReasonLine = (char*)msg;
	    req->content_encoding = NULL;
	    req->content_language = NULL;

	    /*
	    **  If HEAD add a stream that makes sure we only send the head
	    **/
	    if (req->method == METHOD_HEAD)
		req->output_stream = HTHeadStream(req->output_stream);

	    if (HTClientProtocol) {
		char * headers;

		if (num == 401)
		    HTWWWAuthenticate = HTAA_composeAuthHeaders(req);

		headers = HTReplyHeaders(req);
		if (headers) {
		    (*req->output_stream->isa->put_string)(req->output_stream, headers);
		    free(headers);
		}
	    }

	    (*req->output_stream->isa->put_string)(req->output_stream, buf);
	    (*req->output_stream->isa->_free)(req->output_stream);
	    free(buf);
	}
    }
    return HT_LOADED;
}


/*                                                              HTErrorMsg
**
**      Creates a HTML error message from using the whole error_stack.
**      Only if the global variable HTErrorInfoPath != NULL, an anchor
**      will be created to an message help file.
*/
PUBLIC void HTErrorMsg ARGS1(HTRequest *, req)
{
    HTList *cur = req->error_stack;
    BOOL highest = YES;
    HTErrorInfo *pres;
    char body[4000];				/* @@@ SHOULD BE A CHUNK @@@ */
    char * s = body;
    *s = 0;

    if (!req->output_stream) return;

    if (req->error_block)
        goto do_free;

    out.status_code = 500;
    HTReasonLine = "Error";

    /* Output messages */
    while ((pres = (HTErrorInfo *) HTList_nextObject(cur))) {

        /* Check if we are going to show the message */
        if ((!pres->ignore || HTErrorShowMask & HT_ERR_SHOW_IGNORE) && 
            (HTErrorShowMask & pres->severity)) {

            /* Output code number */
            if (highest) {                          /* If first time through */
                if (TRACE)
                    fprintf(stderr,
                            "HTError..... Generating error message.\n");
                /* Output title */
                strcpy(s,"<HTML>\n<HEAD>\n<TITLE>Error Message</TITLE>\n</HEAD>\n<BODY>\n<H1>");
                s += strlen(s);

                if (pres->severity == ERR_WARNING)
                    strcat(s, "Warning ");
                else if (pres->severity == ERR_NON_FATAL)
                    strcat(s, "Non Fatal Error ");
                else if (pres->severity == ERR_FATAL)
                    strcat(s, "Fatal Error ");
                else {
                    strcat(s, "Unknown Classification of Error</H1>\n");
                    goto output_errors;
                }

                /* Only output error code if it is a real HTTP code */
                if (pres->element < HTERR_HTTP_CODES_END) {
                    char codestr[20];
                    sprintf(codestr, "%d", error_info[pres->element].code);
                    strcat(s, codestr);
                    out.status_code = error_info[pres->element].code;
                    HTReasonLine = get_http_reason(out.status_code);
                }
                strcat(s, "</H1>\n");
                highest = NO;
            } else {
                strcat(s, "<B>Reason:</B> ");
            }

            /* Output error message */
 	    if (HTErrorShowMask & HT_ERR_SHOW_LINKS) {
		CONST char *prefix = HTErrorGetPrefix();
		strcat(s, "<A HREF=\"");
		if (prefix)
		    strcat(s, prefix);
		strcat(s, error_info[pres->element].url);
		strcat(s, "\">");
	    }
            if (pres->element != HTERR_SYSTEM) {
                strcat(s, error_info[pres->element].msg);
                strcat(s, ":");
            }
 	    if (HTErrorShowMask & HT_ERR_SHOW_LINKS) {
		strcat(s, "</A>");
	    }
            strcat(s, " ");

            /* Output parameters */
            if (pres->par && HTErrorShowMask & HT_ERR_SHOW_PARS) {
                int cnt;
                strcat(s, " ");
                for (cnt=0; cnt<pres->par_length; cnt++) {
                    if (*((char *)(pres->par)+cnt) < 0x20 ||
                        *((char *)(pres->par)+cnt) >= 0x7F)
                        strcat(s, "-");
                    else {
                        s += strlen(s);
                        *s = *((char *)(pres->par)+cnt);
                        *++s = 0;
                    }
                }
                strcat(s, ".\n");
            }

            /* Output location */
            s += strlen(s);
            if (pres->where && HTErrorShowMask & HT_ERR_SHOW_LOCATION) {
                strcat(s, "<BR>\nThis occured in ");
                strcat(s, pres->where);
                strcat(s, "\n");
            }
            
            /* If we only are going to show the higest entry */
            if (HTErrorShowMask & HT_ERR_SHOW_FIRST)
                break;
            strcat(s, "<P>\n");
        }
    }

  output_errors:
    s += strlen(s);
    strcat(s, "\n<P><HR>\n<ADDRESS><A HREF=\"http://www.w3.org\">\n ");
    strcat(s, HTAppName);
    strcat(s, HTAppVersion);
    strcat(s, "</A></ADDRESS>\n</BODY>\n</HTML>\n");

    req->content_type = WWW_HTML;
    out.content_length = strlen(body);
    if (!out.http_header_sent) {
        char * headers = HTReplyHeaders(req);
        if (headers) {
            (*req->output_stream->isa->put_string)(req->output_stream,headers);
            free(headers);
        }
        out.http_header_sent = YES;
    }
    (*req->output_stream->isa->put_string)(req->output_stream,body);

  do_free:
    (*req->output_stream->isa->_free)(req->output_stream);
}



PUBLIC int HTLoadStrToStream ARGS2(HTStream *,    sink,
                                   CONST char *,  str)
{
    if (!sink || !sink->isa || !str) return -1;
    (*sink->isa->put_string)(sink, str);
    return 0;
}


PUBLIC int HTCloseStream ARGS1(HTStream *, sink)
{
    if (sink && sink->isa) {
        (*sink->isa->_free)(sink);
        return 0;
    }
    return -1;
}



/*____________________________________________________________________
**
**                      Networking code
*/

/*              Bind to a TCP port
 *              ------------------
 *
 * On entry,
 *      port    is the port number to listen to;
 *              0       means data is taken from stdin
 *              80      means "listen to anyone on port 80"
 *
 * On exit,
 *      returns         Negative value if error.
 */
int do_bind ARGS1(int, port)
{
#ifdef SELECT
    FD_ZERO(&open_sockets);     /* Clear our record of open sockets */
    num_sockets = 0;
#endif

    if (!port) {
        /*
         *      Inetd => use stdin/stdout
         */
        dynamic_allocation = FALSE;             /* not dynamically allocated */
        role = passive; /* Passive: started by daemon */

#ifdef VMS
        return 0;

#ifdef OLD_CODE /* Got the channel at the top of main() -- F.Macrides */
        {   unsigned short channel;         /* VMS I/O channel */
            struct string_descriptor {      /* This is NOT a proper descriptor*/
                    int size;               /*  but it will work.             */
                    char *ptr;              /* Should be word,byte,byte,long  */
            } sys_input = {10, "SYS$INPUT:"};
            int status;             /* Returned status of assign */
            extern int sys$assign();

            status = sys$assign(&sys_input, &channel, 0, 0);
            com_soc = channel;  /* The channel is stdin */
            CTRACE(tfp, "IP.......... Opened PASSIVE socket %d\n", channel);
            return 1 - (status&1);
        }       
#endif /* OLD_CODE */
#else /* not VMS */
        com_soc = 0;        /* The channel is stdin */
        CTRACE(tfp, "IP.......... PASSIVE socket 0 assumed from inet daemon\n");
        return 0;               /* Good */
#endif /* not VMS */

    } else {
        /*
         *      Standalone => really listen to port
         */
        char *p;                /* pointer to string */
        char *q;
        struct hostent  *phost;     /* Pointer to host - See netdb.h */
        char buffer[256];               /* One we can play with */
        register SockA * sin = &server_soc_addr;

        sprintf(buffer, "*:%d", port);
        p = buffer;

        /*
         *      Set up defaults:
         */
#ifdef DECNET
        sin->sdn_family = AF_DECnet;        /* Family = DECnet, host order  */
        sin->sdn_objnum = 0;                /* Default: new object number, */
#else  /* Internet */
        sin->sin_family = AF_INET;          /* Family = internet, host order  */
        sin->sin_port = 0;                  /* Default: new port,    */
#endif /* Internet */
        dynamic_allocation = TRUE;          /*  dynamically allocated */
        role = passive;                     /*  by default */

        /*  Check for special characters: */
        if (*p == WILDCARD) {           /* Any node */
            role = master;
            p++;
        }

        /*  Strip off trailing port number if any: */
        for(q=p; *q; q++)
            if (*q==':') {
                int status = 0;
                *q++ = 0;               /* Terminate node string */
#ifdef DECNET
                sin->sdn_objnum =
                    (unsigned char)HTCardinal(&status, &q, (unsigned int)65535);
#else  /* Internet */
                sin->sin_port =
                    htons((unsigned short)HTCardinal(&status, &q,
                                                     (unsigned int)65535));
                if (status<0) return status;
#endif /* Internet */
                if (*q) return -2;  /* Junk follows port number */
                dynamic_allocation = FALSE;
                break;      /* Exit from loop before we skip the zero */
            } /*if*/

        /* Get node name: */
#ifdef DECNET  /* Empty address (don't care about the command) */
        sin->sdn_add.a_addr[0] = 0;
        sin->sdn_add.a_addr[1] = 0;
        CTRACE(tfp, "Daemon...... Parsed address as port %d, DECnet %d.%d\n",
               (int) sin->sdn_objnum,
               (int) sin->sdn_add.a_addr[0],
               (int) sin->sdn_add.a_addr[1] ) ;
#else /* Internet */
        if (*p == 0) {
            sin->sin_addr.s_addr = INADDR_ANY; /* Default: any address */

        } else if (*p>='0' && *p<='9') {   /* Numeric node address: */
            sin->sin_addr.s_addr = inet_addr(p); /* See arpa/inet.h */

        } else {                    /* Alphanumeric node name: */
            phost=gethostbyname(p);     /* See netdb.h */
            if (!phost) {
                CTRACE(tfp,
                       "IP.......... Can't find internet node name \"%s\"\n",p);
                return HTInetStatus("gethostbyname");  /* Fail? */
            }
            memcpy(&sin->sin_addr, phost->h_addr, phost->h_length);
        }
        CTRACE(tfp, 
               "Daemon...... Parsed address as port %d, inet %d.%d.%d.%d\n",
               (int)ntohs(sin->sin_port),
               (int)*((unsigned char *)(&sin->sin_addr)+0),
               (int)*((unsigned char *)(&sin->sin_addr)+1),
               (int)*((unsigned char *)(&sin->sin_addr)+2),
               (int)*((unsigned char *)(&sin->sin_addr)+3));
        HTServerPort = (int)ntohs(sin->sin_port);
#endif /* Internet */
    } /* scope of p */


    /*  Master socket for server: */
    if (role == master) {
        int one=1;
        struct linger Linger;
        Linger.l_onoff = 0;
        Linger.l_linger = 0;

        /*  Create internet socket */
#ifdef DECNET
        master_soc = socket(AF_DECnet, SOCK_STREAM, 0);
#else /* Internet */
        master_soc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif /* Internet */
        if (master_soc<0) {
            fprintf(stderr, "Couldn't create socket\n");
            HTLog_error("Couldn't create socket");
            return HTInetStatus("socket");
        }
        CTRACE(tfp, "IP.......... Opened socket number %d\n", master_soc);


        if ((setsockopt(master_soc,SOL_SOCKET,SO_REUSEADDR,
                        (char*)&one,sizeof(one)))
            == -1) {
            fprintf(stderr, "Couldn't set socket option\n");
            HTLog_error("Couldn't set socket option");
        }

/*
 * Trying again without these on Unix...
 */
#ifdef VMS
        if ((setsockopt(master_soc,SOL_SOCKET,SO_KEEPALIVE,
                        (char*)&one,sizeof(one)))
            == -1) {
            fprintf(stderr, "Couldn't set socket option\n");
            HTLog_error("Couldn't set socket option");
        }
        if ((setsockopt(master_soc,SOL_SOCKET,SO_LINGER,
                        (char*)&Linger,sizeof(Linger)))
            == -1) {
            fprintf(stderr, "Couldn't set socket option\n");
            HTLog_error("Couldn't set socket option");
        }
#endif /* VMS */


        /*  If the port number was not specified look for a free one */
#ifndef DECNET  /* irrelevant: no inetd */
        if (dynamic_allocation) {
            unsigned short try;
            for (try=FIRST_TCP_PORT; try<=LAST_TCP_PORT; try++) { 
                server_soc_addr.sin_port = htons(try);
                if (bind(master_soc,
                         (struct sockaddr*)&server_soc_addr,
                         sizeof(server_soc_addr)) == 0)
                    break;
                if (try == LAST_TCP_PORT)
                    return HTInetStatus("bind");
            }
            CTRACE(tfp, "IP..........  Bound to port %d.\n",
                   ntohs(server_soc_addr.sin_port));
        } else
#endif
        {                                       /* Port was specified */
            if (bind(master_soc,
                     (struct sockaddr*)&server_soc_addr,
                     sizeof(server_soc_addr))<0)
                return HTInetStatus("bind");
        }
        if (listen(master_soc, LISTEN_BACKLOG)<0)
            return HTInetStatus("listen");

        CTRACE(tfp, "Daemon...... Master socket(), bind() and listen() all OK\n");
#ifdef SELECT
        FD_SET(master_soc, &open_sockets);
        if ((master_soc+1) > num_sockets) num_sockets=master_soc+1;
#endif
        return master_soc;
    } /* if master */
    
    return -1;          /* unimplemented role */

} /* do_bind */



/*
 *      Timout handling
 */

PRIVATE void clean_exit NOARGS
{
    if (com_soc > 0) {
        shutdown(com_soc, 2);
        NETCLOSE(com_soc);
    }
    exit(0);
}

PRIVATE void input_timed_out NOARGS
{
    HTLog_error("Timed out when reading request");
    clean_exit();
}

PRIVATE void output_timed_out NOARGS
{
    HTLog_error("Timed out while sending response");
    clean_exit();
}

PRIVATE void script_timed_out NOARGS
{
    HTLog_error("Script timed out");

#ifndef VMS
    CTRACE(stderr,
      "Sending..... TERM signal to script and waiting for it to do cleanup\n");
    kill(script_pid, SIGTERM); /* Script may catch TERM signal to do cleanup */
    sleep(5);

    CTRACE(stderr, "Sending..... KILL signal to script\n");
    kill(script_pid, SIGKILL); /* This one really kills it */

#ifdef NeXT
    wait((union wait *)0);
#else
    {
#ifdef USE_WAITPID
	waitpid(script_pid, NULL, 0);
#else /* ! USE_WAITPID */
        int status;				   /* union wait is obsolete */
	wait3(&status, 0, NULL);
#endif /* USE_WAITPID */
    }
#endif /* not Next */
#endif /* not VMS */

    CTRACE(stderr, "Script is... dead.  Long live The Script.\n");

    clean_exit();
}

PRIVATE void cleanup_timed_out NOARGS
{
    /* No error log entry since that would probably block, too */
    CTRACE(stderr,"TIMED OUT... after serving [probably on log file write]\n");
    clean_exit();
}


#ifdef VMS
#define START_TIMER(f,t) HTVMS_start_timer(0,f,t)
#define CANCEL_TIMER     HTVMS_cancel_timer(0)
#else /* not VMS */
#define START_TIMER(f,t) { signal(SIGALRM,(void(*)()) f); alarm(t); }
#define CANCEL_TIMER     { alarm(0); signal(SIGALRM,SIG_IGN); }
#endif /* not VMS */


PRIVATE void input_timeout_on NOARGS
{
    START_TIMER(input_timed_out, sc.input_timeout);
}

PUBLIC void output_timeout_on NOARGS
{
    START_TIMER(output_timed_out, sc.output_timeout);
}

PUBLIC void script_timeout_on ARGS1(int, pid)
{
    CTRACE(stderr, "TimeOut..... set for process %d (%d secs)\n",
           pid, sc.script_timeout);
    script_pid = pid;
    START_TIMER(script_timed_out, sc.script_timeout);
}

PUBLIC void cleanup_timeout_on NOARGS
{
    START_TIMER(cleanup_timed_out, DEFAULT_CLEANUP_TIMEOUT);
}

PUBLIC void timeout_off NOARGS
{
    script_pid = 0;
    CANCEL_TIMER;
}



/*      Handle one message
**      ------------------
**
** On entry,
**      soc             A file descriptor for input and output.
** On exit,
**      returns         >0      Channel is still open.
**                      0       End of file was found, please close file
**                      <0      Error found, please close file.
*/
PUBLIC int HTHandle ARGS1(int, soc)
{
    HTRequest *req;
    int status;
    int redir_hops = 0;
    HTInputSocket * isoc;

    reset_server_env();

    isoc = HTInputSocket_new(soc);

    input_timeout_on();
    req = HTParseRequest(isoc);
    timeout_off();

    if (!req) {
        HTLog_error("Request parsing failed fatally");
        goto clean_up;
    }

    HTSoc = soc;
    req->isoc = isoc;
    req->output_stream = HTWriter_newNoClose(soc);

    if (req->method == METHOD_INVALID) {
        if (HTReqLine) {
            char * msg = (char*)malloc(strlen(HTReqLine) + 60);
            sprintf(msg, "Invalid request \"%s\" (unknown method)", HTReqLine);
#if 0
            out.status_code = 400;
            HTReasonLine = "Invalid method";
#endif
            HTLoadError(req, 400, msg);
            free(msg);
        }
        else HTLoadError(req, 400,
                         "Invalid request -- completely unable to parse it");
        goto clean_up;
    }
    if (sc.disabled[(int)req->method]) {
        char msg[80];
        sprintf(msg, "Method %s is disabled on this server",
                HTMethod_name(req->method));
#if 0
        out.status_code = 403;
        HTReasonLine = "Disabled method";
#endif
        HTLoadError(req, 403, msg);
        goto clean_up;
    }
    if (!HTReqArg) {
        char * msg = (char*)malloc((HTReqLine ?strlen(HTReqLine) :0)+30);
        if (!msg) outofmem(__FILE__, "HTHandle");
        sprintf(msg, "Invalid request \"%s\"",
                (HTReqLine ? HTReqLine : "-null-"));
#if 0
        out.status_code = 400;
        HTReasonLine = "Invalid request";
#endif
        HTLoadError(req, 400, msg);
        free(msg);
        goto clean_up;
    }


do_redirection_on_fly:

/*
**      Check Access Authorization (and translate by rules at the same time).
*/

#ifdef VMS
    /* enable sysprv to get to security files */
    HTVMS_enableSysPrv();
#endif /* VMS */

    out.status_code = HTAA_checkAuthorization(req);
    CTRACE(stderr, "AA.......... check returned %d\n", out.status_code);
    CTRACE(stderr, "Translated.. \"%s\"\n",
           HTReqTranslated ? HTReqTranslated : "-null-");

#ifdef VMS
    HTVMS_disableSysPrv();
#endif /* VMS */

    if (out.status_code != 200  &&  out.status_code != 302 &&
	out.status_code != 301) {
        HTLoadError(req, out.status_code, HTAA_statusMessage(req));
        goto clean_up;
    }
    /* otherwise access is authorized, continue */

#ifdef VMS
  if (HTVMS_authSysPrv() == YES) {
      char *filename = HTReqTranslated;
      char *uid_name = NULL;

      if (!filename)
         filename = HTReqScript;

      /* if SETUID is specified we will try to access the file for that UID */
      if (sc.do_setuid  &&  HTUser  &&  HTUser->username)
         uid_name = HTUser->username;

      /* if UID still not specified we take the one from the protect rule */
      if (!uid_name)
         uid_name = HTAA_getUidName();

      /* check if we can access the file from UID; GID is ignored */
      if ((filename) && (0 != strcmp(uid_name,""))) {
         CTRACE(stderr, "HTHandle.... checking access to file '%s' for user '%s'\n",
                     filename, uid_name);

         /* enable SYSPRV if indeed we have access; actual access is done via system field in the protection */
         HTVMS_enableSysPrv();

        CTRACE(stderr,
               "WARNING..... serving document when running with SYSPRV\n");

         if (HTVMS_checkAccess(filename,uid_name,req->method) != YES) {
            HTVMS_disableSysPrv();
            HTLoadError(req, 403, "Forbidden -- by rule");
            goto clean_up;
         }

         /* access under VMS authorized, go on with SYSPRV enabled... */
      } /* No Protection defined, or no uidname given */
  }
  else {
      CTRACE(stderr, "HTHandle.... Not running with SYSPRV\n");
  }

#else /* not VMS */
    if (getuid() == 0) {
        struct passwd *pw = NULL;
        int uid = 0;
        int gid = 0;

        if (sc.do_setuid &&  HTUser  &&  HTUser->username) {
            pw = getpwnam(HTUser->username);
            if (pw) {
                uid = pw->pw_uid;
                gid = pw->pw_gid;
                CTRACE(stderr, "SETUID...... %s [uid %d] [gid %d]\n",
                       HTUser->username, uid, gid);
            }
            else {
                HTLog_error2("SETUID ERROR no such user:", HTUser->username);
            }
        }
        if (!pw) { 
            uid = HTAA_getUid();
            gid = HTAA_getGid();
        }

        if (!pw && !(pw = getpwuid(uid)))
            HTLog_errorN("Failed to get passwd entry for uid",uid);
        else if (initgroups(pw->pw_name, gid) == -1)
            HTLog_error2("Failed to init groups for user",pw->pw_name);

        CTRACE(stderr, "Doing... setgid(%d) and setuid(%d)\n", gid, uid);
        if (setgid(gid) == -1)
            HTLog_errorN("Failed to set group id to", gid);
        if (setuid(uid) == -1) {
            HTLog_errorN("Failed to set user id to", uid);
            CTRACE(stderr, "WARNING..... serving doc when running as root\n");
        }
    }
    else {
        CTRACE(stderr, "HTHandle.... Not running as root (uid=%d)\n",
               (int) getuid());
    }
#endif /* not VMS */


    /*
     *  If HEAD add a stream that makes sure we only send the head
     */
    if (req->method == METHOD_HEAD)
        req->output_stream = HTHeadStream(req->output_stream);

    /*
     *  Handle command
     */
    if (HTReason == HTAA_OK_GATEWAY) {  /* NOT a local file -- gatewaying */

        output_timeout_on();
        HTImProxy = YES;
        proxy_access = YES;     /* Write to proxy log instead of access log */
        HTImServer = HTReqTranslated;

        if (cc.cache_root  &&  !req->authorization  &&  !HTReqArgKeywords  &&
            (req->method==METHOD_GET ||
             (req->method==METHOD_HEAD &&
              (!strncmp(HTReqTranslated,"ftp:",4) ||
               !strncmp(HTReqTranslated,"gopher:",7))))) {

            char * cfn = NULL;
            FILE * cf = NULL;
            time_t if_ms = 0;
            time_t expires = 0;
            HTCacheTask task;

            /*
             *  If a conditional GET request make sure we send only
             *  304 Not modified response if not modified.
             */
            if (HTIfModifiedSince)
                req->output_stream = HTIfModSinceStream(HTIfModifiedSince,
                                                        req->output_stream);

            /*
             * We don't want to die when we are doing cache operations,
             * even if we get a SIGPIPE signal.
             */
            ignore_sigpipes = YES;

            task = cache_lookup(HTReqTranslated,&cfn,&cf,&if_ms,&expires);

            if (task == CACHE_FOUND) {          /* Cache file exists */
                ignore_sigpipes = NO;
                CTRACE(stderr,"Cache hit... Reading from \"%s\"\n", cfn);
                cf = fopen(cfn, "r");
                if (cf) {
                    HTLoadCacheToStream(cf, req->output_stream, expires);
                    fclose(cf);
                    (*req->output_stream->isa->_free)(req->output_stream);
                    status = HT_LOADED;
                }
                else {
                    HTLog_error2("Couldn't open cache file for reading:",cfn);
                    /* Load from remote host */
                    if (cc.cache_no_connect)
                        status = HTLoadError(req, 400, "Internal proxy cache standalone mode error; couldn't open cache file for reading.");
                    else {
                        HTProxyHeaders = hbuf_proxy_headers(req);
                        status = HTLoadToStream(HTReqArgPath, NO, req);
                    }
                }
            }
            else if (cc.cache_no_connect) {     /* Standalone cache mode */
                if (task == CACHE_NO)
                    HTLoadError(req, 404, 
"Proxy is in standalone cache mode [no external connections enabled] and the document was <B>not found in the cache</B>.");
                else /* task == CACHE_IF_MODIFIED */
                    HTLoadError(req, 404, 
"Proxy is in standalone cache mode with expiry checking enabled; document <B>was found in the cache but it had expired</B>. If it is desirable to see all cached documents, even if they have expired, expiry checking should be turned off.");
            }
            else {
                if (task == CACHE_CREATE && cfn && cf) {
                    /* Write to cache file */
                    req->output_stream =
                        HTProxyCache(req->output_stream,
                                     cf, cfn, HTReqTranslated, (time_t)0);
                    CTRACE(stderr,
                           "Cache....... Writing to lockfile \"%s%s\"\n",
                           cfn, LOCK_SUFFIX);
                }
                else if (task == CACHE_IF_MODIFIED && cfn && cf) {
                    CTRACE(stderr,"Cache....... IMS-GET %s",ctime(&if_ms));
                    req->output_stream =
                        HTProxyCache(req->output_stream,
                                     cf, cfn, HTReqTranslated, if_ms);
                    HTIfModifiedSince = if_ms;
                }
                else {          /* task == CACHE_NO */
                    CTRACE(stderr, "Cache....... Not caching\n");
                    ignore_sigpipes = NO;
                }

                /*
                 * Calculate content-length for log if http: access
                 * (others we get automatically).
                 */
                if (HTReqTranslated && !strncmp(HTReqTranslated,"http:",5)) {
                    CTRACE(stderr,
                           "Need to..... find content-length for http:\n");
                    req->output_stream = HTBodyStream(req->output_stream);
                }
                HTProxyHeaders = hbuf_proxy_headers(req);
                status = HTLoadToStream(HTReqArgPath, NO, req);
            }
            FREE(cfn);
        }
        else {
            /* Normal retrieve with no caching */
            CTRACE(stderr, "No caching.. %s\n",
                   !cc.cache_root               ? "enabled" :
                   req->authorization           ? "for protected documents" :
                   HTReqArgKeywords             ? "queries" :
                   "for other methods than GET (or HEAD with FTP and gopher)");
            /*
             * Calculate content-length for log if http: access
             * (others we get automatically).
             */
            if (HTReqTranslated && !strncmp(HTReqTranslated,"http:",5)) {
                CTRACE(stderr,"Need to..... find content-length for http:\n");
                req->output_stream = HTBodyStream(req->output_stream);
            }
            HTProxyHeaders = hbuf_proxy_headers(req);
            status = HTLoadToStream(HTReqArgPath, NO, req);
        }

        HTImProxy = NO;
        timeout_off();

    }
    else if (HTReason == HTAA_OK_REDIRECT ||
	     HTReason == HTAA_OK_MOVED) {    /* Do a redirection */
        CTRACE(stderr, "Redirection.\n");
        output_timeout_on();
        status = HTLoadRedirection(req);
        timeout_off();
    }
    else if (HTReqScript) {     /* Call a script to generate a document */
        CTRACE(stderr, "Script call.\n");
        status = HTCallScript(req);
    }
    else if (HTReqArgKeywords) {
        CTRACE(stderr, "Search......\n");
        if (rc.search_script) { /* Search enabled */
            if (HTReqTranslated) {
                StrAllocCopy(HTScriptPathTrans, HTReqTranslated);
                StrAllocCopy(HTScriptPathInfo, HTReqArgPath);
                StrAllocCopy(HTReqScript, rc.search_script);
                status = HTCallScript(req);
            }
            else {
                HTLog_error("Translated NULL when should do search");
                HTLoadError(req, 403, "Sorry, forbidden by rule");
            }
        }
        else {
            CTRACE(stderr, "HTHandle.... can't perform search %s\n", HTReqArg);
            status =
                HTLoadError(req, 501,
                            "Sorry, this server does not perform searches.");
        }
    } /* If keywords given */
    else {  /* Load the document normally into the client */

        CTRACE(stderr, "HTHandle.... method %s\n", HTMethod_name(req->method));

        output_timeout_on();

        if (req->method == METHOD_GET  ||
            req->method == METHOD_CHECKOUT) {

            if (HTIfModifiedSince  &&  HTLastModified  &&
                parse_http_time(HTLastModified) <= HTIfModifiedSince) {
                CTRACE(stderr,
                       "Not modified since %s", ctime(&HTIfModifiedSince));
                out.status_code = 304;
                HTReasonLine = "Not modified";
                status = HTLoadHead(req);
            }
            else
                status = HTRetrieve(req);

        } else if (req->method == METHOD_HEAD) {

            status = HTLoadHead(req);

        } else if (req->method == METHOD_PUT  ||
                   req->method == METHOD_CHECKIN) {

            if (rc.put_script) {
                CTRACE(stderr,
                       "HTHandle.... handling PUT with script \"%s\"\n",
                       rc.put_script);

                StrAllocCopy(HTReqScript, rc.put_script);
                StrAllocCopy(HTScriptPathInfo, HTReqArgPath);
                HTScriptPathTrans = HTParse(HTReqTranslated, "",
                                            PARSE_PATH | PARSE_PUNCTUATION);
                status = HTCallScript(req);
            }
            else {
                HTLoadError(req, 500,
                            "This server is not configured to handle PUT");
            }

        } else if (req->method == METHOD_POST) {

            if (rc.post_script) {
                CTRACE(stderr,
                       "HTHandle.... handling POST with script \"%s\"\n",
                       rc.post_script);

                StrAllocCopy(HTReqScript, rc.post_script);
                StrAllocCopy(HTScriptPathInfo, HTReqArgPath);
                HTScriptPathTrans=HTParse(HTReqTranslated, "",
                                          PARSE_PATH | PARSE_PUNCTUATION);
                status = HTCallScript(req);
            }
            else {
                HTLoadError(req, 500,
                            "This server is not configured to handle POST");
            }

        } else if (req->method == METHOD_DELETE) {

            if (rc.delete_script) {
                CTRACE(stderr,
                       "HTHandle.... handling DELETE with script \"%s\"\n",
                       rc.delete_script);

                StrAllocCopy(HTReqScript, rc.delete_script);
                StrAllocCopy(HTScriptPathInfo, HTReqArgPath);
                HTScriptPathTrans=HTParse(HTReqTranslated, "",
                                          PARSE_PATH | PARSE_PUNCTUATION);
                status = HTCallScript(req);
            }
            else {
                HTLoadError(req, 500,
                            "This server is not configured to handle DELETE");
            }

        } else {
            char *msg;

            msg = (char*)malloc(strlen(HTMethod_name(req->method))+50);
            if (!msg) outofmem(__FILE__, "HTHandle");

            sprintf(msg, "Method '%s' invalid or not implemented",
                    HTMethod_name(req->method));
            HTLoadError(req, 501, msg);
            CTRACE(stderr, "Daemon...... %s\n", msg);
            free(msg);
        }

        timeout_off();

    } /* Handle command */

    if (status == HT_REDIRECTION_ON_FLY) {
        CTRACE(stderr, "Redirection. on the fly for \"%s\"\n", HTLocation);
        if (++redir_hops > 10)
            HTLoadError(req, 501,
            "Too many redirection-on-the-fly hops, max 10 (probably looping)");
        else {
            if (req->method != METHOD_GET  &&  req->method != METHOD_HEAD)
                req->method = METHOD_GET;
            FREE(HTReqArg);     /* Leak fixed AL 6 Feb 1994 */
            HTReqArg = HTLocation;
            HTLocation = NULL;
            compute_server_env();

            goto do_redirection_on_fly;
        }
    }

#ifdef VMS
    /* disable SYSPRV again */
    HTVMS_disableSysPrv();
#endif /* VMS */


 clean_up:

#ifdef VMS
#ifdef ACCESS_AUTH
   HTSetCurrentIds(0);
#endif /* ACCESS_AUTH */
#endif /* VMS */

    cleanup_timeout_on();

    /*
    **  Log the call
    */
    HTLog_access(req);
    if (req) {
        if (req->isoc) HTInputSocket_free(req->isoc);
        HTRequest_delete(req);
    }

    timeout_off();

    return 0;           /* End of file - please close socket */

} /* HTHandle */



#ifndef VMS
PRIVATE void gc_and_exit ARGS2(int,     how_many_times,
                               int,     exit_status)
{
    int round = 1;

    if (getuid() == 0) {
        int uid = HTAA_getUid();
        int gid = HTAA_getGid();
        CTRACE(stderr, "Gc...... setgid(%d) and setuid(%d)\n", gid, uid);
        if (setgid(gid) == -1)
            HTLog_errorN("Failed to set group id to", gid);
        if (setuid(uid) == -1) {
            HTLog_errorN("Failed to set user id to", uid);
        }
    }
    CTRACE(stderr,"Lowering.... process priority while doing gc\n");
    nice(6);
    do {
        CTRACE(stderr, "Child....... doing gc round #%d\n\n",round);
        gc();
        CTRACE(stderr, "Child....... gc round #%d complete\n",round);
        if (how_many_times > 1)
            read_gc_results();
    } while (round++ < how_many_times);

    CTRACE(stderr, "Child....... exiting with status %d\n", exit_status);
    exit(exit_status);
}
#endif /* not VMS */



/*                                                    standalone_server_loop()
 *      New server_loop() for Unixes in standalone mode
 */
#if !defined(VMS) && defined(FORKING)
PRIVATE int standalone_server_loop NOARGS
{
    int soc_addr_len = sizeof(client_soc_addr);
    int tcp_status;
    int timeout;

    CTRACE(stderr,"ServerLoop.. Unix standalone\n");

    for(;;) {
      do_again_select:
        timeout = get_next_timeout();
        if (timeout < 0) {
            CTRACE(stderr, "No timeout.. not doing select()\n");
        }
        else {
            fd_set              read_chans;
            fd_set              write_chans;
            fd_set              except_chans;
            struct timeval      max_wait;
            int nfound;

            read_chans = open_sockets;
            FD_ZERO(&write_chans);
            FD_ZERO(&except_chans);
            max_wait.tv_sec = timeout/100;
            max_wait.tv_usec = (timeout%100)*10000;

            if (TRACE) {
                fprintf(stderr,"Next timeout after %d hours %d mins %d secs\n",
                        timeout/360000, (timeout/6000)%60, (timeout/100)%60);
                fprintf(stderr, "Daemon...... %s (Mask=%x hex, max=%x hex).\n",
                        "Waiting for connection",
                        *(unsigned int *)(&read_chans),
                        (unsigned int)num_sockets);
            }

#ifdef __hpux
	    nfound = select(num_sockets, (int *) &read_chans,
			    (int *) &write_chans, (int *) &except_chans,
			    timeout >= 0 ? &max_wait : NULL);
#else
	    nfound = select(num_sockets, &read_chans,
			    &write_chans, &except_chans,
			    timeout >= 0 ? &max_wait : NULL);
#endif
            if (nfound < 0) {   /* Interrupted */
                if (errno != EINTR) (void)HTInetStatus("select");
                goto do_again_select;
            }

            if (nfound == 0) {  /* Timeout */
                CTRACE(stderr,"Time to do.. daily garbage collection\n");
                gc_pid = fork();
                if (!gc_pid) {  /* Child */
                    gc_and_exit(1,0);
                }
                else {
                    if (gc_pid < 0) {
                        HTLog_error("FORK FAILED when launching gc\n");
                        gc_pid = 0;
                    }
                }
                goto do_again_select;
            }
            CTRACE(stderr,"Daemon...... New incoming connection\n");
        }

        CTRACE(tfp, "Daemon...... accepting connection...\n");
        com_soc = accept(master_soc,
                         (struct sockaddr *)&client_soc_addr,
                         &soc_addr_len);
        if (com_soc < 0) {
            if (errno != EINTR) (void)HTInetStatus("accept");
            goto do_again_select;       /* Have do select because we might   */
                                        /* need to turn timeout on this time */
                                        /* (if gc just returned)             */
        }
        CTRACE(tfp,"Accepted.... new socket %d\n",com_soc);

        /*
         * com_soc is now valid for read. Do fork() to handle it.
         */
        {
            int pid;
            BOOL do_gc = NO;

            time(&cur_time);

            CTRACE(stderr, "StandAlone.. Doing fork()\n");
            child_serial++;
            if (cc.cache_root && !gc_pid && time_to_do_gc())  do_gc = YES;
            pid = fork();

            if (pid != 0) {     /* fork() failed or parent */
                NETCLOSE(com_soc); /* We can close this -- child handles it */
                if (pid < 0) {  /* fork() failed */
                    HTLog_error("fork() FAILED");
                }
                else if (pid > 0) {     /* Parent */
                    CTRACE(stderr, "HTHandle.... fork() succeeded\n");
                    if (do_gc) {
                        CTRACE(stderr, "Parent...... That child will do gc\n");
                        gc_pid = pid;
                        do_gc = NO;
                    }
                }
#ifdef SIGTSTP  /* BSD */
                sig_child();
#endif
            }
            else {      /* Child */
                CTRACE(stderr, "Child....... I'%s\n",
                       do_gc ? "ll do gc upon exit" : "m alive");

                /*
                 * This no-linger stuff is borrowed from NCSA httpd code.
                 * In Linux this causes problems -- is this necessary at
                 * all -- well, I've disabled for now, and if I was wrong
                 * this can be turned on by "Linger On" config directive.
		 * NEEDED ON UTS 2.1 AND OTHER PLATFORMS!!!
                 */
                if (sc.do_linger) {
                    struct linger sl;
                    sl.l_onoff = 1;
                    sl.l_linger = 600; /* currently ignored anyway */

                    setsockopt(com_soc, SOL_SOCKET, SO_LINGER,
                               (char*)&sl, sizeof(sl));
                }
                else {
                    CTRACE(stderr, "Linger...... no longer set by default\n");
                }

                {
                    SockA addr;
                    int namelen = sizeof(addr);

                    memset((char*)&addr, 0, sizeof(addr));
                    getpeername(com_soc, (struct sockaddr*)&addr, &namelen);
#ifdef OLD_CODE
		    /* inet_ntoa() causes a lot of problems */
                    char ip_address[16];
                    if (inet_ntoa(addr.sin_addr)) {
                        strncpy(ip_address, (char*)inet_ntoa(addr.sin_addr),
                                sizeof(ip_address));
                        ip_address[15] = 0;
                    }
                    else {
                        strcpy(ip_address, "0.0.0.0");
                    }
#endif
		    {
			CONST char *host = HTInetString(&addr);
			if (!host || !*host)
			    StrAllocCopy(HTClientHost, "0.0.0.0");
			else
			    StrAllocCopy(HTClientHost, host);
		    }
                    FREE(HTClientHostName);
                    if (sc.do_dns_lookup)
                        HTClientHostName = HTGetHostBySock(com_soc);
                }

                CTRACE(tfp,"Reading..... socket %d from host %s\n",
                       com_soc, HTClientHost);

                remote_ident = NULL;
                if (sc.do_rfc931) {
                    int l = sizeof(server_soc_addr);
                    if (com_soc ||
                        getsockname(fileno(stdout), (struct sockaddr *)
				    &server_soc_addr,&l) != -1)
                        remote_ident=rfc931(&client_soc_addr,&server_soc_addr);
                    else {
                        CTRACE(stderr,"Failed...... to get local sock addr\n");
                    }
                }

                tcp_status = HTHandle(com_soc);

                if (TRACE && tcp_status <= 0) {         /* EOF or error */
                    if (tcp_status < 0)                 /* error */
                        fprintf(tfp,
                                "ERROR....... %d handling msg (errno=%d).\n",
                                tcp_status, errno);
                    else
                        fprintf(tfp,"Socket...... %d disconnected by peer\n",
                                com_soc);
                }
                shutdown(com_soc, 2);
                NETCLOSE(com_soc);
                if (do_gc)
                    gc_and_exit(1, HTChildExitStatus);
                else {
                    CTRACE(stderr,"Child....... exiting with status %d\n",
                           HTChildExitStatus);
                    exit(HTChildExitStatus);
                }
            } /* child */
        }
    } /* for loop */
}
#endif /* standalone_server_loop() is not for VMS and only if we are FORKING */




/*      Handle incoming messages                                server_loop()
**      -------------------------
**
** On entry:
**
**      timeout         -1 for infinite, 0 for poll, else in units of 10ms
**
** On exit,
**      returns         The status of the operation, <0 if failed.
**                      0       means end of file
**
*/
PRIVATE int server_loop NOARGS
{
    int tcp_status;             /* <0 if error, in general */

    for(;;) {

        int soc_addr_len = sizeof(client_soc_addr);

#if !defined(VMS) && defined(FORKING) && defined(SELECT) && !defined(DECNET)
        /*
         * Use new server loop, old one is a mess.
         */
        if (role == master) standalone_server_loop();
#endif

        /*  If it's a master socket, then find a slave: */
        if (role == master) {

#ifdef SELECT
            fd_set              read_chans;
            fd_set              write_chans;
            fd_set              except_chans;
            int                 nfound;     /* Number of ready channels */
            struct timeval      max_wait;   /* timeout in form for select() */

            FD_ZERO(&write_chans);          /* Clear the write mask */
            FD_ZERO(&except_chans);         /* Clear the exception mask */

            for (com_soc=(-1); com_soc<0;) {    /* Loop while connections keep coming */
                int timeout;

              retry_select:

                /*
                 * The read mask expresses interest in the master
                 * channel for incoming connections) or any slave
                 * channel (for incoming messages).
                 */

                /*  Wait for incoming connection or message */
                read_chans = open_sockets;   /* Read on all active channels */
                CTRACE(stderr, "Daemon...... %s (Mask=%x hex, max=%x hex).\n",
                       "Waiting for connection or message (select())",
                       *(unsigned int *)(&read_chans),
                       (unsigned int)num_sockets);

                timeout = get_next_timeout();
                if (timeout < 0) {
                    CTRACE(stderr, "No timeout.. for select()\n");
                }
                else {
                    CTRACE(stderr,
                           "Next timeout after %d hours %d mins %d secs\n",
                           timeout/360000,
                           (timeout/6000)%60,
                           (timeout/100)%60);
                }

                if (timeout>=0) {
                    max_wait.tv_sec = timeout/100;
                    max_wait.tv_usec = (timeout%100)*10000;
                }

#ifdef __hpux
                nfound=select(num_sockets, (int *) &read_chans,
                              (int *) &write_chans, (int *) &except_chans,
                              timeout >= 0 ? &max_wait : NULL);
#else
                nfound=select(num_sockets, &read_chans,
                              &write_chans, &except_chans,
                              timeout >= 0 ? &max_wait : NULL);
#endif
                if (nfound < 0) {       /* Interrupted */
                    if (errno != EINTR) (void)HTInetStatus("select");
                    goto retry_select;
                }

#ifndef VMS
                if (nfound==0) {        /* Timeout */
                    CTRACE(stderr, "Time to do.. daily gc\n");
                    gc_pid = fork();
                    if (!gc_pid) {      /* Child */
                        gc_and_exit(1,0);
                    }
                    else {
                        if (gc_pid < 0) {
                            HTLog_error("FORK FAILED when launching gc\n");
                            gc_pid = 0;
                        }
                    }
                    goto retry_select;
                }
#endif /* not VMS */

                /*
                 * We give priority to existing connected customers. When
                 * there are no outstanding commands from them, we look for
                 * new customers.
                 *
                 * If a message has arrived on one of the channels, take
                 * that channel:
                 */
                {
                    int i;
                    for(i=0; i<num_sockets; i++)
                        if (i != master_soc)
                            if (FD_ISSET(i, &read_chans)) {
                                CTRACE(stderr,
                                       "Message..... waiting on sock %d\n",i);
                                com_soc = i;            /* Got one! */
                                break;
                            }
                    if (com_soc>=0) break; /* Found input socket */
                    
                } /* block */
                
                /*
                 * If an incoming connection has arrived, accept the new
                 * socket:
                 */
                if (FD_ISSET(master_soc, &read_chans)) {
                    CTRACE(tfp, "Daemon...... New incoming connection:\n");

                    tcp_status = accept(master_soc,
                                        (struct sockaddr *)&client_soc_addr,
                                        &soc_addr_len);

                    if (tcp_status<0) {
                        if (errno != EINTR) (void)HTInetStatus("accept");
                        goto retry_select;
                    }

                    CTRACE(tfp,"Accepted.... new socket %d\n",tcp_status);
                    FD_SET(tcp_status, &open_sockets);
                    if ((tcp_status+1) > num_sockets)
                        num_sockets=tcp_status+1;
                } /* end if new connection */
	    } /* loop on event */
#else	/* SELECT not supported */

            CTRACE(tfp,"Daemon...... %s\n",
                   "Waiting for incoming connection (accept())");

#ifdef DECNET
            tcp_status = accept(master_soc, &client_soc_addr, &soc_addr_len);
#else  /* For which machine is this ??? rsoc is undeclared, what's mdp ? */
            tcp_status = accept(master_soc,
                                &rsoc->mdp.soc_tcp.soc_address,
                                &rsoc->mdp.soc_tcp.soc_addrlen);
#endif
            if (tcp_status<0) {
                if (errno != EINTR) (void)HTInetStatus("accept");
                goto retry_select;
            }

            com_soc = tcp_status;       /* socket number */
            CTRACE(tfp, "Accepted.... socket %d\n", tcp_status);
    
#endif /* SELECT not supported */

        }  /* end if master */


/* com_soc is now valid for read */
/* Do fork() to handle it*/

        {
            int fork_status;
            BOOL child = NO;
            BOOL do_gc = NO;

            time(&cur_time);

#ifdef FORKING
            if (HTForkEnabled) {        /* fork -- child handles the request */
                CTRACE(stderr, "server_loop. Doing fork()\n");
                child_serial++;
                if (cc.cache_root  &&  !gc_pid  &&  time_to_do_gc())
                    do_gc = YES;
                fork_status = fork();
            }
            else fork_status = 0;
#else  /* non FORKING */
            fork_status = 0;
#endif /* non FORKING */

            if (fork_status < 0) {      /* fork() failed */
                HTLog_error("fork() FAILED");
            }
            else if (fork_status > 0) { /* Parent */
                CTRACE(stderr, "HTHandle.... fork() succeeded\n");
                if (do_gc) {
                    CTRACE(stderr, "Parent...... That child will do gc\n");
                    gc_pid = fork_status;
                    do_gc = NO;
                }
            }
            if (fork_status != 0) {     /* fork() failed or parent */
                NETCLOSE(com_soc); /* We can close this -- child handles it */
#ifdef SELECT
                FD_CLR(com_soc, &open_sockets);
#endif

#ifdef SIGTSTP  /* BSD */
                /* If previous children have finished wait the zombie
                ** processes away.
                */
                sig_child();
#endif
            }
            else {      /* Child */
                /* This no-linger stuff is borrowed from NCSA httpd code */

#ifdef FORKING
                if (HTForkEnabled) {
                    CTRACE(stderr, "Child....... I'm alive\n");
                    child = YES;
                    if (do_gc) {
                        CTRACE(stderr,"Child....... I will do gc upon exit\n");
                    }
                }
#endif
                if (TRACE && !child)
                    fprintf(stderr, "server_loop. Parent serving\n");

                if (sc.do_linger) {
                    struct linger sl;
                    sl.l_onoff = 1;
                    sl.l_linger = 600; /* currently ignored anyway */
                    /* this should check error status, but it's not crucial */

                    setsockopt(com_soc, SOL_SOCKET, SO_LINGER,
                               (char*)&sl, sizeof(sl));
                }
                else {
                    CTRACE(stderr, "Linger...... no longer set by default\n");
                }

                {
                    SockA addr;
                    int namelen = sizeof(addr);

                    memset((char*)&addr, 0, sizeof(addr));
#ifdef DECNET
                    StrAllocCopy(HTClientHost, "DecnetClient");
                    /* TBD */
                    FREE(HTClientHostName);
#else /* Internet */
                    getpeername(com_soc, (struct sockaddr*)&addr, &namelen);
                    
#ifdef OLD_CODE
		    /* inet_ntoa() causes a lot of problems */
                    char ip_address[16];
                    if (inet_ntoa(addr.sin_addr)) {
                        strncpy(ip_address,
                                (char*)inet_ntoa(addr.sin_addr),
                                sizeof(ip_address));
                        ip_address[15] = 0;
                    }
                    else {
                        strcpy(ip_address, "0.0.0.0");
                    }
#endif
		    {
			CONST char *host = HTInetString(&addr);
			if (!host || !*host)
			    StrAllocCopy(HTClientHost, "0.0.0.0");
			else
			    StrAllocCopy(HTClientHost, host);
		    }
                    FREE(HTClientHostName);
                    if (sc.do_dns_lookup)
                        HTClientHostName = HTGetHostBySock(com_soc);
#endif /* Internet */
                }

                /*  Read the message now on whatever channel there is */
                CTRACE(tfp,"Reading..... socket %d from host %s\n",
                       com_soc, HTClientHost);

                remote_ident = NULL;
                if (sc.do_rfc931) {
                    int l = sizeof(server_soc_addr);
                    if (com_soc ||
                        getsockname(fileno(stdout),(struct sockaddr *)
				    &server_soc_addr,&l) != -1)
                        remote_ident=rfc931(&client_soc_addr,&server_soc_addr);
                    else {
                        CTRACE(stderr,"Failed...... to get local sock addr\n");
                    }
                }

                tcp_status = HTHandle(com_soc);

                if(tcp_status <= 0) {           /* EOF or error */
                    if (tcp_status < 0) {               /* error */
                        CTRACE(tfp,
                               "ERROR....... %d handling msg (errno=%d).\n",
                               tcp_status, errno);
                        /* DONT return HTInetStatus("netread");  error */
                    } else {
                        CTRACE(tfp,
                               "Disconnected Socket %d disconnected by peer\n",
                               com_soc);
                    }
                    if (role==master) {
                        shutdown(com_soc, 2);
                        NETCLOSE(com_soc);
#ifdef SELECT
                        FD_CLR(com_soc, &open_sockets);
#endif /* SELECT */
                    } else {  /* Not multiclient mode */
#ifdef VM
                        return -69;
#else /* not VM */
                        return -ECONNRESET;
#endif /* not VM */
                    }
                } else {/* end if handler left socket open */
                    shutdown(com_soc, 2);
                    NETCLOSE(com_soc);
#ifdef SELECT
                    FD_CLR(com_soc, &open_sockets);
#endif /* SELECT */
                }
#ifndef VMS
                if (child) {
                    if (do_gc)
                        gc_and_exit(1,HTChildExitStatus);
                    else {
                        CTRACE(stderr,"Child....... exiting with status %d\n",
                               HTChildExitStatus);
                        exit(HTChildExitStatus);
                    }
                }
#endif /* not VMS */
            }
        }

    } /* for loop */
    
    /* NOT REACHED */

} /* end server_loop */



#ifdef FORKING
/*
** This function (modified) is taken from:
**      W.Richard Stevens: UNIX Network Programming,
**      Prentice Hall 1990, pp. 82-85
*/
PRIVATE int daemon_start NOARGS
{
    register int        childpid, fd;

#ifndef NO_BACKGROUNDING        /* If this should prove to be non-portable */

    sc.standalone = YES;

    /*
    ** If started by init there's no need to detach
    */
    if (getppid() == 1)
        goto out;

    /*
    ** Fork and let the parent exit to go backgroud
    */
    if ((childpid = fork()) < 0)
        HTLog_error("Can't fork to go background");
    else if (childpid > 0)      /* Parent */
        exit(0);

    /* I'm first child now */

    /*
    ** Disassociate from controlling terminal and process group.
    ** Ensure the process can't reacquire a new controlling terminal.
    */
#ifdef SIGTSTP  /* BSD */

#if defined(__svr4__) || defined(_POSIX_SOURCE) || defined(__hpux)
    pgrp = setsid();
#else
    pgrp = setpgrp(0, getpid());

    if ((fd = open("/dev/tty", O_RDWR)) >= 0) {
        ioctl(fd, TIOCNOTTY, (char*)NULL);      /* Lose controlling tty */
        close(fd);
    }
#endif
    if (pgrp == -1)
        HTLog_error("Can't change process group");

#else   /* SysV */

    pgrp = setpgrp();
    if (pgrp == -1)
        HTLog_error("Can't change process group");

    if ((childpid = fork()) < 0)
        HTLog_error("Can't fork second child");
    else if (childpid > 0)      /* First child */
        exit(0);

    /* I'm second child now */

#endif

  out:

    /*
    ** Move the current working directory to root, to make sure we
    ** aren't on a mounted filesystem.
    */
    chdir("/");

    /*
    ** Clear any inherited file mode creation mask
    */
    umask(0);

#endif /* NO_BACKGROUNDING */

    return 0;
}
#endif /* FORKING */



/*              Main program
 *              ------------
 *
 *      Options:
 *      -v              verbose: turn trace output on to stdout 
 *      -p port         Prefered
 *      -l file         Log requests in ths file
 *      -r file         Take rules from this file
 *
 *      Parameters:
 *      directory       directory to export
 */
PUBLIC int main ARGS2 (int,     argc,
                       char**,  argv)
{
    int status;
    int rulefiles = 0;          /* Count number loaded */
    char *directory = NULL;
    BOOL gc_only = NO;
    BOOL restart_only = NO;

#ifdef SOCKS
    SOCKSinit(argv[0]);
#endif


#ifdef VMS
    /*
    **  The first thing we should always do on VMS is find out if we
    **  are running under Inetd/MULTINET_SERVER/AUX, and if so, get a
    **  channel to the socket for MULTINET and Wollongong.  Then, because
    **  we can't pass parameters to an Inetd process via switches, we get
    **  the rule file via an environment logical.       -- F.Macrides
    */
    BOOLEAN HTRunFromInetd = FALSE;
    char *VMSRuleFileLogical = (char *)malloc(sizeof(char)*256);
    char *VMSRuleFile = (char *)malloc(sizeof(char)*256);


#if defined(MULTINET) || defined(WIN_TCP)
    unsigned short chan;
    $DESCRIPTOR(chan_desc, "SYS$INPUT");
    /*
    **  See if we're running under Inetd/MULTINET_SERVER
    **  and if so, get a channel to the socket.
    */
    if (strstr(getenv("SYS$INPUT"), "_INET")) {
        status = sys$assign(&chan_desc, &chan, 0, 0, 0);
        if (!(status & 1))
            exit(status);
        com_soc = (int) chan;
        HTRunFromInetd = TRUE;
    }
#endif /* MultiNet or Wollongong */

#if defined(UCX) && defined(UCX$C_AUXS)
    /*
    **  This socket call will try to get the client connection
    **  from the AUX Server.  If it succeeds, we are running
    **  from the AUX server (Inetd).
    */
    if ((com_soc = socket (UCX$C_AUXS, 0, 0)) != -1)
        HTRunFromInetd = TRUE;
#endif /* UCX v2+ */

    if (HTRunFromInetd) {
    /*
    **  We can't pass switches to a MultiNet Inetd process, so we use a
    **  system logical (HTTPD_CONFIG<port number> or just HTTPD_CONFIG)
    **  pointing to the rule file, which should include all the
    **  information we'd otherwise pass via switches.  We'll do it
    **  this way for UCX and Wollongong, too.   -- F.Macrides
    **
    **  Track down the rule file.
    */
        struct sockaddr_in serv_addr;
        int length = sizeof(serv_addr);

        /* 
        **  In multiserver environments, the port number is appended
        **  to a system logical for each server's configuration file.
        */
        if (getsockname(com_soc, (struct sockaddr *) &serv_addr,&length) == 0)
            HTServerPort = (ntohs(serv_addr.sin_port));
        else
            exit(SS$_NOIOCHAN);
        sprintf(VMSRuleFileLogical, "HTTPD_CONFIG%d", HTServerPort);
        if (getenv(VMSRuleFileLogical) != NULL) {
            strcpy(VMSRuleFile, getenv(VMSRuleFileLogical));
        } else {
            /*
            **  If that's not defined, try the system logical
            **  without a numeric suffix.
            */
            strcpy(VMSRuleFileLogical, "HTTPD_CONFIG");
            if (getenv(VMSRuleFileLogical) != NULL) {
                strcpy(VMSRuleFile, getenv(VMSRuleFileLogical));
            } else {
                /*
                **  No environment logical, so use the program logical.
                */
                strcpy(VMSRuleFile, RULE_FILE);
            }
        }
        free(VMSRuleFileLogical);
        role = passive;
    }
#endif /* VMS */

    WWW_TraceFlag = 0;          /* diagnostics off by default */
    HTImProxy = YES;            /* Always be able to run as proxy */

    time(&cur_time);

    HTDefaultConfig();

#ifdef VMS
   /* enable sysprv to get to rule file and allocate port */
   HTVMS_enableSysPrv();
#endif /* VMS */

#ifndef NO_INIT
    HTFileInit();       /* Initialize filename suffix table */
#endif

#ifdef VMS
    if (HTRunFromInetd) {
        if (HTLoadConfig(VMSRuleFile) < 0)
            exit(SS$_FILACCERR);
        rulefiles++;
        free(VMSRuleFile);
    } else
#endif /* VMS */

    {
        int a;

        for (a=1; a<argc; a++) {

            if (0==strcmp(argv[a], "-v") || 0==strcmp(argv[a], "-vv")) {
                WWW_TraceFlag = SHOW_ALL_TRACE;
                sc.no_bg = YES;
                CTRACE(stderr,
               "............ This is %s, version %s, using libwww version %s\n",
                       HTAppName, HTAppVersion, HTLibraryVersion);
                if (!strcmp(argv[a], "-vv"))
                    trace_all = YES;

            } else if (0==strcmp(argv[a], "-vc")) {
                trace_cache = YES;
                sc.no_bg = YES;
                fprintf(stderr, "CacheTrace.. On\n");

            } else if (0==strcmp(argv[a], "-version")) {
		printf("\n\nW3C Reference Software\n\n");
		printf("\tW3C HTTPD version %s.\n", HTAppVersion);
		printf("\tW3C Reference Library version %s.\n\n", HTLibraryVersion);
                exit(0);

            } else if (0==strcmp(argv[a], "-setuid")) {
                sc.do_setuid = YES;
                CTRACE(stderr, "Option...... SETUID feature on\n");

            } else if (0==strncmp(argv[a], "-cacheroot", 5)) {
                if (++a < argc) {
                    StrAllocCopy(cc.cache_root, argv[a]);
                    CTRACE(stderr, "Cache root.. \"%s\"\n", argv[a]);
                }

            } else if (0==strncmp(argv[a], "-gc_only", 3)) {
                CTRACE(stderr, "gc-only mode\n");
                gc_only = YES;

            } else if (0==strcmp(argv[a], "-restart")) {
                fprintf(stderr, "Restarting.. httpd\n");
                restart_only = YES;

            } else if (0==strcmp(argv[a], "-nobg")) {
                sc.no_bg = YES;

            } else if (0==strcmp(argv[a], "-nodns")) {
                sc.do_dns_lookup = NO;

            } else if (0==strcmp(argv[a], "-nolog")) {
                if (++a < argc) {
                    if (!sc.no_log) sc.no_log = HTList_new();
                    HTList_addObject(sc.no_log, (void*)argv[a]);
                }

            } else if (0==strcmp(argv[a], "-disable")) {
                if (++a < argc) {
                    HTMethod m = HTMethod_enum(argv[a]);
                    if (m != METHOD_INVALID) {
                        sc.disabled[(int)m] = YES;
                        CTRACE(stderr, "Disabled.... %s\n", HTMethod_name(m));
                    }
                    else HTLog_error2("Invalid method to disable:", argv[a]);
                }

            } else if (0==strcmp(argv[a], "-enable")) {
                if (++a < argc) {
                    HTMethod m = HTMethod_enum(argv[a]);
                    if (m != METHOD_INVALID) {
                        sc.disabled[(int)m] = NO;
                        CTRACE(stderr,"Enabled..... %s\n",HTMethod_name(m));
                    }
                    else HTLog_error2("Invalid method to enable:", argv[a]);
                }

            } else if (0==strcmp(argv[a], "-a")) {
                fprintf(stderr, "-a option is no longer supported -- use -p");
                HTLog_error("-a option is no longer supported -- use -p");
                exit(1);

            } else if (0==strcmp(argv[a], "-p")) {
                if (++a<argc) {
                    sc.port = atoi(argv[a]);
                    if (sc.port <=0 ) {
                        HTLog_error2("Invalid param to -p option:",argv[a]);
                        exit(1);
                    }
#ifdef FORKING
                    HTForkEnabled = YES;
#endif /* FORKING */
                }

            } else if (0==strcmp(argv[a], "-r")) {
                if (++a<argc) { 
                    if (!HTLoadConfig(argv[a])) exit(-1);
                    rulefiles++;
                }
            } else if (0==strcmp(argv[a], "-R")) {
                fprintf(stderr, "-R option is no longer supported\n");
                HTLog_error("-R option is no longer supported");
                exit(1);

#ifdef DIR_OPTIONS
            } else if (0==strncmp(argv[a], "-d", 2)) {
                char *p = argv[a]+2;
                for(;*p;p++) {
                    switch (*p) {
                      case 'b': HTDirReadme = HT_DIR_README_BOTTOM;     break;
                      case 'n': HTDirAccess = HT_DIR_FORBID;            break;
                      case 'r': HTDirReadme = HT_DIR_README_NONE;       break;
                      case 's': HTDirAccess = HT_DIR_SELECTIVE;         break;
                      case 't': HTDirReadme = HT_DIR_README_TOP;        break;
                      case 'y': HTDirAccess = HT_DIR_OK;                break;
                      default:
                        fprintf(stderr, 
                                "ERROR....... bad -d option %s\n", argv[a]);
                        HTLog_error2("Bad -d option", argv[a]);
                        exit(-4);
                    }
                } /* loop over characters */
#endif
            } else if (0==strcmp(argv[a], "-gmt")) {
                CTRACE(stderr, "Logging..... in GMT\n");
                sc.use_gmt = YES;

            } else if (0==strcmp(argv[a], "-localtime")) {
                CTRACE(stderr, "Logging..... in local time\n");
                sc.use_gmt = NO;

            } else if (0==strcmp(argv[a], "-l") ||
                       0==strcmp(argv[a], "-newlog") ||
                       0==strcmp(argv[a], "-oldlog")) { /* template */
                if (argv[a][1] == 'o')
                    sc.new_logfile_format = NO;
                if (++a<argc) {
                    StrAllocCopy(sc.access_log_name, argv[a]);
                }
                else {
                    fprintf(stderr,
                            "ERROR....... No parameter for -l option\n");
                    HTLog_error("No parameter for -l/-newlog/-oldlog option");
                }

            } else if (0==strcmp(argv[a], "-errlog")) { /* template */
                if (++a<argc) {
                    StrAllocCopy(sc.error_log_name,argv[a]);
                }
                else {
                    fprintf(stderr,
                            "ERROR....... No parameter for -errlog option\n");
                    HTLog_error("No parameter for -errlog option");
                }

            } else if (argv[a][0] != '-') {     /* Parameter */
                if (!directory) directory = argv[a];
                else {
                    fprintf(stderr,"Extra parameter: %s\n",argv[a]);
                    HTLog_error2("Extra command line parameter:",argv[a]);
                    exit(1);
                }
            } else {
                fprintf(stderr,"Unknown command line option: %s\n", argv[a]);
                HTLog_error2("Unknown command line option:",argv[a]);
                exit(1);
            }
        } /* for each arg */
    } /* scope of a */

    if (!rulefiles) {
        if (!directory && !HTLoadConfig(RULE_FILE))   /* Default rule file? */
            directory = DEFAULT_EXPORT;
        if (directory) {
            char * mapto = (char*)malloc(strlen(directory) + 3);
            sprintf(mapto, "%s/*", directory);
            HTAppendRule(HT_Pass, "/*", mapto);
            free(mapto);
        }
    }
    else if (directory) {
        HTLog_error2("Warning: -r specified so dir param ignored:",directory);
    }

#ifdef FORKING
    if (restart_only || gc_only)
        goto skip_inits;
#endif /* FORKING */

    HTServerInit();

    if (sc.server_type == SERVER_TYPE_STANDALONE) {
        if (!sc.port) {
            sc.port = 80;
            CTRACE(stderr,"Default..... port 80 for standalone server\n");
        }
    }
    else if (sc.server_type == SERVER_TYPE_INETD) {
        if (sc.port) {
            sc.port = 0;
            CTRACE(stderr,"Port........ ignored for inetd type server\n");
        }
    }
    else if (sc.port) {
        sc.server_type = SERVER_TYPE_STANDALONE;
        CTRACE(stderr, "Default..... server type StandAlone (Port defined)\n");
    }
    else {      
        sc.server_type = SERVER_TYPE_INETD;
        CTRACE(stderr, "Default..... server type Inetd (no Port specified)\n");
    }

    if (sc.server_type == SERVER_TYPE_STANDALONE) {
#ifdef FORKING
        HTForkEnabled = YES;
        CTRACE(stderr, "ServerType.. standalone (turning on forking)\n");
#else /* not FORKING */
        CTRACE(stderr, "ServerType.. standalone non-forking\n");
#endif /* not FORKING */
    }
    else {
        CTRACE(stderr, "ServerType.. inetd\n");
    }

    status = do_bind(sc.port);
    if (status<0) {
        HTLog_error(
          "Can't bind and listen on port (maybe httpd already running)");
        HTLog_error(
          "If running from inetd DO NOT use -p option or Port directive");
        fprintf(stderr,
          "HTTPD ERROR: Bad setup: Can't bind and listen on port.\n");
        fprintf(stderr,
          "Explanation: Possibly server already running, or if running\n");
        fprintf(stderr,
          "from inetd make sure you're not using -p flag or Port directive\n");
        exit(status);
    }

#ifdef VMS
    /*
    **  Disable all privileges except TMPMBX and NETMBX.
    **  We can then turn SYSPRV on and off selectively.
    */
    HTVMS_disableAllPrv();

    /*
    **  Set default to the default data tree root, if defined.
    */
    if (sc.server_root) {
    	char *Server_Root;
	if (strchr(sc.server_root, '/') != NULL)
	    Server_Root = HTVMS_name("", sc.server_root);
	else
	    Server_Root = sc.server_root;
        status = chdir(Server_Root);
	if (status<0) {
	    HTLog_error2("Can't set default to:", Server_Root);
    	    fprintf(stderr,
		    "HTTPD ERROR: Bad setup: Can't set default to %s.\n",
		    	   Server_Root);
	    exit(0);
	} else {
	    CTRACE(stderr,"Daemon...... Set default to %s\n", Server_Root);
	}
    }
#endif /* not VMS */

#ifndef VMS
    /*
     * Set parent process uid and gid
     */
    if (getuid() == 0 && sc.parent_uid) {
        struct passwd *pw = NULL;
        struct group *gr = NULL;
        int gid = 0;

        if (!(pw = getpwnam(sc.parent_uid)))
            HTLog_error2("Can't get passwd entry for user",sc.parent_uid);
        else if (initgroups(pw->pw_name, pw->pw_gid) == -1)
            HTLog_error2("Can't init groups for user",pw->pw_name);

        if (sc.parent_gid) {
            gr = getgrnam(sc.parent_gid);
            if (!gr) HTLog_error2("Can't get group entry for",sc.parent_gid);
        }

        if (gr)
            gid = gr->gr_gid;
        else if (pw)
            gid = pw->pw_gid;

        if (gid) {
            CTRACE(stderr, "Doing....... setgid(%d)\n", gid);
            if (setgid(gid) == -1)
                HTLog_errorN("Failed to set parent group id to", gid);
        }
        if (pw && pw->pw_uid) {
            CTRACE(stderr, "Doing....... setuid(%d)\n", (int) pw->pw_uid);
            if (setuid(pw->pw_uid) == -1)
                HTLog_errorN("Failed to set parent user id to", pw->pw_uid);
        }
    }
#endif /* not VMS */
    
#ifdef FORKING
    /*
     * Go to background and disconnect from the terminal
     * when run standalone.
     */
    if (sc.port && !sc.no_bg && !restart_only)
        daemon_start();

    set_signals();

    /*
     * Write the process id to pid_file or restart server.
     */
  skip_inits:
    if (!gc_only && (sc.port || restart_only)) {
        FILE * fp = NULL;
        char * fn = NULL;
        if (sc.pid_file && sc.pid_file[0] == '/')
            StrAllocCopy(fn, sc.pid_file);
        else {
            StrAllocCopy(fn, sc.server_root ? sc.server_root : "/tmp");
            StrAllocCat(fn, "/");
            StrAllocCat(fn, sc.pid_file ? sc.pid_file : "httpd-pid");
        }
        if (restart_only) {
            int pid;
            fp = fopen(fn, "r");
            if (!fp) {
                fprintf(stderr,"Error....... Can't open pid file %s\n",fn);
                exit(1);
            }
            if (fscanf(fp,"%d",&pid) != 1) {
                fprintf(stderr,"Error....... Can't read pid from %s\n",fn);
                exit(1);
            }
            fprintf(stderr,"Sending..... HUP signal to process %d\n",pid);
            if (kill(pid,SIGHUP) == -1) {
                fprintf(stderr, "Error....... %s\n",
                        errno==EPERM ? "permission denied" :
                        errno==ESRCH ? "no such process" : "dunno");
                exit(1);
            }
            exit(0);
        }
        else {
            fp = fopen(fn, "w");
            if (!fp) HTLog_error2("Can't open pid file for writing:",fn);
            else {
                CTRACE(stderr, "PidFile..... %s\n", fn);
                fprintf(fp,"%d\n",(int)getpid());
                fclose(fp);
            }
        }
        free(fn);
    }

#endif /* FORKING */

#ifndef VMS
    if (cc.cache_root) {
        gc_info_file = (char*)malloc(strlen(cc.cache_root) + 20);
        sprintf(gc_info_file, "%s/.gc_info", cc.cache_root);
        CTRACE(stderr, "Recovering.. cache to last gc state...\n");
        read_gc_results();
        if (gc_only)
            gc_and_exit(2,0);
    }
    else if (gc_only) {
        fprintf(stderr,
        "ERROR: Cache root not specified when started in gc-only mode\n");
        HTLog_error("Cache root not specified when started in gc-only mode");
        exit(-1);
    }
#endif /* not VMS */

    status = HTUserInit();
    if (status<0) {
       if (role != passive)
           exit(status);
       else
           exit(0);
    }

    status = server_loop();
    if (status<0 && role != passive) {
        exit(status);
    }

    exit(0);
    return 0;   /* NOTREACHED -- For gcc */
}

